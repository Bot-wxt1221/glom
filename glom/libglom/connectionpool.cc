/* Glom
 *
 * Copyright (C) 2001-2004 Murray Cumming
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 */

#include <libglom/connectionpool.h>
#include <libglom/document/document.h>
#include <libglom/utils.h>
#include <libglom/db_utils.h>
#include <libglom/connectionpool_backends/postgres_central.h>
#include <libglom/connectionpool_backends/postgres_self.h>
#include <libglom/connectionpool_backends/sqlite.h>
#include <libglom/connectionpool_backends/mysql_central.h>
#include <libglom/connectionpool_backends/mysql_self.h>

#include <glibmm/main.h>

#ifdef G_OS_WIN32
# include <windows.h>
#else
# include <libepc/shell.h> //For epc_shell_set_progress_hooks().
# include <libepc/publisher.h>
#endif

#include <csignal> //To catch segfaults

#include <glibmm/convert.h>
#include <glibmm/miscutils.h>

#include <iostream>


#ifndef G_OS_WIN32
static EpcProtocol publish_protocol = EPC_PROTOCOL_HTTPS;
#endif

// Uncomment to see debug messages
//#define GLOM_CONNECTION_DEBUG

namespace Glom
{

SharedConnection::SharedConnection()
{
}

SharedConnection::SharedConnection(const Glib::RefPtr<Gnome::Gda::Connection>& gda_connection)
: m_gda_connection(gda_connection)
{
}

SharedConnection::~SharedConnection()
{
  if(m_gda_connection)
    m_signal_finished.emit();
}

Glib::RefPtr<Gnome::Gda::Connection> SharedConnection::get_gda_connection()
{
  return m_gda_connection;
}

Glib::RefPtr<const Gnome::Gda::Connection> SharedConnection::get_gda_connection() const
{
  return m_gda_connection;
}

SharedConnection::type_signal_finished SharedConnection::signal_finished()
{
  return m_signal_finished;
}

void SharedConnection::close()
{
  if(m_gda_connection)
  {
    m_gda_connection.reset();
  }

  //Tell the connection pool that we have finished with this connection.
  //It might want to close it, or keep it open if somebody else is using it.
  //It might even give it to someone else while it is waiting for that other person to finish with it.
  m_signal_finished.emit();
}

//init_db_details static data:
std::shared_ptr<ConnectionPool> ConnectionPool::m_instance;

ConnectionPool::ConnectionPool()
:
  m_epc_publisher(nullptr),
  m_sharedconnection_refcount(0),
  m_ready_to_connect(false),
  m_show_debug_output(false),
  m_auto_server_shutdown(true),
  m_fake_connection(false)
{
}

//static
std::shared_ptr<ConnectionPool> ConnectionPool::get_instance()
{
  //TODO: Synchronize this for threads?
  if(m_instance)
    return m_instance;
  else
  {
    m_instance = std::make_shared<ConnectionPool>();
    return m_instance;
  }
}

void ConnectionPool::setup_from_document(const std::shared_ptr<const Document>& document)
{
  switch(document->get_hosting_mode())
  {
  case Document::HostingMode::POSTGRES_SELF:
    {
      auto backend = std::make_shared<ConnectionPoolBackends::PostgresSelfHosted>();
      backend->set_database_directory_uri(document->get_connection_self_hosted_directory_uri());
      set_backend(backend);
    }
    break;
  case Document::HostingMode::POSTGRES_CENTRAL:
    {
      auto backend = std::make_shared<ConnectionPoolBackends::PostgresCentralHosted>();
      backend->set_host(document->get_connection_server());
      backend->set_port(document->get_connection_port());
      backend->set_try_other_ports(document->get_connection_try_other_ports());
      set_backend(backend);
    }
    break;
  case Document::HostingMode::SQLITE:
    {
      auto backend = std::make_shared<ConnectionPoolBackends::Sqlite>();
      backend->set_database_directory_uri(document->get_connection_self_hosted_directory_uri());
      set_backend(backend);
    }
    break;
  case Document::HostingMode::MYSQL_SELF:
    {
      auto backend = std::make_shared<ConnectionPoolBackends::MySQLSelfHosted>();
      backend->set_database_directory_uri(document->get_connection_self_hosted_directory_uri());
      set_backend(backend);
    }
    break;
  case Document::HostingMode::MYSQL_CENTRAL:
    {
      auto backend = std::make_shared<ConnectionPoolBackends::MySQLCentralHosted>();
      backend->set_host(document->get_connection_server());
      backend->set_port(document->get_connection_port());
      backend->set_try_other_ports(document->get_connection_try_other_ports());
      set_backend(backend);
    }
    break;

  default:
    //on_document_load() should have checked for this already, informing the user.
    std::cerr << G_STRFUNC << ": Unhandled hosting mode: " << Utils::to_utype(document->get_hosting_mode()) << std::endl;
    g_assert_not_reached();
    break;
  }

  // Might be overwritten later when actually attempting a connection:
  set_user(document->get_connection_user());
  set_database(document->get_connection_database());

  set_ready_to_connect();
}

void ConnectionPool::delete_instance()
{
  m_instance.reset();
}

bool ConnectionPool::get_ready_to_connect() const
{
  return m_ready_to_connect;
}

void ConnectionPool::set_ready_to_connect(bool val)
{
  m_ready_to_connect = val;
}

void ConnectionPool::set_backend(const std::shared_ptr<Backend>& backend)
{
  m_backend = backend;
}

std::shared_ptr<ConnectionPool::Backend> ConnectionPool::get_backend()
{
  return m_backend;
}

std::shared_ptr<const ConnectionPool::Backend> ConnectionPool::get_backend() const
{
  return m_backend;
}

bool ConnectionPool::get_backend_supports_cursor() const
{
  //TODO: Is there a generic libgda way to discover this?
  const auto sqlite_backend = std::dynamic_pointer_cast<const ConnectionPoolBackends::Sqlite>(get_backend());
  return !sqlite_backend;
}

//static:
std::shared_ptr<SharedConnection> ConnectionPool::get_and_connect()
{
  std::shared_ptr<SharedConnection> result;

  auto connection_pool = ConnectionPool::get_instance();
  if(!connection_pool)
    return result;

  if(!(connection_pool->m_backend))
  {
    std::cerr << G_STRFUNC << ": m_backend is null.\n";
    return result; //TODO: Return a failure_type::NO_BACKEND error?, though that would be tedious.
  }

  result = connection_pool->connect();

  return result;
}

bool ConnectionPool::get_instance_is_ready()
{
  auto instance = get_instance();
  if(!instance)
    return false;

  return instance->get_ready_to_connect();
}


// Store the connection for a few seconds in case it
// is immediately requested again, to avoid making a new connection
// and introspecting again, which is slow.
// TODO: Why aren't these member variables?
static std::shared_ptr<SharedConnection> connection_cached;
static sigc::connection connection_cached_timeout_connection;
static sigc::connection connection_cached_finished_connection;

static bool on_connection_pool_cache_timeout()
{
  //std::cout << "DEBUG: Clearing connection cache.\n";

  //Forget the cached connection after a few seconds:
  connection_cached.reset();

  return false; //Don't call this again.
}


std::shared_ptr<SharedConnection> ConnectionPool::connect()
{
  //std::cout << G_STRFUNC << ": debug\n";

  //Don't try to connect if we don't have a backend to connect to.
  g_return_val_if_fail(m_backend, std::shared_ptr<SharedConnection>());

  if(get_ready_to_connect() || m_fake_connection)
  {
    if(connection_cached)
    {
      //Avoid a reconnection immediately after disconnecting:
      return connection_cached;
    }
    //If the connection is already open (because it is being used by somebody):
    else if(m_gda_connection)
    {
      std::shared_ptr<SharedConnection> sharedConnection( new SharedConnection(m_gda_connection) );

      //Ask for notification when the SharedConnection has been finished with:
      //TODO: Note that we are overwriting the connection to a signal of a
      //previous sharedconnection here. This can be problematic when
      //invalidate_connection() is called, because then we don't disconnect
      //from the signal of the previous instance, and when the signal
      //handler is called later we might decrement the reference count for
      //a completely different shared connection.
      connection_cached_finished_connection = sharedConnection->signal_finished().connect( sigc::mem_fun(*this, &ConnectionPool::on_sharedconnection_finished) );

      //Remember that somebody is using it:
      m_sharedconnection_refcount++;

      //Store the connection in a cache for a few seconds to avoid unnecessary disconnections/reconnections:
      //std::cout << "DEBUG: Stored connection cache.\n";
      connection_cached = sharedConnection;
      connection_cached_timeout_connection.disconnect(); //Stop the existing timeout if it has not run yet.
      connection_cached_timeout_connection = Glib::signal_timeout().connect_seconds(&on_connection_pool_cache_timeout, 30 /* seconds */);

      return sharedConnection;
    }
    else
    {
      m_gda_connection = m_backend->connect(m_database, get_user(), get_password(), m_fake_connection);

      {
        //Allow get_meta_store_data() to succeed:
        //Hopefully this (and the update_meta_store_table() calls) is all we need.
        //std::cout << "DEBUG: Calling update_meta_store_data_types() ...\n";
        try
        {
          m_gda_connection->update_meta_store_data_types();
        }
        catch(const Glib::Error& ex)
        {
          //If the conneciton was not opened, because it is a fake connection,
          //then we should not be surprised that this fails,
          //and a warning will only be useful later when get_meta_store_data() fails when used in FieldTypes, from Field::set_*().
          if(!m_fake_connection)
          {
            std::cerr << G_STRFUNC << ": update_meta_store_data_types() failed: " << ex.what() << std::endl;
          }
        }
        //std::cout << "DEBUG: ... update_meta_store_data_types() has finished.\n";

        //std::cout << "DEBUG: Calling update_meta_store_table_names() ...\n";

        if(!update_meta_store_for_table_names()) {
          std::cerr << G_STRFUNC << ": update_meta_store_table_names() failed without an exception.\n";
        }
        //std::cout << "DEBUG: ... update_meta_store_table_names() has finished.\n";

        // Connection succeeded
        // Create the fieldtypes member if it has not already been done:
        if(!m_field_types)
          m_field_types = std::make_shared<FieldTypes>(m_gda_connection);

#ifndef G_OS_WIN32
        //Let other clients discover this server via avahi:
        //TODO: Only advertize if we are the first to open the document,
        //to avoid duplicates.
        //TODO: Only advertize if it makes sense for the backend,
        //it does not for sqlite
        auto document = get_document();
        if(document && document->get_network_shared())
          avahi_start_publishing(); //Stopped in the signal_finished handler.
#endif // !G_OS_WIN32

        return connect(); //Call this method recursively. This time m_gda_connection exists.
      }
    }
  }
  else
  {
    //std::cerr << G_STRFUNC << ": not ready to connect.\n";
  }

  return std::shared_ptr<SharedConnection>();
}

void ConnectionPool::create_database(const SlotProgress& slot_progress, const Glib::ustring& database_name)
{
  if(m_backend)
    m_backend->create_database(slot_progress, database_name, get_user(), get_password());
}

void ConnectionPool::set_user(const Glib::ustring& value)
{
  if(value.empty())
  {
#ifdef GLOM_CONNECTION_DEBUG
    std::cout << "debug: " << G_STRFUNC << ": user is empty.\n";
#endif
  }

  m_user = value;

  //Make sure that connect() makes a new connection:
  invalidate_connection();
}

bool ConnectionPool::save_backup(const SlotProgress& slot_progress, const std::string& path_dir)
{
  g_assert(m_backend);

  const auto old_uri = m_backend->get_database_directory_uri();

  std::string uri;
  try
  {
    //TODO_MySQL:
    //TODO: Avoid the copy/paste of glom_postgres_data and make it work for sqlite too.
    const auto subdir = Glib::build_filename(path_dir, "glom_postgres_data");
    uri = Glib::filename_to_uri(subdir);
  }
  catch(const Glib::Error& ex)
  {
    std::cerr << G_STRFUNC << ": exception from Glib::build_filename(): " << ex.what() << std::endl;
    return false;
  }

  m_backend->set_database_directory_uri(uri);
  const auto result = m_backend->save_backup(slot_progress, m_user, m_password, m_database);
  m_backend->set_database_directory_uri(old_uri);
  return result;
}

bool ConnectionPool::convert_backup(const SlotProgress& slot_progress, const std::string& backup_data_file_path)
{
  g_assert(m_backend);

  const auto result = m_backend->convert_backup(slot_progress, backup_data_file_path, m_user, m_password, m_database);
  if(!result)
    return false;

  try
  {
    //update_meta_store_table_names() has been known to throw an exception.
    //Glom is mostly unusable when it fails, but that's still better than a crash.
    m_gda_connection->update_meta_store_table_names(m_backend->get_public_schema_name());
  }
  catch(const Glib::Error& ex)
  {
    //If the conneciton was not opened, because it is a fake connection,
    //then we should not be surprised that this fails,
    //and a warning will only be useful later when get_meta_store_data() fails when used in get_table_names_from_database().
    if(!m_fake_connection)
    {
      std::cerr << G_STRFUNC << ": ConnectionPool::connect(): update_meta_store_table_names() failed: " << ex.what() << std::endl;
    }
  }

  return result;
}

void ConnectionPool::set_password(const Glib::ustring& value)
{
  m_password = value;

  //Make sure that connect() makes a new connection:
  invalidate_connection();
}

void ConnectionPool::set_database(const Glib::ustring& value)
{
  m_database = value;

  //Make sure that connect() makes a new connection:
  invalidate_connection();
}

Glib::ustring ConnectionPool::get_user() const
{
  return m_user;
}

Glib::ustring ConnectionPool::get_password() const
{
  return m_password;
}

Glib::ustring ConnectionPool::get_database() const
{
  return m_database;
}

std::shared_ptr<const FieldTypes> ConnectionPool::get_field_types() const
{
  //TODO: Investigate this:
  //if(!m_field_types)
  //  std::cerr << G_STRFUNC << ": m_field_types is null but this should never happen.\n";

  return m_field_types;
}

Gnome::Gda::SqlOperatorType ConnectionPool::get_string_find_operator() const
{
  g_assert(m_backend);
  return m_backend->get_string_find_operator();
}

void ConnectionPool::invalidate_connection()
{
  //std::cerr << G_STRFUNC << ": debug\n";
  connection_cached.reset();
  connection_cached_timeout_connection.disconnect();
  connection_cached_finished_connection.disconnect();

  if(m_gda_connection)
    m_gda_connection->close();

  m_gda_connection.reset();
  m_sharedconnection_refcount = 0;

  m_field_types.reset();
}

void ConnectionPool::on_sharedconnection_finished()
{
  //g_warning("ConnectionPool::on_sharedconnection_finished().");

  //One SharedConnection is no longer being used:
  m_sharedconnection_refcount--;

  //If this was the last user of SharedConnection then we can close the connection.
  if(m_sharedconnection_refcount == 0)
  {
    //There should be no copies of the m_refConnection, so the Gnome::Gda::Connection destructor should
    //run when we clear this last RefPtr of it, but we will explicitly close it just in case.
    //std::cerr << G_STRFUNC << ": closing GdaConnection\n";
    m_gda_connection->close();

    m_gda_connection.reset();

#ifndef G_OS_WIN32
    //TODO: this should only even be started if we are the first to open the .glom file:
    avahi_stop_publishing();
#endif

    //g_warning("ConnectionPool: connection closed");
  }
}

//static
bool ConnectionPool::handle_error_cerr_only()
{
  auto sharedconnection = get_and_connect();

  if(sharedconnection)
  {
    auto gda_connection = sharedconnection->get_gda_connection();

    const auto list_errors = gda_connection->get_events();

    if(!list_errors.empty())
    {
      Glib::ustring error_details;
      for(const auto& event : list_errors)
      {
        if(event && (event->get_event_type() == Gnome::Gda::CONNECTION_EVENT_ERROR))
        {
          if(!error_details.empty())
            error_details += '\n'; //Add newline after each error.

          error_details += event->get_description();
          std::cerr << G_STRFUNC << ": Internal error (Database): " << error_details << std::endl;
        }
      }

      return true; //There really was an error.
    }
  }

  //There was no error. libgda just did not return any data, and has no concept of an empty datamodel.
  return false;
}

#ifdef G_OS_WIN32
// TODO: This is probably mingw specific
static __p_sig_fn_t previous_sig_handler = SIG_DFL;
#else
#ifdef __APPLE__
static sig_t previous_sig_handler = SIG_DFL;
#else
static sighandler_t previous_sig_handler = SIG_DFL; /* Arbitrary default */
#endif
#endif

/* This is a Linux/Unix signal handler,
* so we can respond to a crash.
*/
static void on_linux_signal(int signum)
{
  auto connection_pool = ConnectionPool::get_instance();
  if(!connection_pool)
    return;

  if(signum == SIGSEGV)
  {
    ConnectionPool::SlotProgress slot_ignored;
    connection_pool->cleanup(slot_ignored);

    //Let GNOME/Ubuntu's crash handler still handle this?
    if(previous_sig_handler)
      (*previous_sig_handler)(signum);
    else
      exit(1);
  }
}

ConnectionPool::StartupErrors ConnectionPool::startup(const SlotProgress& slot_progress, bool network_shared)
{
  if(!m_backend)
    return Backend::StartupErrors::FAILED_UNKNOWN_REASON;

  const auto started = m_backend->startup(slot_progress, network_shared);
  if(started != Backend::StartupErrors::NONE)
    return started;

#ifndef G_OS_WIN32
  //Let clients discover this server via avahi:
  //avahi_start_publishing();
#endif // !G_OS_WIN32

  //If we crash while running (unlikely, hopefully), then try to cleanup.
  //Comment this out if you want to see the backtrace in a debugger.
  if(m_auto_server_shutdown)
    previous_sig_handler = signal(SIGSEGV, &on_linux_signal);

  return started;
}

bool ConnectionPool::cleanup(const SlotProgress& slot_progress)
{
  // Without a valid backend instance we should not state that we are ready to
  // connect. Fixes crash described in #577821.

  // TODO: Implement checks in set_ready_to_connect() to only set the flag to
  // "true" when the ConnectionPool instance is in a consistent state
  // afterwards. Or, have a private set_ready_to_connect() and a public
  // set_not_ready_to_connect()?
  set_ready_to_connect(false);

  bool result = false;

  //Make sure that no connections are still open.
  //Otherwise database shutdown could fail.
  //And make sure that connect() tries to make a new connection:
  invalidate_connection();

  if(m_backend)
    result = m_backend->cleanup(slot_progress);

#ifndef G_OS_WIN32
  /* Stop advertising the self-hosting database server via avahi: */
  //avahi_stop_publishing();
#endif // !G_OS_WIN32

  //We don't need the segfault handler anymore:
  if(previous_sig_handler != SIG_DFL) /* Arbitrary default */
  {
    signal(SIGSEGV, previous_sig_handler);
    previous_sig_handler = SIG_DFL; /* Arbitrary default */
  }

  return result;
}


bool ConnectionPool::set_network_shared(const SlotProgress& slot_progress, bool network_shared)
{
  if(m_backend)
    return m_backend->set_network_shared(slot_progress, network_shared);
  else
    return false;
}

bool ConnectionPool::connect_nothrow()
{
  if(!m_gda_connection)
  {
    try
    {
      connect();
    }
    catch (const Glib::Error& ex)
    {
      std::cerr << G_STRFUNC << ": connect() failed: " << ex.what() << std::endl;
      return false;
    }
  }

  return (bool)m_gda_connection;
}

//TODO: Why do we use noexcept here and on change_columns()?
bool ConnectionPool::add_column(const Glib::ustring& table_name, const std::shared_ptr<const Field>& field) noexcept
{
  if(!connect_nothrow())
    return false;

  try
  {
    const auto result = m_backend->add_column(m_gda_connection, table_name, field);
    m_gda_connection->update_meta_store_table(table_name, m_backend->get_public_schema_name());
    return result;
  }
  catch(const Glib::Error& ex)
  {
    std::cerr << G_STRFUNC << ": exception:" << ex.what() << std::endl;
  }

  return false;
}

bool ConnectionPool::drop_column(const Glib::ustring& table_name, const Glib::ustring& field_name) noexcept
{
  if(!connect_nothrow())
    return false;

  try
  {
    const auto result = m_backend->drop_column(m_gda_connection, table_name, field_name);
    m_gda_connection->update_meta_store_table(table_name, m_backend->get_public_schema_name());
    return result;
  }
  catch(const Glib::Error& ex)
  {
    std::cerr << G_STRFUNC << ": exception:" << ex.what() << std::endl;
  }

  return false;
}

bool ConnectionPool::change_column(const Glib::ustring& table_name, const std::shared_ptr<const Field>& field_old, const std::shared_ptr<const Field>& field) noexcept
{
  type_vec_const_fields old_fields(1, field_old);
  type_vec_const_fields new_fields(1, field);

  return change_columns(table_name, old_fields, new_fields);
}

bool ConnectionPool::change_columns(const Glib::ustring& table_name, const type_vec_const_fields& old_fields, const type_vec_const_fields& new_fields) noexcept
{
  if(!connect_nothrow())
    return false;

  const auto result = m_backend->change_columns(m_gda_connection, table_name, old_fields, new_fields);

  try
  {
    m_gda_connection->update_meta_store_table(table_name, m_backend->get_public_schema_name());
  }
  catch(const Glib::Error& ex)
  {
    std::cerr << G_STRFUNC << ": update_meta_store_table() failed: " << ex.what() << std::endl;
    return false;
  }

  if(!result)
    return false;

  //Add or remove any auto-increment rows:
  //The new auto-increment row would actually be added automatically,
  //but this makes it available even before a record has been added.
  auto iter_old = old_fields.begin();
  auto iter_new = new_fields.begin();
  while( (iter_old != old_fields.end()) && (iter_new != new_fields.end()) )
  {
    const auto field_old = *iter_old;
    const auto field_new = *iter_new;
    if(field_old && field_new
      && field_old->get_auto_increment() != field_new->get_auto_increment())
    {
      if(field_new->get_auto_increment())
        DbUtils::auto_increment_insert_first_if_necessary(table_name, field_new->get_name());
      else
        DbUtils::remove_auto_increment(table_name, field_new->get_name());
    }

    ++iter_old;
    ++iter_new;
  }

  return result;
}

ConnectionPool::InitErrors ConnectionPool::initialize(const SlotProgress& slot_progress, bool network_shared)
{
  if(m_backend)
    return m_backend->initialize(slot_progress, get_user(), get_password(), network_shared);
  else
    return Backend::InitErrors::OTHER;
}

std::shared_ptr<Document> ConnectionPool::get_document()
{
  if(!m_slot_get_document)
  {
    //Don't bother warning because all the code that calls get_document() checks
    //for 0 and responds reasonably.
    //std::cerr << G_STRFUNC << ": m_slot_get_document is null.\n";
    return nullptr;
  }

  return m_slot_get_document();
}

#ifndef G_OS_WIN32
//static
EpcContents* ConnectionPool::on_publisher_document_requested(EpcPublisher* /* publisher */, const gchar* /* key */, gpointer user_data)
{
  auto connection_pool = static_cast<Glom::ConnectionPool*>(user_data);
  if(!connection_pool)
    return nullptr;

  const auto document = connection_pool->get_document();
  if(!document)
    return nullptr;

  const auto contents = document->get_contents();
  //std::cout << "debug: " << G_STRFUNC << ": returning: \n" << "  " << contents << std::endl;
  return epc_contents_new_dup ("text/plain", (void*)contents.c_str(), -1);
}




//static
gboolean ConnectionPool::on_publisher_document_authentication(EpcAuthContext* context, const gchar* user_name, gpointer user_data)
{
  g_return_val_if_fail(context, false);

  auto connection_pool = (ConnectionPool*)(user_data);
  g_return_val_if_fail(connection_pool, false);

  // Check if the username/password are correct:
  const auto password = epc_auth_context_get_password(context);
  g_return_val_if_fail(password, false); //TODO: This seems to happen once before this callback is called again properly.

  //std::cout << "debug: " << G_STRFUNC << ": username=" << user_name << ", password=" << password << std::endl;

  g_return_val_if_fail(connection_pool->m_backend.get(), false);

  //Attempt a connection with this username/password:
  std::shared_ptr<ExceptionConnection> error;
  auto connection = connection_pool->m_backend->connect(connection_pool->get_database(), user_name, password);

  if(connection)
  {
    //std::cout << "debug: " << G_STRFUNC << ": succeeded.\n";
    return true; //Succeeded.
  }
  else
  {
    //std::cout << "debug: " << G_STRFUNC << ": failed.\n";
    return false; //Failed.
  }
}


void ConnectionPool::on_epc_progress_begin(const gchar* /* title */, gpointer user_data)
{
  //We ignore the title parameter because there is no way that libepc could know what Glom wants to say.

  auto connection_pool = (ConnectionPool*)user_data;
  if(connection_pool && connection_pool->m_epc_slot_begin)
    connection_pool->m_epc_slot_begin();
}

void ConnectionPool::on_epc_progress_update(gdouble /* progress */, const gchar* /* message */, gpointer user_data)
{
  //We ignore the title parameter because there is no way that libepc could know what Glom wants to say.
  //TODO: Show the progress in a ProgressBar.

  auto connection_pool = (ConnectionPool*)user_data;
  if(connection_pool && connection_pool->m_epc_slot_progress)
    connection_pool->m_epc_slot_progress();
}

void ConnectionPool::on_epc_progress_end(gpointer user_data)
{
  auto connection_pool = (ConnectionPool*)user_data;
  if(connection_pool && connection_pool->m_epc_slot_done)
    connection_pool->m_epc_slot_done();
}

void ConnectionPool::set_avahi_publish_callbacks(const type_void_slot& slot_begin, const type_void_slot& slot_progress, const type_void_slot& slot_done)
{
  m_epc_slot_begin = slot_begin;
  m_epc_slot_progress = slot_progress;
  m_epc_slot_done = slot_done;
}


/** Advertise self-hosting via avahi:
 */
void ConnectionPool::avahi_start_publishing()
{
  // Don't advertize if the database cannot be accessed remotely anyway
  if(!m_backend->supports_remote_access())
    return;

  if(m_epc_publisher)
    return;
#ifdef GLOM_CONNECTION_DEBUG
  std::cout << "debug: ConnectionPool::avahi_start_publishing\n";
#endif

  //Publish the document contents over HTTPS (discoverable via avahi):
  const auto document = get_document();
  if(!document)
    return;

  m_epc_publisher = epc_publisher_new(document->get_database_title_original().c_str(), "glom", nullptr);
  epc_publisher_set_protocol(m_epc_publisher, publish_protocol);

  epc_publisher_add_handler(m_epc_publisher, "document", on_publisher_document_requested, this /* user_data */, nullptr);

  //Password-protect the document,
  //because the document's structure could be considered as a business secret:
  epc_publisher_set_auth_flags(m_epc_publisher, EPC_AUTH_PASSWORD_TEXT_NEEDED);
  epc_publisher_set_auth_handler(m_epc_publisher, "document", on_publisher_document_authentication, this /* user_data */, nullptr);

  //Install progress callback, so we can keep the UI responsive while libepc is generating certificates for the first time:
  EpcShellProgressHooks callbacks;
  callbacks.begin = &ConnectionPool::on_epc_progress_begin;
  callbacks.update = &ConnectionPool::on_epc_progress_update;
  callbacks.end = &ConnectionPool::on_epc_progress_end;
  epc_shell_set_progress_hooks(&callbacks, this, nullptr);

  //Prevent the consumer from seeing duplicates,
  //if multiple client computers advertize the same document:
  //
  //Defer announcement until a duplicate disappears:
  epc_publisher_set_collision_handling(m_epc_publisher, EPC_COLLISIONS_UNIQUE_SERVICE);
  //
  // How to uniquely-identify the service. TODO: Use some additional criteria, such as the real host name.
  if(!m_database.empty())
    epc_publisher_set_service_cookie(m_epc_publisher, m_database.c_str());

  //Start the publisher, serving HTTPS:
  GError* error = nullptr;
  epc_publisher_run_async(m_epc_publisher, &error);
  if(error)
  {
#ifdef GLOM_CONNECTION_DEBUG
    std::cout << "debug: " << G_STRFUNC << ": Error while running epc_publisher_run_async: " << error->message << std::endl;
#endif
    g_clear_error(&error);
  }
}

void ConnectionPool::avahi_stop_publishing()
{
  if(!m_backend->supports_remote_access())
    return;

  if(!m_epc_publisher)
    return;
#ifdef GLOM_CONNECTION_DEBUG
  std::cout << "debug: ConnectionPool::avahi_stop_publishing\n";
#endif

#ifndef G_OS_WIN32
  epc_publisher_quit(m_epc_publisher);
#endif // !G_OS_WIN32
  g_object_unref(m_epc_publisher);
  m_epc_publisher = nullptr;
}
#endif // !G_OS_WIN32

void ConnectionPool::set_get_document_func(const SlotGetDocument& slot)
{
  m_slot_get_document = slot;
}

void ConnectionPool::set_show_debug_output(bool val)
{
  m_show_debug_output = val;
}

bool ConnectionPool::get_show_debug_output() const
{
  return m_show_debug_output;
}

void ConnectionPool::set_auto_server_shutdown(bool val)
{
  m_auto_server_shutdown = val;
}

void ConnectionPool::set_fake_connection()
{
  m_fake_connection = true;

  //Set a fake username and password, to avoid a GError from gda_connection_new_from_string():
  set_user("glom_fake_user");
  set_password("glom_fake_password");
}

bool ConnectionPool::update_meta_store_for_table_names()
{
  try
  {
    //update_meta_store_table_names() has been known to throw an exception.
    //Glom is mostly unusable when it fails, but that's still better than a crash.
    //std::cout << G_STRFUNC << ": Before update_meta_store_table_name()\n";
    const auto test = m_gda_connection->update_meta_store_table_names(m_backend->get_public_schema_name());
    if(!test && !m_fake_connection)
    {
      std::cerr << G_STRFUNC << ": update_meta_store_table_names() failed without an exception.\n";
      return false;
    }
  }
  catch(const Glib::Error& ex)
  {
    //If the connection was not opened, because it is a fake connection,
    //then we should not be surprised that this fails,
    //and a warning will only be useful later when get_meta_store_data() fails when used in get_table_names_from_database().
    if(!m_fake_connection)
    {
      std::cerr << G_STRFUNC << ": update_meta_store_table_names() failed: " << ex.what() << std::endl;
      return false;
    }
  }

  return true;
}

} //namespace Glom
