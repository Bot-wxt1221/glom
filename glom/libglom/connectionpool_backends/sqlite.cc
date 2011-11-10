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
#include <libglom/connectionpool_backends/sqlite.h>
#include <libglom/utils.h>
#include <libglom/db_utils.h>
#include <giomm/file.h>
#include <libgdamm/metastore.h>

#include <iostream>

namespace Glom
{

namespace ConnectionPoolBackends
{

Sqlite::Sqlite()
{
}

Glib::RefPtr<Gnome::Gda::Connection> Sqlite::connect(const Glib::ustring& database, const Glib::ustring& username, const Glib::ustring& password)
{
  Glib::RefPtr<Gnome::Gda::Connection> connection;
  if(m_database_directory_uri.empty())
    return connection;

  // Check if the database file exists. If it does not, then we don't try to
  // connect. libgda would create the database file if necessary, but we need
  // to ensure slightly different semantics.
  Glib::RefPtr<Gio::File> db_dir = Gio::File::create_for_uri(m_database_directory_uri);
  Glib::RefPtr<Gio::File> db_file = db_dir->get_child(database + ".db");

  const bool file_exists = Glom::Utils::file_exists(db_file);
  if(!file_exists)
  {
    std::cerr << G_STRFUNC << ": The db file does not exist at path: " << db_file->get_uri() << std::endl;
    //std::cerr << "  as filepath: " << db_file->get_path() << std::endl;
  }
  else
  {
    if(db_file->query_file_type() != Gio::FILE_TYPE_REGULAR)
    {
      std::cerr << G_STRFUNC << ": The db file is not a regular file at path: " << db_file->get_uri() << std::endl;
    }
    else
    {
      // Convert URI to path, for GDA connection string
      const std::string database_directory = db_dir->get_path();

      const Glib::ustring cnc_string = "DB_DIR=" + DbUtils::gda_cnc_string_encode(database_directory) + 
        ";DB_NAME=" + DbUtils::gda_cnc_string_encode(database);
      const Glib::ustring auth_string = Glib::ustring::compose("USERNAME=%1;PASSWORD=%2", 
        DbUtils::gda_cnc_string_encode(username), DbUtils::gda_cnc_string_encode(password));

      connection = Gnome::Gda::Connection::open_from_string("SQLite",
        cnc_string, auth_string,
        Gnome::Gda::CONNECTION_OPTIONS_SQL_IDENTIFIERS_CASE_SENSITIVE);
    }
  }

  if(!connection)
  {
    // If the database directory is valid, then only the database (file) is
    // missing, otherwise we pretend the "server" is not running.
    if(Glom::Utils::file_exists(db_dir) &&
      (db_dir->query_file_type() == Gio::FILE_TYPE_DIRECTORY))
    {
      throw ExceptionConnection(ExceptionConnection::FAILURE_NO_DATABASE);
    }
    else
      throw ExceptionConnection(ExceptionConnection::FAILURE_NO_SERVER);
  }

  return connection;
}

bool Sqlite::create_database(const Glib::ustring& database_name, const Glib::ustring& /* username */, const Glib::ustring& /* password */)
{
  if(m_database_directory_uri.empty())
  {
    std::cerr << G_STRFUNC << ": m_database_directory_uri was empty." << std::endl;
    return false;
  }
  
  Glib::RefPtr<Gio::File> file = Gio::File::create_for_uri(m_database_directory_uri);
  const std::string database_directory = file->get_path();
  const Glib::ustring cnc_string = Glib::ustring::compose("DB_DIR=%1;DB_NAME=%2", 
    DbUtils::gda_cnc_string_encode(database_directory), DbUtils::gda_cnc_string_encode(database_name));

  Glib::RefPtr<Gnome::Gda::Connection> cnc =
    Gnome::Gda::Connection::open_from_string("SQLite",
      cnc_string, "",
      Gnome::Gda::CONNECTION_OPTIONS_SQL_IDENTIFIERS_CASE_SENSITIVE);

  return true;
}


bool Sqlite::add_column_to_server_operation(const Glib::RefPtr<Gnome::Gda::ServerOperation>& operation, GdaMetaTableColumn* column, unsigned int i)
{
  //TODO: Quote column name?
  const Glib::ustring name_path = Glib::ustring::compose("/FIELDS_A/@COLUMN_NAME/%1", i);
  const Glib::ustring type_path = Glib::ustring::compose("/FIELDS_A/@COLUMN_TYPE/%1", i);
  const Glib::ustring pkey_path = Glib::ustring::compose("/FIELDS_A/@COLUMN_PKEY/%1", i);
  const Glib::ustring nnul_path = Glib::ustring::compose("/FIELDS_A/@COLUMN_NNUL/%1", i);
  // TODO: Find out whether the column is unique.
  const Glib::ustring default_path = Glib::ustring::compose("/FIELDS_A/@COLUMN_DEFAULT/%1", i);

  operation->set_value_at(name_path, column->column_name);
  operation->set_value_at(type_path, column->column_type);
  operation->set_value_at(pkey_path, column->pkey);
  operation->set_value_at(nnul_path, !column->nullok);

  if(column->default_value)
  {
    operation->set_value_at(default_path, column->default_value);
  }

  return true;
}

bool Sqlite::add_column_to_server_operation(const Glib::RefPtr<Gnome::Gda::ServerOperation>& operation, const sharedptr<const Field>& column, unsigned int i)
{
  //TODO: Quote column name?
  const Glib::ustring name_path = Glib::ustring::compose("/FIELDS_A/@COLUMN_NAME/%1", i);
  const Glib::ustring type_path = Glib::ustring::compose("/FIELDS_A/@COLUMN_TYPE/%1", i);
  const Glib::ustring pkey_path = Glib::ustring::compose("/FIELDS_A/@COLUMN_PKEY/%1", i);
  const Glib::ustring unique_path = Glib::ustring::compose("/FIELDS_A/@COLUMN_UNIQUE/%1", i);
  const Glib::ustring default_path = Glib::ustring::compose("/FIELDS_A/@COLUMN_DEFAULT/%1", i);

  operation->set_value_at(name_path, column->get_name());
  operation->set_value_at(type_path, column->get_sql_type());
  operation->set_value_at(pkey_path, column->get_primary_key());
  operation->set_value_at(unique_path, column->get_unique_key());

  return true;
}

bool Sqlite::recreate_table(const Glib::RefPtr<Gnome::Gda::Connection>& connection, const Glib::ustring& table_name, const type_vec_strings& fields_removed, const type_vec_const_fields& fields_added, const type_mapFieldChanges& fields_changed) throw()
{
  static const gchar TEMPORARY_TABLE_NAME[] = "GLOM_TEMP_TABLE"; // TODO: Make sure this is unique.
  static const gchar TRANSACTION_NAME[] = "GLOM_RECREATE_TABLE_TRANSACTION";

  Glib::RefPtr<Gnome::Gda::MetaStore> store = connection->get_meta_store();
  Glib::RefPtr<Gnome::Gda::MetaStruct> metastruct = Gnome::Gda::MetaStruct::create(store, Gnome::Gda::META_STRUCT_FEATURE_NONE);

  GdaMetaDbObject* object = metastruct->complement(Gnome::Gda::META_DB_TABLE, Gnome::Gda::Value(), Gnome::Gda::Value(), Gnome::Gda::Value(table_name));
  if(!object)
    return false;

  Glib::RefPtr<Gnome::Gda::ServerOperation> operation = connection->get_provider()->create_operation(connection, Gnome::Gda::SERVER_OPERATION_CREATE_TABLE);
  if(!operation)
    return false;

  //TODO: Quote table name?
  operation->set_value_at("/TABLE_DEF_P/TABLE_NAME", TEMPORARY_TABLE_NAME);

  GdaMetaTable* table = GDA_META_TABLE(object);
  unsigned int i = 0;

  Glib::ustring trans_fields;

  for(GSList* item = table->columns; item != 0; item = item->next)
  {
    GdaMetaTableColumn* column = GDA_META_TABLE_COLUMN(item->data);

    // Don't add if field was removed
    if(std::find(fields_removed.begin(), fields_removed.end(), column->column_name) != fields_removed.end())
      continue;
#if 0
    {
      // If it was removed, and added again, then it has changed, so use the
      // definition from the added_fields vector.
      type_vec_fields::const_iterator iter = std::find_if(fields_added.begin(), fields_added.end(), predicate_FieldHasName<Field>(column->column_name));
      if(iter == fields_added.end())
        continue;
      else
        added = *iter;
    }
#endif

    if(!trans_fields.empty())
      trans_fields += ',';

    const type_mapFieldChanges::const_iterator changed_iter = fields_changed.find(column->column_name);
    if(changed_iter != fields_changed.end())
    {
      // Convert values to date or time, accordingly.
      switch(changed_iter->second->get_glom_type())
      {
      case Field::TYPE_TEXT:
        if(column->gtype == G_TYPE_BOOLEAN)
	  trans_fields += "(CASE WHEN "+ DbUtils::escape_sql_id(column->column_name) + " = 1 THEN 'true' "
                                              "WHEN " + DbUtils::escape_sql_id(column->column_name) + " = 0 THEN 'false' "
                                              "WHEN " + DbUtils::escape_sql_id(column->column_name) + " IS NULL THEN 'false' END)";
	else if(column->gtype == GDA_TYPE_BLOB)
	  trans_fields += "''";
	else
          // Make sure we don't insert NULL strings, as we ignore that concept in Glom.
          trans_fields += "(CASE WHEN "+ DbUtils::escape_sql_id(column->column_name) + " IS NULL THEN '' "
                                              "WHEN " + DbUtils::escape_sql_id(column->column_name) + " IS NOT NULL THEN " + DbUtils::escape_sql_id(column->column_name) + " END)";
	break;
      case Field::TYPE_NUMERIC:
        if(column->gtype == G_TYPE_BOOLEAN)
          trans_fields += "(CASE WHEN "+ DbUtils::escape_sql_id(column->column_name) + " = 0 THEN 0 "
                                              "WHEN " + DbUtils::escape_sql_id(column->column_name) + " != 0 THEN 1 "
                                              "WHEN " + DbUtils::escape_sql_id(column->column_name) + " IS NULL THEN 0 END)";
        else if(column->gtype == GDA_TYPE_BLOB || column->gtype == G_TYPE_DATE || column->gtype == GDA_TYPE_TIME)
          trans_fields += '0';
        else
          trans_fields += Glib::ustring("CAST(")+ DbUtils::escape_sql_id(column->column_name) + " AS real)";
        break;
      case Field::TYPE_BOOLEAN:
        if(column->gtype == G_TYPE_STRING)
          trans_fields += "(CASE WHEN "+ DbUtils::escape_sql_id(column->column_name) + " = 'true' THEN 1 "
                                              "WHEN " + DbUtils::escape_sql_id(column->column_name) + " = 'false' THEN 0 "
                                              "WHEN " + DbUtils::escape_sql_id(column->column_name) + " IS NULL THEN 0 END)";
        else if(column->gtype == G_TYPE_DOUBLE)
          trans_fields += "(CASE WHEN "+ DbUtils::escape_sql_id(column->column_name) + " = 0 THEN 0 "
                                              "WHEN " + DbUtils::escape_sql_id(column->column_name) + " != 0 THEN 1 "
                                              "WHEN " + DbUtils::escape_sql_id(column->column_name) + " IS NULL THEN 0 END)";
        else if(column->gtype == G_TYPE_BOOLEAN)
          trans_fields += column->column_name;
        else
          trans_fields += Glib::ustring(column->column_name) + " IS NOT NULL";
        break;
      case Field::TYPE_DATE:
        if(column->gtype == G_TYPE_BOOLEAN || column->gtype == GDA_TYPE_BLOB || column->gtype == G_TYPE_DOUBLE)
          trans_fields += "NULL";
        else
          trans_fields += Glib::ustring("date(")+ DbUtils::escape_sql_id(column->column_name) + ')';
        break;
      case Field::TYPE_TIME:
        if(column->gtype == G_TYPE_BOOLEAN || column->gtype == GDA_TYPE_BLOB || column->gtype == G_TYPE_DOUBLE)
          trans_fields += "NULL";
        else
          trans_fields += Glib::ustring("time(")+ DbUtils::escape_sql_id(column->column_name) + ')';
        break;
      case Field::TYPE_IMAGE:
        if(column->gtype == GDA_TYPE_BLOB)
          trans_fields += column->column_name;
        else
          trans_fields += "NULL";
        break;
      default:
        trans_fields += column->column_name;
        break;
      };

      add_column_to_server_operation(operation, changed_iter->second, i++);
    }
    else
    {
      trans_fields += column->column_name;
      add_column_to_server_operation(operation, column, i++);
    }
  }

  for(type_vec_const_fields::const_iterator iter = fields_added.begin(); iter != fields_added.end(); ++ iter)
  {
    // Add new fields to the table. Fields that have changed have already
    // been handled above.
    const sharedptr<const Field>& field = *iter;
    type_vec_strings::const_iterator removed_iter = std::find(fields_removed.begin(), fields_removed.end(), field->get_name());
    if(removed_iter == fields_removed.end())
    {
      add_column_to_server_operation(operation, field, i++);

      if(!trans_fields.empty())
        trans_fields += ',';
      const Gnome::Gda::Value default_value = field->get_default_value();
      if(default_value.get_value_type() != G_TYPE_NONE && !default_value.is_null())
        trans_fields += field->sql(default_value, connection);
      else
      {
        switch(field->get_glom_type())
        {
        case Field::TYPE_NUMERIC:
          trans_fields += '0';
          break;
        case Field::TYPE_BOOLEAN:
          trans_fields += '0';
          break;
        case Field::TYPE_TEXT:
          trans_fields += "''";
          break;
        default:
          trans_fields += "NULL";
          break;
        }
      }
    }
  }

  try
  {
    connection->begin_transaction(TRANSACTION_NAME, Gnome::Gda::TRANSACTION_ISOLATION_UNKNOWN);
  }
  catch(const Glib::Error& ex)
  {
    std::cerr << G_STRFUNC << ": Could not begin transaction: exception=" << ex.what() << std::endl;
    return false;
  }

  //Do everything in one big try/catch block,
  //reverting the transaction if anything fail:
  try
  {
    connection->get_provider()->perform_operation(connection, operation);

    if(!trans_fields.empty())
    {
      const Glib::ustring query_insert = "INSERT INTO " + DbUtils::escape_sql_id(TEMPORARY_TABLE_NAME) + " SELECT " + trans_fields + " FROM " + DbUtils::escape_sql_id(table_name);
      //std::cout << "debug: query_insert=" << query_insert << std::endl;
      connection->statement_execute_non_select(query_insert);
      connection->statement_execute_non_select("DROP TABLE " + DbUtils::escape_sql_id(table_name));
      connection->statement_execute_non_select("ALTER TABLE " + DbUtils::escape_sql_id(TEMPORARY_TABLE_NAME) + " RENAME TO " + DbUtils::escape_sql_id(table_name));

      connection->commit_transaction(TRANSACTION_NAME);

      return true;
    }
  }
  catch(const Glib::Error& ex)
  {
    std::cerr << G_STRFUNC << ": exception=" << ex.what() << std::endl;
    std::cerr << G_STRFUNC << ": Reverting the transaction." << std::endl;

    try
    {
      connection->rollback_transaction(TRANSACTION_NAME);
    }
    catch(const Glib::Error& ex)
    {
       std::cerr << G_STRFUNC << ": Could not revert the transaction. exception=" << ex.what() << std::endl;
    }
  }

  return false;
}

bool Sqlite::add_column(const Glib::RefPtr<Gnome::Gda::Connection>& connection, const Glib::ustring& table_name, const sharedptr<const Field>& field)
{
  // Sqlite does not support adding primary key columns. So recreate the table
  // in that case.
  if(!field->get_primary_key())
  {
    return Backend::add_column(connection, table_name, field);
  }
  else
  {
    return recreate_table(connection, table_name, type_vec_strings(), type_vec_const_fields(1, field), type_mapFieldChanges());
  }
}

bool Sqlite::drop_column(const Glib::RefPtr<Gnome::Gda::Connection>& connection, const Glib::ustring& table_name, const Glib::ustring& field_name)
{
  return recreate_table(connection, table_name, type_vec_strings(1, field_name), type_vec_const_fields(), type_mapFieldChanges());
}

bool Sqlite::change_columns(const Glib::RefPtr<Gnome::Gda::Connection>& connection, const Glib::ustring& table_name, const type_vec_const_fields& old_fields, const type_vec_const_fields& new_fields) throw()
{
  type_mapFieldChanges fields_changed;

  for(type_vec_const_fields::size_type i = 0; i < old_fields.size(); ++ i)
    fields_changed[old_fields[i]->get_name()] = new_fields[i];

  return recreate_table(connection, table_name, type_vec_strings(), type_vec_const_fields(), fields_changed);
}

bool Sqlite::save_backup(const SlotProgress& /* slot_progress */, const Glib::ustring& /* username */, const Glib::ustring& /* password */, const Glib::ustring& /* database_name */)
{
  //TODO:
  std::cerr << G_STRFUNC << ": Not implemented.";
  return false;
}

bool Sqlite::convert_backup(const SlotProgress& /* slot_progress */, const std::string& /* base_directory */, const Glib::ustring& /* username */, const Glib::ustring& /* password */, const Glib::ustring& /* database_name */)
{
  //TODO:
  std::cerr << G_STRFUNC << ": Not implemented.";
  return false;
}

Field::sql_format Sqlite::get_sql_format() const
{
  return Field::SQL_FORMAT_SQLITE;
}

bool Sqlite::supports_remote_access() const
{
  return false;
}

Gnome::Gda::SqlOperatorType Sqlite::get_string_find_operator() const
{
  return Gnome::Gda::SQL_OPERATOR_TYPE_LIKE;
}

const char* Sqlite::get_public_schema_name() const
{
  return "main";
}

} // namespace ConnectionPoolBackends

} // namespace Glom
