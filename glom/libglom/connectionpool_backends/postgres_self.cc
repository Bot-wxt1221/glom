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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <libglom/connectionpool_backends/postgres_self.h>
#include <libglom/connectionpool.h>
#include <libglom/utils.h>
#include <libglom/db_utils.h>
#include <libglom/spawn_with_feedback.h>
#include <giomm/file.h>
#include <glib/gstdio.h> // For g_remove

#include <glibmm/convert.h>
#include <glibmm/miscutils.h>
#include <glibmm/stringutils.h>
#include <glibmm/regex.h>
#include <glibmm/i18n.h>

#include <libglom/gst-package.h>
#include <sstream> //For stringstream
#include <iostream>

#ifdef G_OS_WIN32
# include <windows.h>
# include <winsock2.h>
#else
# include <sys/types.h>
# include <sys/socket.h>
# include <errno.h>
# include <netinet/in.h> //For sockaddr_in
#endif

#include <signal.h> //To catch segfaults

// Uncomment to see debug messages
//#define GLOM_CONNECTION_DEBUG

namespace Glom
{

namespace ConnectionPoolBackends
{

//TODO: Do we need these sameuser lines?

// We need both <=8.3 and >=8.4 versions, because the ident line changed syntax
// incompatibly: http://www.postgresql.org/about/press/features84#security

#define DEFAULT_CONFIG_PG_HBA_LOCAL_8p3 \
"# TYPE  DATABASE    USER        CIDR-ADDRESS          METHOD\n" \
"\n" \
"# local is for Unix domain socket connections only\n" \
"# trust allows connection from the current PC without a password:\n" \
"local   all         all                               trust\n" \
"local   all         all                               ident sameuser\n" \
"local   all         all                               md5\n" \
"\n" \
"# TCP connections from the same computer, with a password:\n" \
"host    all         all         127.0.0.1    255.255.255.255    md5\n" \
"# IPv6 local connections:\n" \
"host    all         all         ::1/128               md5\n"

#define DEFAULT_CONFIG_PG_HBA_LOCAL_8p4 \
"# TYPE  DATABASE    USER        CIDR-ADDRESS          METHOD\n" \
"\n" \
"# local is for Unix domain socket connections only\n" \
"# trust allows connection from the current PC without a password:\n" \
"local   all         all                               trust\n" \
"local   all         all                               ident\n" \
"local   all         all                               md5\n" \
"\n" \
"# TCP connections from the same computer, with a password:\n" \
"host    all         all         127.0.0.1    255.255.255.255    md5\n" \
"# IPv6 local connections:\n" \
"host    all         all         ::1/128               md5\n"

#define DEFAULT_CONFIG_PG_HBA_REMOTE_EXTRA \
"\n" \
"# IPv4 local connections:\n" \
"host    all         all         0.0.0.0/0          md5\n" \
"# IPv6 local connections:\n" \
"host    all         all         ::1/128               md5\n"

#define DEFAULT_CONFIG_PG_HBA_REMOTE_8p3 \
DEFAULT_CONFIG_PG_HBA_LOCAL_8p3 \
DEFAULT_CONFIG_PG_HBA_REMOTE_EXTRA

#define DEFAULT_CONFIG_PG_HBA_REMOTE_8p4 \
DEFAULT_CONFIG_PG_HBA_LOCAL_8p3 \
DEFAULT_CONFIG_PG_HBA_REMOTE_EXTRA

static const int PORT_POSTGRESQL_SELF_HOSTED_START = 5433;
static const int PORT_POSTGRESQL_SELF_HOSTED_END = 5500;

static const char DEFAULT_CONFIG_PG_IDENT[] = "";
static const char FILENAME_DATA[] = "data";
static const char FILENAME_BACKUP[] = "backup";

PostgresSelfHosted::PostgresSelfHosted()
: m_network_shared(false)
{
  m_host = "localhost";
}

bool PostgresSelfHosted::get_self_hosting_active() const
{
  return m_port != 0;
}

unsigned int PostgresSelfHosted::get_port() const
{
  return m_port;
}

/** Try to install postgres on the distro, though this will require a
 * distro-specific patch to the implementation.
 */
bool PostgresSelfHosted::install_postgres(const SlotProgress& /* slot_progress */)
{
#if 0
  // This  is example code for Ubuntu, and possibly Debian,
  // using code from the gnome-system-tools Debian/Ubuntu patches.
  // (But please, just fix the dependencies instead. PostgreSQL is not optional.)
  //
  // You will also need to remove the "ifdef 0"s around the code in gst-package.[h|c],
  // and define DISTRO_SPECIFIC_POSTGRES_INSTALL_IMPLEMENTED above.

  //Careful. Maybe you want a different version.
  //Also, Glom will start its own instance of PostgreSQL, on its own port, when it needs to,
  //so there is no need to start a Glom service after installation at system startup,
  //though it will not hurt Glom if you do that.
  const gchar *packages[] = { "postgresql-8.1", 0 };
  const bool result = gst_packages_install(parent_window->gobj() /* parent window */, packages);
  if(result)
  {
    std::cout << "Glom: gst_packages_install() reports success." << std::endl;
    //Double-check, because gst_packages_install() incorrectly returns TRUE if it fails because
    //a) synaptic is already running, or
    //b) synaptic did not know about the package (no warning is shown in this case.)
    //Maybe gst_packages_install() never returns FALSE.
    return check_postgres_is_available_with_warning(); //This is recursive, but clicking Cancel will stop everything.
  }
  else
  {
    std::cout << "Glom: gst_packages_install() reports failure." << std::endl;
    return false; //Failed to install postgres.
  }
#else
  return false; //Failed to install postgres because no installation technique was implemented.
#endif // #if 0
}

Backend::InitErrors PostgresSelfHosted::initialize(const SlotProgress& slot_progress, const Glib::ustring& initial_username, const Glib::ustring& password, bool network_shared)
{
  m_network_shared = network_shared;

  if(m_database_directory_uri.empty())
  {
    std::cerr << G_STRFUNC << ": initialize: m_self_hosting_data_uri is empty." << std::endl;
    return INITERROR_OTHER;
  }

  if(initial_username.empty())
  {
    std::cerr << "PostgresSelfHosted::initialize(). Username was empty while attempting to create self-hosting database" << std::endl;
    return INITERROR_OTHER;
  }

  //Get the filepath of the directory that we should create:
  const std::string dbdir_uri = m_database_directory_uri;
  //std::cout << "debug: dbdir_uri=" << dbdir_uri << std::endl;

  if(file_exists_uri(dbdir_uri))
    return INITERROR_DIRECTORY_ALREADY_EXISTS;

  const std::string dbdir = Glib::filename_from_uri(dbdir_uri);
  //std::cout << "debug: dbdir=" << dbdir << std::endl;
  g_assert(!dbdir.empty());

  const bool dbdir_created = create_directory_filepath(dbdir);
  if(!dbdir_created)
  {
    std::cerr << "Couldn't create directory: " << dbdir << std::endl;

    return INITERROR_COULD_NOT_CREATE_DIRECTORY;
  }

  //Create the config directory:
  const std::string dbdir_config = get_self_hosting_config_path(true /* create */);
  if(dbdir_config.empty())
  {
    std::cerr << "Couldn't create the config directory: " << dbdir << std::endl;

    return INITERROR_COULD_NOT_CREATE_DIRECTORY;
  }

  //Create these files: environment  pg_hba.conf  pg_ident.conf  start.conf
  set_network_shared(slot_progress, m_network_shared); //Creates pg_hba.conf and pg_ident.conf

  //Check that there is not an existing data directory:
  const std::string dbdir_data = get_self_hosting_data_path(true /* create */);
  if(dbdir_data.empty())
  {
    std::cerr << "Couldn't create the data directory: " << dbdir << std::endl;

    return INITERROR_COULD_NOT_CREATE_DIRECTORY;
  }

  // initdb creates a new postgres database cluster:

  //Get file:// URI for the tmp/ directory:
  const std::string temp_pwfile = Glib::build_filename(Glib::get_tmp_dir(), "glom_initdb_pwfile");
  Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(temp_pwfile);
  const std::string temp_pwfile_uri = file->get_uri();
  const bool pwfile_creation_succeeded = create_text_file(temp_pwfile_uri, password);
  g_assert(pwfile_creation_succeeded);

  // Make sure to use double quotes for the executable path, because the
  // CreateProcess() API used on Windows does not support single quotes.
  const std::string command_initdb = "\"" + get_path_to_postgres_executable("initdb") + "\" -D \"" + dbdir_data + "\"" +
                                        " -U " + initial_username + " --pwfile=\"" + temp_pwfile + "\"";

  //Note that --pwfile takes the password from the first line of a file. It's an alternative to supplying it when prompted on stdin.
  const bool result = Glom::Spawn::execute_command_line_and_wait(command_initdb, slot_progress);
  if(!result)
  {
    std::cerr << "Error while attempting to create self-hosting database." << std::endl;
  }

  const int temp_pwfile_removed = g_remove(temp_pwfile.c_str()); //Of course, we don't want this to stay around. It would be a security risk.
  g_assert(temp_pwfile_removed == 0);

  return result ? INITERROR_NONE : INITERROR_COULD_NOT_START_SERVER;
}

Glib::ustring PostgresSelfHosted::get_postgresql_utils_version(const SlotProgress& slot_progress)
{
  Glib::ustring result;

  const std::string command = "\"" + get_path_to_postgres_executable("pg_ctl") + "\" --version";

  //The first command does not return, but the second command can check whether it succeeded:
  std::string output;
  const bool spawn_result = Glom::Spawn::execute_command_line_and_wait(command, slot_progress, output);
  if(!spawn_result)
  {
    std::cerr << "Error while attempting to discover the pg_ctl version." << std::endl;
    return result;
  }

  //Use a regex to get the version number:
  Glib::RefPtr<Glib::Regex> regex;

  //We want the characters at the end:
  const gchar VERSION_REGEX[] = "pg_ctl \\(PostgreSQL\\) (.*)";

  try
  {
    regex = Glib::Regex::create(VERSION_REGEX);
  }
  catch(const Glib::Error& ex)
  {
    std::cerr << "Glom: Glib::Regex::create() failed: " << ex.what() << std::endl;
    return result;
  }

  if(!regex)
    return result;

  typedef std::vector<Glib::ustring> type_vec_strings;
  const type_vec_strings vec = regex->split(output, Glib::REGEX_MATCH_NOTEMPTY);
  //std::cout << "DEBUG: output == " << output << std::endl;
  //std::cout << "DEBUG: vec.size() == " << vec.size() << std::endl;

  // We get, for instance, "\n" and 8.4.1" and "\n".
  for(type_vec_strings::const_iterator iter = vec.begin();
       iter != vec.end();
       ++iter)
  {
    const Glib::ustring str = *iter;
    if(!str.empty())
      return str; //Found.
  }

  return result;
}

float PostgresSelfHosted::get_postgresql_utils_version_as_number(const SlotProgress& slot_progress)
{
  float result = 0;

  const Glib::ustring version_str = get_postgresql_utils_version(slot_progress);

  Glib::RefPtr<Glib::Regex> regex;

  //We want the characters at the end:
  const gchar VERSION_REGEX[] = "^(\\d*)\\.(\\d*)";

  try
  {
    regex = Glib::Regex::create(VERSION_REGEX);
  }
  catch(const Glib::Error& ex)
  {
    std::cerr << "Glom: Glib::Regex::create() failed: " << ex.what() << std::endl;
    return result;
  }

  if(!regex)
    return result;

  typedef std::vector<Glib::ustring> type_vec_strings;
  const type_vec_strings vec = regex->split(version_str, Glib::REGEX_MATCH_NOTEMPTY);
  //std::cout << "DEBUG: str == " << version_str << std::endl;
  //std::cout << "DEBUG: vec.size() == " << vec.size() << std::endl;

  //We need to loop over the numbers because we get some "" items that we want to ignore:
  guint count = 0; //We want 2 numbers.
  for(type_vec_strings::const_iterator iter = vec.begin();
       iter != vec.end();
       ++iter)
  {
    //std::cout << "regex item: START" << *iter << "END" << std::endl;

    const Glib::ustring str = *iter;
    if(str.empty())
      continue;

    const float num = atoi(str.c_str());
    if(count == 0)
      result = num;
    else if(count == 1)
    {
      result += (0.1 * num);
      break;
    }

    ++count;
  }

  return result;
}


Backend::StartupErrors PostgresSelfHosted::startup(const SlotProgress& slot_progress, bool network_shared)
{
  m_network_shared = network_shared;

  // Don't risk random crashes, although this really shouldn't be called
  // twice of course.
  //g_assert(!get_self_hosting_active());

  if(get_self_hosting_active())
  {
    std::cerr << G_STRFUNC << ": Already started." << std::endl;
    return STARTUPERROR_NONE; //Just do it once.
  }

  const std::string dbdir_uri = m_database_directory_uri;

  if(!(file_exists_uri(dbdir_uri)))
  {
    //TODO: Use a return enum or exception so we can tell the user about this:
    std::cerr << G_STRFUNC << ": The data directory could not be found: " << dbdir_uri << std::endl;
    return STARTUPERROR_FAILED_UNKNOWN_REASON;
  }

  const std::string dbdir = Glib::filename_from_uri(dbdir_uri);
  g_assert(!dbdir.empty());

  const std::string dbdir_data = Glib::build_filename(dbdir, FILENAME_DATA);
  const Glib::ustring dbdir_data_uri = Glib::filename_to_uri(dbdir_data);
  if(!(file_exists_uri(dbdir_data_uri)))
  {
    const std::string dbdir_backup = Glib::build_filename(dbdir, FILENAME_BACKUP);
    const Glib::ustring dbdir_backup_uri = Glib::filename_to_uri(dbdir_backup);
    if(file_exists_uri(dbdir_backup_uri))
    {
      std::cerr << G_STRFUNC << ": There is no data, but there is backup data." << std::endl;
      //Let the caller convert the backup to real data and then try again:
      return STARTUPERROR_FAILED_NO_DATA_HAS_BACKUP_DATA;
    }
    else
    {
      std::cerr << "ConnectionPool::create_self_hosting(): The data sub-directory could not be found." << dbdir_data_uri << std::endl;
      return STARTUPERROR_FAILED_NO_DATA;
    }
  }

  //Attempt to ensure that the config files are correct:
  set_network_shared(slot_progress, m_network_shared); //Creates pg_hba.conf and pg_ident.conf

  const unsigned int available_port = discover_first_free_port(PORT_POSTGRESQL_SELF_HOSTED_START, PORT_POSTGRESQL_SELF_HOSTED_END);
  //std::cout << "debug: " << G_STRFUNC << ":() : debug: Available port for self-hosting: " << available_port << std::endl;
  if(available_port == 0)
  {
    //TODO: Use a return enum or exception so we can tell the user about this:
    std::cerr << G_STRFUNC << ": No port was available between " << PORT_POSTGRESQL_SELF_HOSTED_START << " and " << PORT_POSTGRESQL_SELF_HOSTED_END << std::endl;
    return STARTUPERROR_FAILED_UNKNOWN_REASON;
  }

  //TODO: Performance:
  const std::string port_as_text = Glib::Ascii::dtostr(available_port);

  // -D specifies the data directory.
  // -c config_file= specifies the configuration file
  // -k specifies a directory to use for the socket. This must be writable by us.
  // Make sure to use double quotes for the executable path, because the
  // CreateProcess() API used on Windows does not support single quotes.
  const std::string command_postgres_start = "\"" + get_path_to_postgres_executable("postgres") + "\" -D \"" + dbdir_data + "\" "
                                  + " -p " + port_as_text
                                  + " -i " //Equivalent to -h "*", which in turn is equivalent to listen_addresses in postgresql.conf. Listen to all IP addresses, so any client can connect (with a username+password)
                                  + " -c hba_file=\"" + dbdir + "/config/pg_hba.conf\""
                                  + " -c ident_file=\"" + dbdir + "/config/pg_ident.conf\""
                                  + " -k \"" + dbdir + "\""
                                  + " --external_pid_file=\"" + dbdir + "/pid\"";

  // Make sure to use double quotes for the executable path, because the
  // CreateProcess() API used on Windows does not support single quotes.
  const std::string command_check_postgres_has_started = "\"" + get_path_to_postgres_executable("pg_ctl") + "\" status -D \"" + dbdir_data + "\"";

  //For postgres 8.1, this is "postmaster is running".
  //For postgres 8.2, this is "server is running".
  //This is a big hack that we should avoid. murrayc.
  //
  //pg_ctl actually seems to return a 0 result code for "is running" and a 1 for not running, at least with Postgres 8.2,
  //so maybe we can avoid this in future.
  //Please do test it with your postgres version, using "echo $?" to see the result code of the last command.
  const std::string second_command_success_text = "is running"; //TODO: This is not a stable API. Also, watch out for localisation.

  //The first command does not return, but the second command can check whether it succeeded:
  const bool result = Glom::Spawn::execute_command_line_and_wait_until_second_command_returns_success(command_postgres_start, command_check_postgres_has_started, slot_progress, second_command_success_text);
  if(!result)
  {
    std::cerr << "Error while attempting to self-host a database." << std::endl;
    return STARTUPERROR_FAILED_UNKNOWN_REASON;
  }

  m_port = available_port; //Remember it for later.

  return STARTUPERROR_NONE;
}

void PostgresSelfHosted::show_active_connections()
{
  Glib::RefPtr<Gnome::Gda::SqlBuilder> builder =
      Gnome::Gda::SqlBuilder::create(Gnome::Gda::SQL_STATEMENT_SELECT);
  builder->select_add_field("*", "pg_stat_activity");
  builder->select_add_target("pg_stat_activity");
 
  Glib::RefPtr<Gnome::Gda::Connection> gda_connection = connect(m_saved_database_name, m_saved_username, m_saved_password);
  if(!gda_connection)
    std::cerr << G_STRFUNC << ": connection failed." << std::endl;
  
  Glib::RefPtr<Gnome::Gda::DataModel> datamodel = DbUtils::query_execute_select(builder);
  if(!datamodel)
    std::cerr << G_STRFUNC << ": pg_stat_activity SQL query failed." << std::endl;
  
  const int rows_count = datamodel->get_n_rows(); 
  if(datamodel->get_n_rows() < 1)
    std::cerr << G_STRFUNC << ": pg_stat_activity SQL query returned no rows." << std::endl;

  std::cout << "Active connections according to a pg_stat_activity SQL query:" << std::endl;
  const int cols_count = datamodel->get_n_columns();
  for(int row = 0; row < rows_count; ++row)
  {
    for(int col = 0; col < cols_count; ++col)
    {
      if(col != 0)
        std::cout << ", ";
        
      std::cout << datamodel->get_value_at(col, row).to_string();
    }
    
    std::cout << std::endl;
  }
  
  //Make sure that this connection does not stop a further attempt to stop the server.
  gda_connection->close();
}

bool PostgresSelfHosted::cleanup(const SlotProgress& slot_progress)
{
  // This seems to be called twice sometimes, so we don't assert here until
  // this is fixed.
  //g_assert(get_self_hosting_active());

  if(!get_self_hosting_active())
    return true; //Don't try to stop it if we have not started it.

  const std::string dbdir_uri = m_database_directory_uri;
  const std::string dbdir = Glib::filename_from_uri(dbdir_uri);
  g_assert(!dbdir.empty());

  const std::string dbdir_data = Glib::build_filename(dbdir, FILENAME_DATA);


  // TODO: Detect other instances on the same computer, and use a different port number,
  // or refuse to continue, showing an error dialog.

  // -D specifies the data directory.
  // -c config_file= specifies the configuration file
  // -k specifies a directory to use for the socket. This must be writable by us.
  // We use "-m fast" instead of the default "-m smart" because that waits for clients to disconnect (and sometimes never succeeds).
  // TODO: Warn about connected clients on other computers? Warn those other users?
  // Make sure to use double quotes for the executable path, because the
  // CreateProcess() API used on Windows does not support single quotes.
  const std::string command_postgres_stop = "\"" + get_path_to_postgres_executable("pg_ctl") + "\" -D \"" + dbdir_data + "\" stop -m fast";
  const bool result = Glom::Spawn::execute_command_line_and_wait(command_postgres_stop, slot_progress);
  if(!result)
  {
    std::cerr << "Error while attempting to stop self-hosting of the database. Trying again."  << std::endl;
    
    //Show open connections for debugging:
    try
    {
      show_active_connections();
    }
    catch(const Glib::Error& ex)
    {
      std::cerr << G_STRFUNC << ": exception while trying to show active connections: " << ex.what() << std::endl;
    }
    
    //I've seen it fail when running under valgrind, and there are reports of failures in bug #420962.
    //Maybe it will help to try again:
    const bool result = Glom::Spawn::execute_command_line_and_wait(command_postgres_stop, slot_progress);
    if(!result)
    {
      std::cerr << "Error while attempting (for a second time) to stop self-hosting of the database."  << std::endl;
      return false;
    }
  }

  m_port = 0;

  return true;
}



bool PostgresSelfHosted::set_network_shared(const SlotProgress& slot_progress, bool network_shared)
{
  //TODO: Use slot_progress, while doing async IO for create_text_file().

  m_network_shared = network_shared;

  const std::string dbdir_uri = m_database_directory_uri;
  const std::string dbdir = Glib::filename_from_uri(dbdir_uri);

  const std::string dbdir_uri_config = dbdir_uri + "/config";
  const char* default_conf_contents = 0;

  // Choose the configuration contents based on the postgresql version
  // and whether we want to be network-shared:
  const float postgresql_version = get_postgresql_utils_version_as_number(slot_progress);
  //std::cout << "DEBUG: postgresql_version=" << postgresql_version << std::endl;

  if(postgresql_version >= 8.4f)
    default_conf_contents = m_network_shared ? DEFAULT_CONFIG_PG_HBA_REMOTE_8p4 : DEFAULT_CONFIG_PG_HBA_LOCAL_8p4;
  else
    default_conf_contents = m_network_shared ? DEFAULT_CONFIG_PG_HBA_REMOTE_8p3 : DEFAULT_CONFIG_PG_HBA_LOCAL_8p3;

  //std::cout << "DEBUG: default_conf_contents=" << default_conf_contents << std::endl;

  const bool hba_conf_creation_succeeded = create_text_file(dbdir_uri_config + "/pg_hba.conf", default_conf_contents);
  g_assert(hba_conf_creation_succeeded);
  if(!hba_conf_creation_succeeded)
    return false;

  const bool ident_conf_creation_succeeded = create_text_file(dbdir_uri_config + "/pg_ident.conf", DEFAULT_CONFIG_PG_IDENT);
  g_assert(ident_conf_creation_succeeded);

  return hba_conf_creation_succeeded;
}

static bool on_timeout_delay(const Glib::RefPtr<Glib::MainLoop>& mainloop)
{
  //Allow our mainloop.run() to return:
  if(mainloop)
    mainloop->quit();

  return false;
}


Glib::RefPtr<Gnome::Gda::Connection> PostgresSelfHosted::connect(const Glib::ustring& database, const Glib::ustring& username, const Glib::ustring& password)
{
  if(!get_self_hosting_active())
  {
    throw ExceptionConnection(ExceptionConnection::FAILURE_NO_BACKEND); //TODO: But there is a backend. It's just not ready.
    return Glib::RefPtr<Gnome::Gda::Connection>();
  }

  Glib::RefPtr<Gnome::Gda::Connection> result;
  bool keep_trying = true;
  guint count_retries = 0;
  const guint MAX_RETRIES_KNOWN_PASSWORD = 30; /* seconds */
  const guint MAX_RETRIES_EVER = 60; /* seconds */
  while(keep_trying)
  {
    try
    {
      result = attempt_connect(port_as_string(m_port), database, username, password);
    }
    catch(const ExceptionConnection& ex)
    {
      if(ex.get_failure_type() == ExceptionConnection::FAILURE_NO_SERVER)
      {
        //It must be using a default password, so any failure would not be due to a wrong password.
        //However, pg_ctl sometimes reports success before it is really ready to let us connect,
        //so in this case we can just keep trying until it works, with a very long timeout.
        count_retries++;
        const guint max_retries = m_network_shared ? MAX_RETRIES_EVER : MAX_RETRIES_KNOWN_PASSWORD;
        if(count_retries > max_retries)
        {
          keep_trying = false;
          continue;
        }

        std::cout << "debug: " << G_STRFUNC << ": Waiting and retrying the connection due to suspected too-early success of pg_ctl. retries=" << count_retries << ", max_retries=" << m_network_shared << std::endl;

        //Wait:
        Glib::RefPtr<Glib::MainLoop> mainloop = Glib::MainLoop::create(false);
          sigc::connection connection_timeout = Glib::signal_timeout().connect(
          sigc::bind(sigc::ptr_fun(&on_timeout_delay), sigc::ref(mainloop)),
          1000 /* 1 second */);
        mainloop->run();
        connection_timeout.disconnect();

        keep_trying = true;
        continue;
      }
      else
      {
        throw ex;
      }
    }

    keep_trying = false;
  }

  //Save the connection details _only_ for later debug use:
  
  m_saved_database_name = database;
  m_saved_username = username;
  m_saved_password = password;
  return result;
}

bool PostgresSelfHosted::create_database(const Glib::ustring& database_name, const Glib::ustring& username, const Glib::ustring& password)
{
  return attempt_create_database(database_name, "localhost", port_as_string(m_port), username, password);
}

unsigned int PostgresSelfHosted::discover_first_free_port(unsigned int start_port, unsigned int end_port)
{
  //Open a socket so we can try to bind it to a port:
  const int fd = socket(AF_INET, SOCK_STREAM, 0);
  if(fd == -1)
  {
#ifdef G_OS_WIN32
    std::cerr << "Create socket: " << WSAGetLastError() << std::endl;
#else
    perror("Create socket");
#endif //G_OS_WIN32
    return 0;
  }

  //This code was originally suggested by Lennart Poettering.

  struct ::sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;

  guint16 port_to_try = start_port;
  while (port_to_try <= end_port)
  {
    sa.sin_port = htons(port_to_try);

    const int result = bind(fd, (sockaddr*)&sa, sizeof(sa));
    bool available = false;
    if(result == 0)
       available = true;
    else if(result < 0)
    {
      #ifdef G_OS_WIN32
      available = (WSAGetLastError() != WSAEADDRINUSE);
      #endif // G_OS_WIN32

      //Some BSDs don't have this.
      //But watch out - if you don't include errno.h then this won't be
      //defined on Linux either, but you really do need to check for it.
      #ifdef EADDRINUSE
      available = (errno != EADDRINUSE);
      #endif

      #ifdef EPORTINUSE //Linux doesn't have this.
      available = (errno != EPORTINUSE);
      #endif
    }
    else
    {
      //std::cout << "debug: " << G_STRFUNC << ": port in use: " << port_to_try << std::endl;
    }

    if(available)
    {
      #ifdef G_OS_WIN32
      closesocket(fd);
      #else
      close(fd);
      #endif //G_OS_WIN32

      //std::cout << "debug: " << G_STRFUNC << ": Found: returning " << port_to_try << std::endl;
      return port_to_try;
    }

    ++port_to_try;
  }

#ifdef G_OS_WIN32
  closesocket(fd);
#else
  close(fd);
#endif //G_OS_WIN32

  std::cerr << G_STRFUNC << ": No port was available." << std::endl;
  return 0;
}

} // namespace ConnectionPoolBackends

} // namespcae Glom
