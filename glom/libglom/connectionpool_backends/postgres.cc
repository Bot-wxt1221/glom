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

#include <libglom/libglom_config.h>

#include <libglom/connectionpool_backends/postgres.h>
#include <libglom/utils.h>
#include <glibmm/i18n.h>

// Uncomment to see debug messages
//#define GLOM_CONNECTION_DEBUG

namespace
{

static Glib::ustring create_auth_string(const Glib::ustring& username, const Glib::ustring& password)
{
  if(username.empty() and password.empty())
    return Glib::ustring();
  else
    return "USERNAME=" + username + ";PASSWORD=" + password; //TODO: How should we quote and escape these?
}

} //anonymous namespace

namespace Glom
{

namespace ConnectionPoolBackends
{

Postgres::Postgres()
:
  m_postgres_server_version(0.0f)
{
}

float Postgres::get_postgres_server_version() const
{
  return m_postgres_server_version;
}

Glib::RefPtr<Gnome::Gda::Connection> Postgres::attempt_connect(const Glib::ustring& host, const Glib::ustring& port, const Glib::ustring& database, const Glib::ustring& username, const Glib::ustring& password, std::auto_ptr<ExceptionConnection>& error)
{
  //We must specify _some_ database even when we just want to create a database.
  //This _might_ be different on some systems. I hope not. murrayc
  const Glib::ustring default_database = "template1";
  //const Glib::ustring& actual_database = (!database.empty()) ? database : default_database;;
  const Glib::ustring cnc_string_main = "HOST=" + host + ";PORT=" + port;
  Glib::ustring cnc_string = cnc_string_main + ";DB_NAME=" + database;

  //std::cout << "debug: connecting: cnc string: " << cnc_string << std::endl;
#ifdef GLOM_CONNECTION_DEBUG          
  std::cout << std::endl << "Glom: trying to connect on port=" << port << std::endl;
#endif

  Glib::RefPtr<Gnome::Gda::Connection> connection;
  Glib::RefPtr<Gnome::Gda::DataModel> data_model;

  const Glib::ustring auth_string = create_auth_string(username, password);   
 
#ifdef GLOM_CONNECTION_DEBUG
  std::cout << "DEBUG: ConnectionPoolBackends::Postgres::attempt_connect(): cnc_string=" << cnc_string << std::endl;
  std::cout << "  DEBUG: auth_string=" << auth_string << std::endl;
#endif

//TODO: Allow the client-only build to specify a read-only connection, 
//so we can use Gnome::Gda::CONNECTION_OPTIONS_READ_ONLY?
//But this must be a runtime thing - it can't be a build-time change, 
//because libglom has no client-only build. It always offers everything.
#ifdef GLIBMM_EXCEPTIONS_ENABLED
  try
  {
    connection = Gnome::Gda::Connection::open_from_string("PostgreSQL", 
      cnc_string, auth_string, 
      Gnome::Gda::CONNECTION_OPTIONS_SQL_IDENTIFIERS_CASE_SENSITIVE
      );
    connection->statement_execute_non_select("SET DATESTYLE = 'ISO'");
    data_model = connection->statement_execute_select("SELECT version()");
  }
  catch(const Glib::Error& ex)
  {
#else
  std::auto_ptr<Glib::Error> ex;
  connection = Gnome::Gda::Connection::open_from_string("PostgreSQL", 
    cnc_string, auth_string,
    Gnome::Gda::CONNECTION_OPTIONS_SQL_IDENTIFIERS_CASE_SENSITIVE,
    ex);
  
  if(!ex.get())
    connection->statement_execute_non_select("SET DATESTYLE = 'ISO'", ex);

  if(!ex.get())
    data_model = connection->statement_execute_select("SELECT version()", Gnome::Gda::STATEMENT_MODEL_RANDOM_ACCESS, ex);

  if(!ex.get())
  {
#endif

#ifdef GLOM_CONNECTION_DEBUG
    std::cout << "ConnectionPoolBackends::Postgres::attempt_connect(): Attempt to connect to database failed on port=" << port << ", database=" << database << ": " << ex.what() << std::endl;
    std::cout << "ConnectionPoolBackends::Postgres::attempt_connect(): Attempting to connect without specifying the database." << std::endl;
#endif

    Glib::ustring cnc_string = cnc_string_main + ";DB_NAME=" + default_database;
    Glib::RefPtr<Gnome::Gda::Connection> temp_conn;
    Glib::ustring auth_string = create_auth_string(username, password);
#ifdef GLIBMM_EXCEPTIONS_ENABLED
    try
    {
      temp_conn = Gnome::Gda::Connection::open_from_string("PostgreSQL", 
        cnc_string, auth_string, 
        Gnome::Gda::CONNECTION_OPTIONS_SQL_IDENTIFIERS_CASE_SENSITIVE);
    }
    catch(const Glib::Error& ex)
    {}
#else
    temp_conn = Gnome::Gda::Connection::open_from_string("PostgreSQL", 
      cnc_string, auth_string, 
      Gnome::Gda::CONNECTION_OPTIONS_SQL_IDENTIFIERS_CASE_SENSITIVE, ex);
#endif

#ifdef GLOM_CONNECTION_DEBUG
    if(temp_conn)
      std::cout << "  (Connection succeeds, but not to the specific database,  database=" << database << std::endl;
    else
      std::cerr << "  (Could not connect even to the default database, database=" << database  << std::endl;
#endif

    error.reset(new ExceptionConnection(temp_conn ? ExceptionConnection::FAILURE_NO_DATABASE : ExceptionConnection::FAILURE_NO_SERVER));
    return Glib::RefPtr<Gnome::Gda::Connection>();
  }

  if(data_model && data_model->get_n_rows() && data_model->get_n_columns())
  {
#ifdef GLIBMM_EXCEPTIONS_ENABLED
    Gnome::Gda::Value value = data_model->get_value_at(0, 0);
#else
    Gnome::Gda::Value value = data_model->get_value_at(0, 0, ex);
#endif
    if(value.get_value_type() == G_TYPE_STRING)
    {
      const Glib::ustring version_text = value.get_string();
      //This seems to have the format "PostgreSQL 7.4.11 on i486-pc-linux"
      const Glib::ustring namePart = "PostgreSQL ";
      const Glib::ustring::size_type posName = version_text.find(namePart);
      if(posName != Glib::ustring::npos)
      {
        const Glib::ustring versionPart = version_text.substr(namePart.size());
        m_postgres_server_version = strtof(versionPart.c_str(), NULL);

#ifdef GLOM_CONNECTION_DEBUG
        std::cout << "  Postgres Server version: " << m_postgres_server_version << std::endl;
#endif
      }
    }
  }

  return connection;
}

bool Postgres::change_columns(const Glib::RefPtr<Gnome::Gda::Connection>& connection, const Glib::ustring& table_name, const type_vec_const_fields& old_fields, const type_vec_const_fields& new_fields, std::auto_ptr<Glib::Error>& error)
{
  static const char* TRANSACTION_NAME = "glom_change_columns_transaction";
  static const gchar* TEMP_COLUMN_NAME = "glom_temp_column"; // TODO: Find a unique name.

  if(!begin_transaction(connection, TRANSACTION_NAME, Gnome::Gda::TRANSACTION_ISOLATION_UNKNOWN, error)) return false; // TODO: What does the transaction isolation do?

  for(unsigned int i = 0; i < old_fields.size(); ++ i)
  {
    // If the type did change, then we need to recreate the column. See
    // http://www.postgresql.org/docs/faqs.FAQ.html#item4.3
    if(old_fields[i]->get_field_info()->get_g_type() != new_fields[i]->get_field_info()->get_g_type())
    {
      // Create a temporary column
      sharedptr<Field> temp_field = glom_sharedptr_clone(new_fields[i]);
      temp_field->set_name(TEMP_COLUMN_NAME);
      // The temporary column must not be primary key as long as the original
      // (primary key) column is still present, because there cannot be two
      // primary key columns.
      temp_field->set_primary_key(false);

      if(!add_column(connection, table_name, temp_field, error)) break;

      Glib::ustring conversion_command;
      const Glib::ustring field_name_old_quoted = "\"" + old_fields[i]->get_name() + "\"";
      const Field::glom_field_type old_field_type = old_fields[i]->get_glom_type();

      if(Field::get_conversion_possible(old_fields[i]->get_glom_type(), new_fields[i]->get_glom_type()))
      {
        //TODO: postgres seems to give an error if the data cannot be converted (for instance if the text is not a numeric digit when converting to numeric) instead of using 0.
        /*
        Maybe, for instance:
        http://groups.google.de/groups?hl=en&lr=&ie=UTF-8&frame=right&th=a7a62337ad5a8f13&seekm=23739.1073660245%40sss.pgh.pa.us#link5
        UPDATE _table
        SET _bbb = to_number(substring(_aaa from 1 for 5), '99999')
        WHERE _aaa <> '     ';  
        */

        switch(new_fields[i]->get_glom_type())
        {
          case Field::TYPE_BOOLEAN:
          {
            if(old_field_type == Field::TYPE_NUMERIC)
            {
              conversion_command = "(CASE WHEN " + field_name_old_quoted + " > 0 THEN true "
                                         "WHEN " + field_name_old_quoted + " = 0 THEN false "
                                         "WHEN " + field_name_old_quoted + " IS NULL THEN false END)";
            }
            else if(old_field_type == Field::TYPE_TEXT)
              conversion_command = "(" + field_name_old_quoted + " !~~* \'false\')"; // !~~* means ! ILIKE
            else // Dates and Times:
              conversion_command = "(" + field_name_old_quoted + " IS NOT NULL)";
            break;
          }

          case Field::TYPE_NUMERIC: // CAST does not work if the destination type is numeric
          {
            if(old_field_type == Field::TYPE_BOOLEAN)
            {
              conversion_command = "(CASE WHEN " + field_name_old_quoted + " = true THEN 1 "
                                         "WHEN " + field_name_old_quoted + " = false THEN 0 "
                                         "WHEN " + field_name_old_quoted + " IS NULL THEN 0 END)";
            }
            else
            {
              //We use to_number, with textcat() so that to_number always has usable data.
              //Otherwise, it says 
              //invalid input syntax for type numeric: " "
              //
              //We must use single quotes with the 0, otherwise it says "column 0 does not exist.".
              conversion_command = "to_number( textcat(\'0\', " + field_name_old_quoted + "), '999999999.99999999' )";
            }

            break;
          }

          case Field::TYPE_DATE: // CAST does not work if the destination type is date.
          {
            conversion_command = "to_date( " + field_name_old_quoted + ", 'YYYYMMDD' )"; // TODO: Standardise date storage format.
            break;
          }
          case Field::TYPE_TIME: // CAST does not work if the destination type is timestamp.
          {
            conversion_command = "to_timestamp( " + field_name_old_quoted + ", 'HHMMSS' )"; // TODO: Standardise time storage format.
            break;
          }

          default:
          {
            // To Text:

            // bool to text:
            if(old_field_type == Field::TYPE_BOOLEAN)
            {
              conversion_command = "(CASE WHEN " + field_name_old_quoted + " = true THEN \'true\' "
                                         "WHEN " + field_name_old_quoted + " = false THEN \'false\' "
                                         "WHEN " + field_name_old_quoted + " IS NULL THEN \'false\' END)";
            }
            else
            {
              // This works for most to-text conversions:
              conversion_command = "CAST(" + field_name_old_quoted + " AS " + new_fields[i]->get_sql_type() + ")";
            }

            break;
          }
        }

        if(!query_execute(connection, "UPDATE \"" + table_name + "\" SET \"" + TEMP_COLUMN_NAME + "\" = " + conversion_command, error))
          break;
      }
      else
      {
        // The conversion is not possible, so drop data in that column
      }

      if(!drop_column(connection, table_name, old_fields[i]->get_name(), error))
        break;

      if(!query_execute(connection, "ALTER TABLE \"" + table_name + "\" RENAME COLUMN \"" + TEMP_COLUMN_NAME + "\" TO \"" + new_fields[i]->get_name() + "\"", error))
        break;

      // Readd primary key constraint
      if(new_fields[i]->get_primary_key())
      {
        if(!query_execute(connection, "ALTER TABLE \"" + table_name + "\" ADD PRIMARY KEY (\"" + new_fields[i]->get_name() + "\")", error))
          break;
      }
    }
    else
    {
      // The type did not change. What could have changed: The field being a
      // unique key, primary key, its name or its default value.

      // Primary key
      // TODO: Test whether this is able to remove unique key constraints
      // added via libgda's DDL API in add_column(). Maybe override
      // add_column() if we can't.
      bool primary_key_was_set = false;
      bool primary_key_was_unset = false;
      if(old_fields[i]->get_primary_key() != new_fields[i]->get_primary_key())
      {
        if(new_fields[i]->get_primary_key())
        {
          primary_key_was_set = true;

          // Primary key was added
          if(!query_execute(connection, "ALTER TABLE \"" + table_name + "\" ADD PRIMARY KEY (\"" + old_fields[i]->get_name() + "\")", error))
            break;

          // Remove unique key constraint, because this is already implied in
          // the field being primary key.
          if(old_fields[i]->get_unique_key())
            if(!query_execute(connection, "ALTER TABLE \"" + table_name + "\" DROP CONSTRAINT \"" + old_fields[i]->get_name() + "_key", error))
              break;
        }
        else
        {
          primary_key_was_unset = true;

          // Primary key was removed
          if(!query_execute(connection, "ALTER TABLE \"" + table_name + "\" DROP CONSTRAINT \"" + table_name + "_pkey\"", error))
            break;
        }
      }

      // Uniqueness
      if(old_fields[i]->get_unique_key() != new_fields[i]->get_unique_key())
      {
        // Postgres automatically makes primary keys unique, so we do not need
        // to do that separately if we already made it a primary key
        if(!primary_key_was_set && new_fields[i]->get_unique_key())
        {
          if(!query_execute(connection, "ALTER TABLE \"" + table_name + "\" ADD CONSTRAINT \"" + old_fields[i]->get_name() + "_key\" UNIQUE (\"" + old_fields[i]->get_name() + "\")", error))
            break;
        }
        else if(!primary_key_was_unset && !new_fields[i]->get_unique_key() && !new_fields[i]->get_primary_key())
        {
          if(!query_execute(connection, "ALTER TABLE \"" + table_name + "\" DROP CONSTRAINT \"" + old_fields[i]->get_name() + "_key\"", error))
            break;
        }
      }

      if(!new_fields[i]->get_auto_increment()) // Auto-increment fields have special code as their default values.
      {
        if(old_fields[i]->get_default_value() != new_fields[i]->get_default_value())
        {
          if(!query_execute(connection, "ALTER TABLE \"" + table_name + "\" ALTER COLUMN \"" + old_fields[i]->get_name() + "\" SET DEFAULT " + new_fields[i]->sql(new_fields[i]->get_default_value(), connection), error))
            break;
        }
      }

      if(old_fields[i]->get_name() != new_fields[i]->get_name())
      {
        if(!query_execute(connection, "ALTER TABLE \"" + table_name + "\" RENAME COLUMN \"" + old_fields[i]->get_name() + "\" TO \"" + new_fields[i]->get_name() + "\"", error))
          break;
      }
    }
  }

  if(error.get() || !commit_transaction(connection, TRANSACTION_NAME, error))
  {
    std::auto_ptr<Glib::Error> rollback_error;
    rollback_transaction(connection, TRANSACTION_NAME, rollback_error);
    return false;
  }

  return true;
}

bool Postgres::attempt_create_database(const Glib::ustring& database_name, const Glib::ustring& host, const Glib::ustring& port, const Glib::ustring& username, const Glib::ustring& password, std::auto_ptr<Glib::Error>& error)
{
  Glib::RefPtr<Gnome::Gda::ServerOperation> op;
#ifdef GLIBMM_EXCEPTIONS_ENABLED
  try
  {
    op = Gnome::Gda::ServerOperation::prepare_create_database("PostgreSQL",
                                                              database_name);
  }
  catch(const Glib::Error& ex)
  {
    error.reset(new Glib::Error(ex));
    return false;
  }
#else
  std::auto_ptr<Glib::Error> ex;
  op = Gnome::Gda::ServerOperation::prepare_create_database("PostgreSQL",
                                                            database_name,
                                                            ex);
  if(ex.get())
    return false;

  //TODO: Why is this here but not in the EXCEPTIONS_ENABLED part?
  // jhs: Does look like a bug to me, shouldn't be there
#if 0
  op = cnc->create_operation(Gnome::Gda::SERVER_OPERATION_CREATE_DB, set, ex.get());
  if(ex.get())
    return false;
#endif
#endif
  g_assert(op);
#ifdef GLIBMM_EXCEPTIONS_ENABLED
  try
  {
    op->set_value_at("/SERVER_CNX_P/HOST", host);
    op->set_value_at("/SERVER_CNX_P/PORT", port);
    op->set_value_at("/SERVER_CNX_P/ADM_LOGIN", username);
    op->set_value_at("/SERVER_CNX_P/ADM_PASSWORD", password);
    op->perform_create_database("PostgreSQL");
  }
  catch(const Glib::Error& ex)
  {
    error.reset(new Glib::Error(ex));
    return false;
  }
#else //GLIBMM_EXCEPTIONS_ENABLED
  op->set_value_at("/SERVER_CNX_P/HOST", host, error);
  op->set_value_at("/SERVER_CNX_P/PORT", port, error);
  op->set_value_at("/SERVER_CNX_P/ADM_LOGIN", username, error);
  op->set_value_at("/SERVER_CNX_P/ADM_PASSWORD", password, error);

  if(error.get() == 0)
    op->perform_create_database("PostgreSQL", ex);
  else
    return false;
  if (error.get())
    return false;
#endif //GLIBMM_EXCEPTIONS_ENABLED

  return true;
}

bool Postgres::check_postgres_gda_client_is_available()
{
  //This API is horrible.
  //See libgda bug http://bugzilla.gnome.org/show_bug.cgi?id=575754
  Glib::RefPtr<Gnome::Gda::DataModel> model = Gnome::Gda::Config::list_providers();
  if(model && model->get_n_columns() && model->get_n_rows())
  {
    Glib::RefPtr<Gnome::Gda::DataModelIter> iter = model->create_iter();

    do
    {
      //See http://library.gnome.org/devel/libgda/unstable/libgda-40-Configuration.html#gda-config-list-providers
      //about the columns of this DataModel:
      const Gnome::Gda::Value name = iter->get_value_at(0);
      if(name.get_value_type() != G_TYPE_STRING)
        continue;

      const Glib::ustring name_as_string = name.get_string();
      //std::cout << "DEBUG: Provider name:" << name_as_string << std::endl;
      if(name_as_string == "PostgreSQL")
        return true;
    }
    while(iter->move_next());
  }

  return false;
}

}

}
