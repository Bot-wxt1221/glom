/* Glom
 *
 * Copyright (C) 2001-2010 Murray Cumming
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

#include <libglom/db_utils.h>
#include <libglom/connectionpool.h>
#include <libglom/data_structure/glomconversions.h>
#include <libglom/connectionpool_backends/postgres_central.h>
#include <libglom/standard_table_prefs_fields.h>
#include <libglom/privs.h>
#include <libglom/data_structure/parameternamegenerator.h>
#include <libglom/utils.h>
#include <libgdamm/value.h>
#include <libgdamm/metastore.h>
#include <glibmm/timer.h>
#include <libgda/libgda.h> // For gda_g_type_from_string
#include <glibmm/i18n.h>


#include <iostream>

namespace Glom
{

namespace DbUtils
{

static Glib::RefPtr<Gnome::Gda::Connection> get_connection()
{
  sharedptr<SharedConnection> sharedconnection;
  try
  {
     sharedconnection = ConnectionPool::get_and_connect();
  }
  catch (const Glib::Error& error)
  {
    std::cerr << G_STRFUNC << ": " << error.what() << std::endl;
  }

  if(!sharedconnection)
  {
    std::cerr << G_STRFUNC << ": No connection yet." << std::endl;
    return Glib::RefPtr<Gnome::Gda::Connection>(0);
  }

  Glib::RefPtr<Gnome::Gda::Connection> gda_connection = sharedconnection->get_gda_connection();

  return gda_connection;
}

/** Update GDA's information about the table structure, such as the
  * field list and their types.
  * Call this whenever changing the table structure, for instance with an ALTER query.
  * This may take a few seconds to return.
  */
static void update_gda_metastore_for_table(const Glib::ustring& table_name)
{
  Glib::RefPtr<Gnome::Gda::Connection> gda_connection = get_connection();
  if(!gda_connection)
  {
    std::cerr << G_STRFUNC << ": No gda_connection." << std::endl;
    return;
  }

  if(table_name.empty())
  {
    std::cerr << G_STRFUNC << ": table_name is empty." << std::endl;
    return;
  }

  //std::cout << "debug: " << G_STRFUNC << ": Calling Gda::Connection::update_meta_store_table(" << table_name << ") ..." << std::endl;
  //TODO: This doesn't seem to quite work yet:
  gda_connection->update_meta_store_table(table_name);

  //This does work, though it takes ages: gda_connection->update_meta_store();
  //std::cout << "debug: " << G_STRFUNC << ": ... Finished calling Gda::Connection::update_meta_store_table()" << std::endl;
}

bool create_database(Document* document, const Glib::ustring& database_name, const Glib::ustring& title, const sigc::slot<void>& progress)
{
#if 1
  // This seems to increase the chance that the database creation does not
  // fail due to the "source database is still in use" error. armin.
  //std::cout << "Going to sleep" << std::endl;
  Glib::usleep(500 * 1000);
  //std::cout << "Awake" << std::endl;
#endif

  progress();

  try
  {
    ConnectionPool::get_instance()->create_database(database_name);
  }
  catch(const Glib::Exception& ex) // libgda does not set error domain
  {
    std::cerr << G_STRFUNC << ":  Gnome::Gda::Connection::create_database(" << database_name << ") failed: " << ex.what() << std::endl;

    return false;
  }

  progress();

  //Connect to the actual database:
  ConnectionPool* connection_pool = ConnectionPool::get_instance();
  connection_pool->set_database(database_name);

  progress();

  sharedptr<SharedConnection> sharedconnection;
  try
  {
    sharedconnection = connection_pool->connect();
  }
  catch(const Glib::Exception& ex)
  {
    std::cerr << G_STRFUNC << ": Could not connect to just-created database. exception caught:" << ex.what() << std::endl;
    return false;
  }
  catch(const std::exception& ex)
  {
    std::cerr << G_STRFUNC << ": Could not connect to just-created database. exception caught:" << ex.what() << std::endl;
    return false;
  }

  if(sharedconnection)
  {
    progress();

    bool test = add_standard_tables(document); //Add internal, hidden, tables.
    if(!test)
    {
      std::cerr << G_STRFUNC << ": add_standard_tables() failed." << std::endl;
      return false;
    }
    
    progress();

    //Create the developer group, and make this user a member of it:
    //If we got this far then the user must really have developer privileges already:
    test = add_standard_groups(document);
    if(!test)
    {
      std::cerr << G_STRFUNC << ": add_standard_groups() failed." << std::endl;
      return false;
    }
    
    progress();

    //std::cout << "debug: " << G_STRFUNC << ": Creation of standard tables and groups finished." << std::endl;

    //Set the title based on the title in the example document, or the user-supplied title when creating new documents:
    SystemPrefs prefs = get_database_preferences(document);
    if(prefs.m_name.empty())
    {
      //std::cout << "debug: " << G_STRFUNC << ": Setting title in the database." << std::endl;
      prefs.m_name = title;
      set_database_preferences(document, prefs);
    }
    else
    {
      //std::cout << "debug: " << G_STRFUNC << ": database has title: " << prefs.m_name << std::endl;
    }

    progress();
    
    //Save the port, if appropriate, so the document can be used to connect again:
    Glom::ConnectionPool::Backend* backend = connection_pool->get_backend();
    Glom::ConnectionPoolBackends::PostgresCentralHosted* central = 
      dynamic_cast<Glom::ConnectionPoolBackends::PostgresCentralHosted*>(backend);
    if(central)
    {
      document->set_connection_port( central->get_port() );
    }

    return true;
  }
  else
  {
    std::cerr << G_STRFUNC << ": Could not connect to just-created database." << std::endl;
    return false;
  }
}

bool recreate_database_from_document(Document* document, const sigc::slot<void>& progress)
{
  ConnectionPool* connection_pool = ConnectionPool::get_instance();
  if(!connection_pool)
    return false; //Impossible anyway.

  //Check whether the database exists already.
  const Glib::ustring db_name = document->get_connection_database();
  if(db_name.empty())
    return false;

  connection_pool->set_database(db_name);
  try
  {
    connection_pool->set_ready_to_connect(); //This has succeeded already.
    sharedptr<SharedConnection> sharedconnection = connection_pool->connect();
    std::cerr << G_STRFUNC << ": Failed because database exists already." << std::endl;

    return false; //Connection to the database succeeded, because no exception was thrown. so the database exists already.
  }
  catch(const ExceptionConnection& ex)
  {
    if(ex.get_failure_type() == ExceptionConnection::FAILURE_NO_SERVER)
    {
      std::cerr << G_STRFUNC << ": Application::recreate_database(): Failed because connection to server failed even without specifying a database." << std::endl;
      return false;
    }

    //Otherwise continue, because we _expected_ connect() to fail if the db does not exist yet.
  }


  //Create the database:
  progress();
  connection_pool->set_database( Glib::ustring() );
  const bool db_created = create_database(document, db_name, document->get_database_title(), progress);

  if(!db_created)
  {
    return false;
  }
  else
    connection_pool->set_database(db_name); //Specify the new database when connecting from now on.

  progress();

  sharedptr<SharedConnection> sharedconnection;
  try
  {
    sharedconnection = connection_pool->connect();
      connection_pool->set_database(db_name); //The database was successfully created, so specify it when connecting from now on.
  }
  catch(const ExceptionConnection& ex)
  {
    std::cerr << G_STRFUNC << ": Failed to connect to the newly-created database." << std::endl;
    return false;
  }

  progress();

  //Create each table:
  Document::type_listTableInfo tables = document->get_tables();
  for(Document::type_listTableInfo::const_iterator iter = tables.begin(); iter != tables.end(); ++iter)
  {
    sharedptr<const TableInfo> table_info = *iter;

    //Create SQL to describe all fields in this table:
    Glib::ustring sql_fields;
    Document::type_vec_fields fields = document->get_table_fields(table_info->get_name());

    progress();
    const bool table_creation_succeeded = create_table(table_info, fields);
    progress();
    if(!table_creation_succeeded)
    {
      std::cerr << G_STRFUNC << ": CREATE TABLE failed with the newly-created database." << std::endl;
      return false;
    }
  }

  //Note that create_database() has already called add_standard_tables() and add_standard_groups(document).

  //Add groups from the document:
  progress();
  if(!add_groups_from_document(document))
  {
    std::cerr << G_STRFUNC << ": add_groups_from_document() failed." << std::endl;
    return false;
  }
  
  //Set table privileges, using the groups we just added:
  progress();
  if(!DbUtils::set_table_privileges_groups_from_document(document))
  {
    std::cerr << G_STRFUNC << ": set_table_privileges_groups_from_document() failed." << std::endl;
    return false;
  }
    
  for(Document::type_listTableInfo::const_iterator iter = tables.begin(); iter != tables.end(); ++iter)
  {
    sharedptr<const TableInfo> table_info = *iter;

    //Add any example data to the table:
    progress();

    //try
    //{
      progress();
      const bool table_insert_succeeded = insert_example_data(document, table_info->get_name());

      if(!table_insert_succeeded)
      {
        std::cerr << G_STRFUNC << ": INSERT of example data failed with the newly-created database." << std::endl;
        return false;
      }
    //}
    //catch(const std::exception& ex)
    //{
    //  std::cerr << G_STRFUNC << ": exception: " << ex.what() << std::endl;
      //HandleError(ex);
    //}

  } //for(tables)
  
  return true; //All tables created successfully.
}


SystemPrefs get_database_preferences(Document* document)
{
  //if(get_userlevel() == AppState::USERLEVEL_DEVELOPER)
  //  add_standard_tables(document);

  SystemPrefs result;

  //Check that the user is allowed to even view this table:
  //TODO_moved:
  //Privileges table_privs = Glom::Privs::get_current_privs(GLOM_STANDARD_TABLE_PREFS_TABLE_NAME);
  //if(!table_privs.m_view)
  //  return result;

  const bool optional_org_logo = get_field_exists_in_database(GLOM_STANDARD_TABLE_PREFS_TABLE_NAME, GLOM_STANDARD_TABLE_PREFS_FIELD_ORG_LOGO);

  Glib::RefPtr<Gnome::Gda::SqlBuilder> builder =
    Gnome::Gda::SqlBuilder::create(Gnome::Gda::SQL_STATEMENT_SELECT);
  builder->select_add_target(GLOM_STANDARD_TABLE_PREFS_TABLE_NAME);

  builder->select_add_field(GLOM_STANDARD_TABLE_PREFS_FIELD_NAME, GLOM_STANDARD_TABLE_PREFS_TABLE_NAME);
  builder->select_add_field(GLOM_STANDARD_TABLE_PREFS_FIELD_ORG_NAME, GLOM_STANDARD_TABLE_PREFS_TABLE_NAME);
  builder->select_add_field(GLOM_STANDARD_TABLE_PREFS_FIELD_ORG_ADDRESS_STREET, GLOM_STANDARD_TABLE_PREFS_TABLE_NAME);
  builder->select_add_field(GLOM_STANDARD_TABLE_PREFS_FIELD_ORG_ADDRESS_STREET2, GLOM_STANDARD_TABLE_PREFS_TABLE_NAME);
  builder->select_add_field(GLOM_STANDARD_TABLE_PREFS_FIELD_ORG_ADDRESS_TOWN, GLOM_STANDARD_TABLE_PREFS_TABLE_NAME);
  builder->select_add_field(GLOM_STANDARD_TABLE_PREFS_FIELD_ORG_ADDRESS_COUNTY, GLOM_STANDARD_TABLE_PREFS_TABLE_NAME);
  builder->select_add_field(GLOM_STANDARD_TABLE_PREFS_FIELD_ORG_ADDRESS_COUNTRY, GLOM_STANDARD_TABLE_PREFS_TABLE_NAME);
  builder->select_add_field(GLOM_STANDARD_TABLE_PREFS_FIELD_ORG_ADDRESS_POSTCODE, GLOM_STANDARD_TABLE_PREFS_TABLE_NAME);

  if(optional_org_logo)
  {
    builder->select_add_field(GLOM_STANDARD_TABLE_PREFS_FIELD_ORG_LOGO, GLOM_STANDARD_TABLE_PREFS_TABLE_NAME);
  }

  int attempts = 0;
  while(attempts < 2)
  {
    bool succeeded = true;
    try
    {
      //const std::string full_query = Utils::sqlbuilder_get_full_query(builder);
      Glib::RefPtr<Gnome::Gda::DataModel> datamodel = query_execute_select(builder);
      if(datamodel && (datamodel->get_n_rows() != 0))
      {
        result.m_name = Conversions::get_text_for_gda_value(Field::TYPE_TEXT, datamodel->get_value_at(0, 0));
        result.m_org_name = Conversions::get_text_for_gda_value(Field::TYPE_TEXT, datamodel->get_value_at(1, 0));
        result.m_org_address_street = Conversions::get_text_for_gda_value(Field::TYPE_TEXT, datamodel->get_value_at(2, 0));
        result.m_org_address_street2 = Conversions::get_text_for_gda_value(Field::TYPE_TEXT, datamodel->get_value_at(3, 0));
        result.m_org_address_town = Conversions::get_text_for_gda_value(Field::TYPE_TEXT, datamodel->get_value_at(4, 0));
        result.m_org_address_county = Conversions::get_text_for_gda_value(Field::TYPE_TEXT, datamodel->get_value_at(5, 0));
        result.m_org_address_country = Conversions::get_text_for_gda_value(Field::TYPE_TEXT, datamodel->get_value_at(6, 0));
        result.m_org_address_postcode = Conversions::get_text_for_gda_value(Field::TYPE_TEXT, datamodel->get_value_at(7, 0));

        //We need to be more clever about these column indexes if we add more new fields:
        if(optional_org_logo)
          result.m_org_logo = datamodel->get_value_at(8, 0);
      }
      else
        succeeded = false;
    }
    catch(const Glib::Exception& ex)
    {
      std::cerr << G_STRFUNC << ": exception: " << ex.what() << std::endl;
      succeeded = false;
    }
    catch(const std::exception& ex)
    {
      std::cerr << G_STRFUNC << ": exception: " << ex.what() << std::endl;
      succeeded = false;
    }
    //Return the result, or try again:
    if(succeeded)
      return result;
    else
    {
      const bool test = add_standard_tables(document);
      if(!test)
      {
         std::cerr << G_STRFUNC << ": add_standard_tables() failed." << std::endl;
      }
      
      ++attempts; //Try again now that we have tried to create the table.
    }
  }

  return result;
}


void set_database_preferences(Document* document, const SystemPrefs& prefs)
{
   //The logo field was introduced in a later version of Glom.
  //If the user is not in developer mode then the new field has not yet been added:

  Glib::RefPtr<Gnome::Gda::SqlBuilder> builder = Gnome::Gda::SqlBuilder::create(Gnome::Gda::SQL_STATEMENT_UPDATE);
  builder->set_table(GLOM_STANDARD_TABLE_PREFS_TABLE_NAME);
  builder->add_field_value(GLOM_STANDARD_TABLE_PREFS_FIELD_NAME, prefs.m_name);
  builder->add_field_value(GLOM_STANDARD_TABLE_PREFS_FIELD_ORG_NAME, prefs.m_org_name);
  builder->add_field_value(GLOM_STANDARD_TABLE_PREFS_FIELD_ORG_ADDRESS_STREET, prefs.m_org_address_street);
  builder->add_field_value(GLOM_STANDARD_TABLE_PREFS_FIELD_ORG_ADDRESS_STREET2, prefs.m_org_address_street2);
  builder->add_field_value(GLOM_STANDARD_TABLE_PREFS_FIELD_ORG_ADDRESS_TOWN, prefs.m_org_address_town);
  builder->add_field_value(GLOM_STANDARD_TABLE_PREFS_FIELD_ORG_ADDRESS_COUNTY, prefs.m_org_address_county);
  builder->add_field_value(GLOM_STANDARD_TABLE_PREFS_FIELD_ORG_ADDRESS_COUNTRY, prefs.m_org_address_country);
  builder->add_field_value(GLOM_STANDARD_TABLE_PREFS_FIELD_ORG_ADDRESS_POSTCODE, prefs.m_org_address_postcode);
  if(get_field_exists_in_database(GLOM_STANDARD_TABLE_PREFS_TABLE_NAME, GLOM_STANDARD_TABLE_PREFS_FIELD_ORG_LOGO))
  {
    builder->add_field_value(GLOM_STANDARD_TABLE_PREFS_FIELD_ORG_LOGO, prefs.m_org_logo);
  }
  builder->set_where(builder->add_cond(Gnome::Gda::SQL_OPERATOR_TYPE_EQ,
                                       builder->add_field_id(GLOM_STANDARD_TABLE_PREFS_FIELD_ID, GLOM_STANDARD_TABLE_PREFS_TABLE_NAME),
                                       builder->add_expr(1)));
  const bool test = query_execute(builder);

  if(!test)
    std::cerr << G_STRFUNC << ": UPDATE failed." << std::endl;

  //Set some information in the document too, so we can use it to recreate the database:
  document->set_database_title(prefs.m_name);
}


bool add_standard_tables(Document* document)
{
  try
  {
    Document::type_vec_fields pref_fields;
    sharedptr<TableInfo> prefs_table_info = Document::create_table_system_preferences(pref_fields);

    //Name, address, etc:
    if(!get_table_exists_in_database(GLOM_STANDARD_TABLE_PREFS_TABLE_NAME))
    {
      const bool test = create_table(prefs_table_info, pref_fields);

      if(test)
      {
        //Add the single record:
        Glib::RefPtr<Gnome::Gda::SqlBuilder> builder = Gnome::Gda::SqlBuilder::create(Gnome::Gda::SQL_STATEMENT_INSERT);
        builder->set_table(GLOM_STANDARD_TABLE_PREFS_TABLE_NAME);
        builder->add_field_value(GLOM_STANDARD_TABLE_PREFS_FIELD_ID, 1);
        const bool test = query_execute(builder);
        if(!test)
          std::cerr << G_STRFUNC << ": INSERT failed." << std::endl;

        //Use the database title from the document, if there is one:
        const Glib::ustring system_name = document->get_database_title();
        if(!system_name.empty())
        {
          Glib::RefPtr<Gnome::Gda::SqlBuilder> builder = Gnome::Gda::SqlBuilder::create(Gnome::Gda::SQL_STATEMENT_UPDATE);
          builder->set_table(GLOM_STANDARD_TABLE_PREFS_TABLE_NAME);
          builder->add_field_value(GLOM_STANDARD_TABLE_PREFS_FIELD_NAME, system_name);
          builder->set_where(builder->add_cond(Gnome::Gda::SQL_OPERATOR_TYPE_EQ,
                                               builder->add_field_id(GLOM_STANDARD_TABLE_PREFS_FIELD_ID, GLOM_STANDARD_TABLE_PREFS_TABLE_NAME),
                                               builder->add_expr(1)));
          const bool test = query_execute(builder);
          if(!test)
            std::cerr << G_STRFUNC << ": UPDATE failed." << std::endl;
        }
      }
      else
      {
        std::cerr << G_STRFUNC << ": add_standard_tables(): create_table(prefs) failed." << std::endl;
        return false;
      }
    }
    else
    {
      //Make sure that it has all the fields it should have,
      //because we sometimes add some in new Glom versions:
      create_table_add_missing_fields(prefs_table_info, pref_fields);
    }

    //Auto-increment next values:
    if(!get_table_exists_in_database(GLOM_STANDARD_TABLE_AUTOINCREMENTS_TABLE_NAME))
    {
      sharedptr<TableInfo> table_info(new TableInfo());
      table_info->set_name(GLOM_STANDARD_TABLE_AUTOINCREMENTS_TABLE_NAME);
      table_info->set_title("System: Auto Increments"); //TODO: Provide standard translations.
      table_info->m_hidden = true;

      Document::type_vec_fields fields;

      sharedptr<Field> primary_key(new Field()); //It's not used, because there's only one record, but we must have one.
      primary_key->set_name(GLOM_STANDARD_TABLE_AUTOINCREMENTS_FIELD_ID);
      primary_key->set_glom_type(Field::TYPE_NUMERIC);
      fields.push_back(primary_key);

      sharedptr<Field> field_table_name(new Field());
      field_table_name->set_name(GLOM_STANDARD_TABLE_AUTOINCREMENTS_FIELD_TABLE_NAME);
      field_table_name->set_glom_type(Field::TYPE_TEXT);
      fields.push_back(field_table_name);

      sharedptr<Field> field_field_name(new Field());
      field_field_name->set_name(GLOM_STANDARD_TABLE_AUTOINCREMENTS_FIELD_FIELD_NAME);
      field_field_name->set_glom_type(Field::TYPE_TEXT);
      fields.push_back(field_field_name);

      sharedptr<Field> field_next_value(new Field());
      field_next_value->set_name(GLOM_STANDARD_TABLE_AUTOINCREMENTS_FIELD_NEXT_VALUE);
      field_next_value->set_glom_type(Field::TYPE_TEXT);
      fields.push_back(field_next_value);

      const bool test = create_table(table_info, fields);
      if(!test)
      {
        std::cerr << G_STRFUNC << ": add_standard_tables(): create_table(autoincrements) failed." << std::endl;
        return false;
      }

      return true;
    }
    else
    {
      return false;
    }
  }
  catch(const Glib::Exception& ex)
  {
    std::cerr << G_STRFUNC << ": caught exception: " << ex.what() << std::endl;
    return false;
  }
  catch(const std::exception& ex)
  {
    std::cerr << G_STRFUNC << ": caught exception: " << ex.what() << std::endl;
    return false;
  }
}

bool add_standard_groups(Document* document)
{
  //Add the glom_developer group if it does not exist:
  const Glib::ustring devgroup = GLOM_STANDARD_GROUP_NAME_DEVELOPER;

  Glib::RefPtr<Gnome::Gda::Connection> gda_connection = get_connection();
  if(!gda_connection)
  {
    std::cerr << G_STRFUNC << ": No connection yet." << std::endl;
  }

  // If the connection doesn't support users we can skip this step
  if(gda_connection->supports_feature(Gnome::Gda::CONNECTION_FEATURE_USERS))
  {
    const type_vec_strings vecGroups = Glom::Privs::get_database_groups();
    type_vec_strings::const_iterator iterFind = std::find(vecGroups.begin(), vecGroups.end(), devgroup);
    if(iterFind == vecGroups.end())
    {
      //TODO: Escape and quote the user and group names here?
      //The "SUPERUSER" here has no effect because SUPERUSER is not "inherited" to member users.
      //But let's keep it to make the purpose of this group obvious.
      bool test = query_execute_string(
        DbUtils::build_query_create_group(GLOM_STANDARD_GROUP_NAME_DEVELOPER, true /* superuser */));
      if(!test)
      {
        std::cerr << G_STRFUNC << ": CREATE GROUP failed when adding the developer group." << std::endl;
        return false;
      }

      //Make sure the current user is in the developer group.
      //(If he is capable of creating these groups then he is obviously a developer, and has developer rights on the postgres server.)
      const Glib::ustring current_user = ConnectionPool::get_instance()->get_user();
      const Glib::ustring strQuery = build_query_add_user_to_group(GLOM_STANDARD_GROUP_NAME_DEVELOPER, current_user);
      test = query_execute_string(strQuery);
      if(!test)
      {
        std::cerr << G_STRFUNC << ": ALTER GROUP failed when adding the user to the developer group." << std::endl;
        return false;
      }

      //std::cout << "DEBUG: Added user " << current_user << " to glom developer group on postgres server." << std::endl;

      Privileges priv_devs;
      priv_devs.m_view = true;
      priv_devs.m_edit = true;
      priv_devs.m_create = true;
      priv_devs.m_delete = true;

      Document::type_listTableInfo table_list = document->get_tables(true /* including system prefs */);

      for(Document::type_listTableInfo::const_iterator iter = table_list.begin(); iter != table_list.end(); ++iter)
      {
        sharedptr<const TableInfo> table_info = *iter;
        if(table_info)
        {
          const Glib::ustring table_name = table_info->get_name();
          if(get_table_exists_in_database(table_name)) //Maybe the table has not been created yet.
            Glom::Privs::set_table_privileges(devgroup, table_name, priv_devs, true /* developer privileges */);
        }
      }

      //Make sure that it is in the database too:
      GroupInfo group_info;
      group_info.set_name(GLOM_STANDARD_GROUP_NAME_DEVELOPER);
      group_info.m_developer = true;
      document->set_group(group_info);
    }
  }
  else
  {
    std::cout << "DEBUG: Connection does not support users" << std::endl;
  }

  return true;
}

bool add_groups_from_document(Document* document)
{
  Glib::RefPtr<Gnome::Gda::Connection> gda_connection = get_connection();
  if(!gda_connection)
  {
    std::cerr << G_STRFUNC << ": add_standard_groups(): No connection yet." << std::endl;
  }

  // If the connection doesn't support users we can skip this step
  if(!(gda_connection->supports_feature(Gnome::Gda::CONNECTION_FEATURE_USERS)))
    return true;

  //Get the list of groups from the database server:
  const type_vec_strings database_groups = Privs::get_database_groups();

  //Get the list of groups from the document:
  const Document::type_list_groups document_groups = document->get_groups();

  //Add each group if it doesn't exist yet:
  for(Document::type_list_groups::const_iterator iter = document_groups.begin();
    iter != document_groups.end(); ++iter)
  {
    const GroupInfo& group = *iter;
    const Glib::ustring name = group.get_name();

    //See if the group exists in the database:
    type_vec_strings::const_iterator iterFind = std::find(database_groups.begin(), database_groups.end(), name);
    if(!name.empty() && iterFind == database_groups.end())
    {
      const Glib::ustring query = build_query_create_group(name, group.m_developer);
      const bool test = query_execute_string(query);
      if(!test)
      {
        std::cerr << G_STRFUNC << ": CREATE GROUP failed when adding the group with name=" << name << std::endl;
        return false;
      }
    }
  }

  return true;
}

bool set_table_privileges_groups_from_document(Document* document)
{
  Glib::RefPtr<Gnome::Gda::Connection> gda_connection = get_connection();
  if(!gda_connection)
  {
    std::cerr << G_STRFUNC << ": add_standard_groups(): No connection yet." << std::endl;
  }

  // If the connection doesn't support users we can skip this step
  if(!(gda_connection->supports_feature(Gnome::Gda::CONNECTION_FEATURE_USERS)))
    return true;

  //Get the list of groups from the database server:
  const type_vec_strings database_groups = Privs::get_database_groups();

  //Get the list of groups from the document:
  const Document::type_list_groups document_groups = document->get_groups();

  //Get the list of tables:
  const Document::type_listTableInfo table_list = document->get_tables();

  bool result = true;

  for(Document::type_list_groups::const_iterator iter = document_groups.begin();
    iter != document_groups.end(); ++iter)
  {
    const GroupInfo& group_info = *iter;
    const Glib::ustring group_name = group_info.get_name();

    //See if the group exists in the database:
    type_vec_strings::const_iterator iterFind = std::find(database_groups.begin(), database_groups.end(), group_name);
    if(!group_name.empty() && iterFind == database_groups.end())
    {
      std::cerr << G_STRFUNC << ": group does not exist in the database. group name=" << group_name << std::endl;
      result = false;
      continue;
    }

    //Look at each table privilege for this group:
    for(GroupInfo::type_map_table_privileges::const_iterator iter = group_info.m_map_privileges.begin();
      iter != group_info.m_map_privileges.end(); ++iter)
    {
      const Glib::ustring table_name = iter->first;
      const Privileges& privs = iter->second;

      //Set the table privilege for the group:
      Privs::set_table_privileges(group_name, table_name, privs, group_info.m_developer);
    }
  }

  return result;
}

namespace { //anonymous

//If the string has quotes around it, remove them
static Glib::ustring remove_quotes(const Glib::ustring& str)
{
  const gchar* quote = "\"";
  const Glib::ustring::size_type posQuoteStart = str.find(quote);
  if(posQuoteStart != 0)
    return str;

  const Glib::ustring::size_type size = str.size();
  const Glib::ustring::size_type posQuoteEnd = str.find(quote, 1);
  if(posQuoteEnd != (size - 1))
    return str;

  return str.substr(1, size - 2);
}

} //anonymous namespace

static bool meta_table_column_is_primary_key(GdaMetaTable* meta_table, const Glib::ustring column_name)
{
  if(!meta_table)
    return false;

  for(GSList* item = meta_table->columns; item != 0; item = item->next)
  {
    GdaMetaTableColumn* column = GDA_META_TABLE_COLUMN(item->data);
    if(!column)
      continue;

    if(column->column_name && (column_name == remove_quotes(column->column_name)))
      return column->pkey;
  }

  return false;
}

bool handle_error()
{
  return ConnectionPool::handle_error_cerr_only();
}

void handle_error(const Glib::Exception& ex)
{
  std::cerr << G_STRFUNC << ": Internal Error (handle_error()): exception type=" << typeid(ex).name() << ", ex.what()=" << ex.what() << std::endl;

  //TODO_Moved:
  //Gtk::MessageDialog dialog(Utils::bold_message(_("Internal error")), true, Gtk::MESSAGE_WARNING );
  //dialog.set_secondary_text(ex.what());
  //TODO: dialog.set_transient_for(*get_application());
  //dialog.run();
}

void handle_error(const std::exception& ex)
{
  std::cerr << G_STRFUNC << ": Internal Error (handle_error()): exception type=" << typeid(ex).name() << ", ex.what()=" << ex.what() << std::endl;

 //TODO_Moved:
  //Gtk::MessageDialog dialog(Utils::bold_message(_("Internal error")), true, Gtk::MESSAGE_WARNING );
  //dialog.set_secondary_text(ex.what());
  //TODO: dialog.set_transient_for(*get_application());
  //dialog.run();
}


bool get_field_exists_in_database(const Glib::ustring& table_name, const Glib::ustring& field_name)
{
  type_vec_fields vecFields = get_fields_for_table_from_database(table_name);
  type_vec_fields::const_iterator iterFind = std::find_if(vecFields.begin(), vecFields.end(), predicate_FieldHasName<Field>(field_name));
  return iterFind != vecFields.end();
}

type_vec_fields get_fields_for_table_from_database(const Glib::ustring& table_name, bool /* including_system_fields */)
{
  type_vec_fields result;

  if(table_name.empty())
    return result;

  // These are documented here:
  // http://library.gnome.org/devel/libgda-4.0/3.99/connection.html#GdaConnectionMetaTypeHead
  enum GlomGdaDataModelFieldColumns
  {
    DATAMODEL_FIELDS_COL_NAME = 0,
    DATAMODEL_FIELDS_COL_TYPE = 1,
    DATAMODEL_FIELDS_COL_GTYPE = 2,
    DATAMODEL_FIELDS_COL_SIZE = 3,
    DATAMODEL_FIELDS_COL_SCALE = 4,
    DATAMODEL_FIELDS_COL_NOTNULL = 5,
    DATAMODEL_FIELDS_COL_DEFAULTVALUE = 6,
    DATAMODEL_FIELDS_COL_EXTRA = 6 // Could be auto-increment
  };

  //TODO: BusyCursor busy_cursor(get_application());

  {
    Glib::RefPtr<Gnome::Gda::Connection> connection = get_connection();
    if(!connection)
    {
      std::cerr << G_STRFUNC << ": connection is null" << std::endl;
      return result;
    }

    Glib::RefPtr<Gnome::Gda::Holder> holder_table_name = Gnome::Gda::Holder::create(G_TYPE_STRING, "name");
    gchar* quoted_table_name_c = gda_meta_store_sql_identifier_quote(table_name.c_str(), connection->gobj());
    g_assert(quoted_table_name_c);
    Glib::ustring quoted_table_name(quoted_table_name_c);
    g_free (quoted_table_name_c);
    quoted_table_name_c = 0;

    holder_table_name->set_value(quoted_table_name);

    std::vector< Glib::RefPtr<Gnome::Gda::Holder> > holder_list;
    holder_list.push_back(holder_table_name);

    Glib::RefPtr<Gnome::Gda::DataModel> data_model_fields;
    try
    {
      //This should work because we called update_meta_store_tables() in ConnectionPool,
      //and that gets the tables' fields too.
      data_model_fields = connection->get_meta_store_data(Gnome::Gda::CONNECTION_META_FIELDS, holder_list);
    }
    catch(const Gnome::Gda::MetaStoreError& ex)
    {
      std::cerr << G_STRFUNC << ": MetaStoreError: " << ex.what() << std::endl;
    }
    catch(const Glib::Error& ex)
    {
      std::cerr << G_STRFUNC << ": Error: " << ex.what() << std::endl;
    }


    if(!data_model_fields)
    {
      std::cerr << G_STRFUNC << ": libgda reported empty fields schema data_model for the table." << std::endl;
    }
    else if(data_model_fields->get_n_columns() == 0)
    {
      std::cerr << G_STRFUNC << ": libgda reported 0 fields for the table." << std::endl;
    }
    else if(data_model_fields->get_n_rows() == 0)
    {
      g_warning("get_fields_for_table_from_database(): table_name=%s, data_model_fields->get_n_rows() == 0: The table probably does not exist in the specified database.", table_name.c_str());
    }
    else
    {
      //We also use the GdaMetaTable to discover primary keys.
      //Both these libgda APIs are awful, and it's awful that we must use two APIs. murrayc.
      Glib::RefPtr<Gnome::Gda::MetaStore> store = connection->get_meta_store();
      Glib::RefPtr<Gnome::Gda::MetaStruct> metastruct =
        Gnome::Gda::MetaStruct::create(store, Gnome::Gda::META_STRUCT_FEATURE_NONE);
      GdaMetaDbObject* meta_dbobject = 0;
      try
      {
        meta_dbobject = metastruct->complement(Gnome::Gda::META_DB_TABLE,
          Gnome::Gda::Value(), /* catalog */
          Gnome::Gda::Value(), /* schema */
          Gnome::Gda::Value(quoted_table_name)); //It's a static instance inside the MetaStore.
      }
      catch(const Glib::Error& ex)
      {
        handle_error(ex);
        //TODO: Really fail.
      }
      GdaMetaTable* meta_table = GDA_META_TABLE(meta_dbobject);

      //Examine each field:
      guint row = 0;
      const gulong rows_count = data_model_fields->get_n_rows();
      while(row < rows_count)
      {
        Glib::RefPtr<Gnome::Gda::Column> field_info = Gnome::Gda::Column::create();

        //Get the field name:
        const Gnome::Gda::Value value_name = data_model_fields->get_value_at(DATAMODEL_FIELDS_COL_NAME, row);
        if(value_name.get_value_type() ==  G_TYPE_STRING)
        {
          if(value_name.get_string().empty())
            g_warning("get_fields_for_table_from_database(): value_name is empty.");

          Glib::ustring field_name = value_name.get_string(); //TODO: get_string() is a dodgy choice. murrayc.
          field_name = remove_quotes(field_name);
          field_info->set_name(field_name);
          //std::cout << "  debug: field_name=" << field_name << std::endl;
        }

        //Get the field type:
        const Gnome::Gda::Value value_fieldtype = data_model_fields->get_value_at(DATAMODEL_FIELDS_COL_GTYPE, row);
        if(value_fieldtype.get_value_type() ==  G_TYPE_STRING)
        {
          const Glib::ustring type_string = value_fieldtype.get_string();
          const GType gdatype = gda_g_type_from_string(type_string.c_str());
          field_info->set_g_type(gdatype);
        }


        //Get the default value:
        /* libgda does not return this correctly yet. TODO: check bug http://bugzilla.gnome.org/show_bug.cgi?id=143576
        const Gnome::Gda::Value value_defaultvalue = data_model_fields->get_value_at(DATAMODEL_FIELDS_COL_DEFAULTVALUE, row);
        if(value_defaultG_VALUE_TYPE(value.gobj()) ==  G_TYPE_STRING)
          field_info->set_default_value(value_defaultvalue);
        */

        //Get whether it can be null:
        const Gnome::Gda::Value value_notnull = data_model_fields->get_value_at(DATAMODEL_FIELDS_COL_NOTNULL, row);
        if(value_notnull.get_value_type() ==  G_TYPE_BOOLEAN)
          field_info->set_allow_null(value_notnull.get_boolean());


        sharedptr<Field> field = sharedptr<Field>::create(); //TODO: Get glom-specific information from the document?
        field->set_field_info(field_info);


        //Get whether it is a primary key:
        field->set_primary_key(
          meta_table_column_is_primary_key(meta_table, field_info->get_name()) );

        result.push_back(field);

        ++row;
      }
    }
  }

  if(result.empty())
  {
    //g_warning("get_fields_for_table_from_database(): returning empty result.");
  }

  //Hide system fields.
  type_vec_fields::iterator iterFind = std::find_if(result.begin(), result.end(), predicate_FieldHasName<Field>(GLOM_STANDARD_FIELD_LOCK));
  if(iterFind != result.end())
    result.erase(iterFind);

  return result;
}

//TODO_Performance: Avoid calling this so often.
//TODO: Put this in libgdamm.
type_vec_strings get_table_names_from_database(bool ignore_system_tables)
{
  type_vec_strings result;

  {
    Glib::RefPtr<Gnome::Gda::Connection> gda_connection = get_connection();

    Glib::RefPtr<Gnome::Gda::DataModel> data_model_tables;
    try
    {
      //This should work because we called update_meta_store_tables() in ConnectionPool.
      data_model_tables = gda_connection->get_meta_store_data(Gnome::Gda::CONNECTION_META_TABLES);
    }
    catch(const Gnome::Gda::MetaStoreError& ex)
    {
      std::cerr << G_STRFUNC << ": MetaStoreError: " << ex.what() << std::endl;
    }
    catch(const Glib::Error& ex)
    {
      std::cerr << G_STRFUNC << ": Error: " << ex.what() << std::endl;
    }

    if(!data_model_tables)
    {
      std::cerr << G_STRFUNC << ": libgda returned an empty tables GdaDataModel for the database." << std::endl;
    }
    else if(data_model_tables->get_n_columns() <= 0)
    {
      std::cerr << G_STRFUNC << ": libgda reported 0 tables for the database." << std::endl;
    }
    else
    {
      //std::cout << "debug: data_model_tables refcount=" << G_OBJECT(data_model_tables->gobj())->ref_count << std::endl;
      const int rows = data_model_tables->get_n_rows();
      for(int i = 0; i < rows; ++i)
      {
        const Gnome::Gda::Value value = data_model_tables->get_value_at(0, i);
        //Get the table name:
        Glib::ustring table_name;
        if(G_VALUE_TYPE(value.gobj()) ==  G_TYPE_STRING)
        {
          table_name = value.get_string();

          //The table names have quotes sometimes. See http://bugzilla.gnome.org/show_bug.cgi?id=593154
          table_name = remove_quotes(table_name);

          //TODO: Unescape the string with gda_server_provider_unescape_string()?

          //std::cout << "DEBUG: Found table: " << table_name << std::endl;

          if(ignore_system_tables)
          {
            //Check whether it's a system table:
            const Glib::ustring prefix = "glom_system_";
            const Glib::ustring table_prefix = table_name.substr(0, prefix.size());
            if(table_prefix == prefix)
              continue;
          }

          //Ignore the pga_* tables that pgadmin adds when you use it:
          if(table_name.substr(0, 4) == "pga_")
            continue;

          //Ignore the pg_* tables that something (Postgres? libgda?) adds:
          //Not needed now that this was fixed again in libgda-4.0.
          //if(table_name.substr(0, 14) == "pg_catalog.pg_")
          //  continue;

          //Ignore the information_schema tables that something (libgda?) adds:
          //Not needed now that this was fixed again in libgda-4.0.
          //if(table_name.substr(0, 23) == "information_schema.sql_")
          //  continue;

          result.push_back(table_name);
        }
      }
    }
  }

  return result;
}

bool get_table_exists_in_database(const Glib::ustring& table_name)
{
  //TODO_Performance

  type_vec_strings tables = get_table_names_from_database();
  type_vec_strings::const_iterator iterFind = std::find(tables.begin(), tables.end(), table_name);
  return (iterFind != tables.end());
}


bool create_table_with_default_fields(Document* document, const Glib::ustring& table_name)
{
  if(table_name.empty())
    return false;

  Glib::RefPtr<Gnome::Gda::Connection> gda_connection = get_connection();
  if(!gda_connection)
  {
    std::cerr << G_STRFUNC << ": No connection yet." << std::endl;
    return false;
  }

  bool created = false;

  //Primary key:
  sharedptr<Field> field_primary_key(new Field());
  field_primary_key->set_name(table_name + "_id");
  field_primary_key->set_title(table_name + " ID");
  field_primary_key->set_primary_key();
  field_primary_key->set_auto_increment();

  Glib::RefPtr<Gnome::Gda::Column> field_info = field_primary_key->get_field_info();
  field_info->set_allow_null(false);
  field_primary_key->set_field_info(field_info);

  field_primary_key->set_glom_type(Field::TYPE_NUMERIC);
  //std::cout << "debug: " << G_STRFUNC << ":" << field_primary_key->get_auto_increment() << std::endl;

  type_vec_fields fields;
  fields.push_back(field_primary_key);

  //Description:
  sharedptr<Field> field_description(new Field());
  field_description->set_name("description");
  field_description->set_title(_("Description")); //Use a translation, because the original locale will be marked as non-English if the current locale is non-English.
  field_description->set_glom_type(Field::TYPE_TEXT);
  fields.push_back(field_description);

  //Comments:
  sharedptr<Field> field_comments(new Field());
  field_comments->set_name("comments");
  field_comments->set_title(_("Comments"));
  field_comments->set_glom_type(Field::TYPE_TEXT);
  field_comments->m_default_formatting.set_text_format_multiline();
  fields.push_back(field_comments);

  sharedptr<TableInfo> table_info(new TableInfo());
  table_info->set_name(table_name);
  table_info->set_title( Utils::title_from_string( table_name ) ); //Start with a title that might be appropriate.

  created = create_table(table_info, fields);

  if(created)
  {
    //Save the changes in the document:
    if(document)
    {
      document->add_table(table_info);
      document->set_table_fields(table_info->get_name(), fields);
    }
  }

  return created;
}
bool create_table(const sharedptr<const TableInfo>& table_info, const Document::type_vec_fields& fields_in)
{
  //std::cout << "debug: " << G_STRFUNC << ": " << table_info->get_name() << ", title=" << table_info->get_title() << std::endl;

  bool table_creation_succeeded = false;


  Document::type_vec_fields fields = fields_in;

  //Create the standard field too:
  //(We don't actually use this yet)
  if(std::find_if(fields.begin(), fields.end(), predicate_FieldHasName<Field>(GLOM_STANDARD_FIELD_LOCK)) == fields.end())
  {
    sharedptr<Field> field = sharedptr<Field>::create();
    field->set_name(GLOM_STANDARD_FIELD_LOCK);
    field->set_glom_type(Field::TYPE_TEXT);
    fields.push_back(field);
  }

  //Create SQL to describe all fields in this table:
  Glib::ustring sql_fields;
  for(Document::type_vec_fields::const_iterator iter = fields.begin(); iter != fields.end(); ++iter)
  {
    //Create SQL to describe this field:
    sharedptr<Field> field = *iter;

    //The field has no gda type, so we set that:
    //This usually comes from the database, but that's a bit strange.
    Glib::RefPtr<Gnome::Gda::Column> info = field->get_field_info();
    info->set_g_type( Field::get_gda_type_for_glom_type(field->get_glom_type()) );
    field->set_field_info(info); //TODO_Performance

    Glib::ustring sql_field_description = escape_sql_id(field->get_name()) + " " + field->get_sql_type();

    if(field->get_primary_key())
      sql_field_description += " NOT NULL  PRIMARY KEY";

    //Append it:
    if(!sql_fields.empty())
      sql_fields += ", ";

    sql_fields += sql_field_description;
  }

  if(sql_fields.empty())
  {
    std::cerr << G_STRFUNC << ": sql_fields is empty." << std::endl;
  }

  //Actually create the table
  try
  {
    //TODO: Escape the table name?
    //TODO: Use GDA_SERVER_OPERATION_CREATE_TABLE instead?
    table_creation_succeeded = query_execute_string( "CREATE TABLE " + escape_sql_id(table_info->get_name()) + " (" + sql_fields + ");" );
    if(!table_creation_succeeded)
      std::cerr << G_STRFUNC << ": CREATE TABLE failed." << std::endl;
  }
  catch(const ExceptionConnection& ex)
  {
    std::cerr << G_STRFUNC << "CREATE TABLE failed: " << ex.what() << std::endl;
    table_creation_succeeded = false;
  }

  if(table_creation_succeeded)
  {
    // Update the libgda meta store, so that get_fields_for_table_from_database()
    // returns the fields correctly for the new table.
    update_gda_metastore_for_table(table_info->get_name());

    // TODO: Maybe we should create the table directly via libgda instead of
    // executing an SQL query ourselves, so that libgda has the chance to
    // do this meta store update automatically.
    // (Yes, generally it would be nice to use libgda API instead of generating SQL. murrayc)
  }

  return table_creation_succeeded;
}

bool create_table_add_missing_fields(const sharedptr<const TableInfo>& table_info, const Document::type_vec_fields& fields)
{
  const Glib::ustring table_name = table_info->get_name();

  for(Document::type_vec_fields::const_iterator iter = fields.begin(); iter != fields.end(); ++iter)
  {
    sharedptr<const Field> field = *iter;
    if(!get_field_exists_in_database(table_name, field->get_name()))
    {
      const bool test = add_column(table_name, field, 0); /* TODO: parent_window */
      if(!test)
       return test;
    }
  }

  return true;
}


bool add_column(const Glib::ustring& table_name, const sharedptr<const Field>& field, Gtk::Window* /* parent_window */)
{
  ConnectionPool* connection_pool = ConnectionPool::get_instance();

  try
  {
    connection_pool->add_column(table_name, field);
  }
  catch(const Glib::Error& ex)
  {
    handle_error(ex);
//    Gtk::MessageDialog window(*parent_window, Utils::bold_message(ex.what()), true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
//    window.run();
    return false;
  }

  return true;
}

bool drop_column(const Glib::ustring& table_name, const Glib::ustring& field_name)
{
  ConnectionPool* connection_pool = ConnectionPool::get_instance();

  try
  {
    return connection_pool->drop_column(table_name, field_name);
  }
  catch(const Glib::Error& ex)
  {
    handle_error(ex);
//    Gtk::MessageDialog window(*parent_window, Utils::bold_message(ex.what()), true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
//    window.run();
    return false;
  }

  return true;
}

static void builder_set_where_autoincrement(const Glib::RefPtr<Gnome::Gda::SqlBuilder>& builder, const Glib::ustring& table_name, const Glib::ustring& field_name)
{
  if(table_name.empty())
  {
    std::cerr << G_STRFUNC << ": table_name is empty" << std::endl;
    return;
  }
  
  if(field_name.empty())
  {
    std::cerr << G_STRFUNC << ": field_name is empty" << std::endl;
    return;
  }
  
  builder->set_where(builder->add_cond(Gnome::Gda::SQL_OPERATOR_TYPE_AND,
    builder->add_cond(Gnome::Gda::SQL_OPERATOR_TYPE_EQ,
      builder->add_field_id(GLOM_STANDARD_TABLE_AUTOINCREMENTS_FIELD_TABLE_NAME, GLOM_STANDARD_TABLE_AUTOINCREMENTS_TABLE_NAME),
      builder->add_expr(table_name)),
    builder->add_cond(Gnome::Gda::SQL_OPERATOR_TYPE_EQ,
      builder->add_field_id(GLOM_STANDARD_TABLE_AUTOINCREMENTS_FIELD_FIELD_NAME, GLOM_STANDARD_TABLE_AUTOINCREMENTS_TABLE_NAME),
      builder->add_expr(field_name))));
}

Gnome::Gda::Value get_next_auto_increment_value(const Glib::ustring& table_name, const Glib::ustring& field_name)
{
  if(table_name.empty())
  {
    std::cerr << G_STRFUNC << ": table_name is empty" << std::endl;
    return Gnome::Gda::Value();
  }
  
  if(field_name.empty())
  {
    std::cerr << G_STRFUNC << ": field_name is empty" << std::endl;
    return Gnome::Gda::Value();
  }
  
  const Gnome::Gda::Value result = DbUtils::auto_increment_insert_first_if_necessary(table_name, field_name);
  double num_result = Conversions::get_double_for_gda_value_numeric(result);

  //Increment the next_value:
  ++num_result;
  const Gnome::Gda::Value next_value = Conversions::parse_value(num_result);

  Glib::RefPtr<Gnome::Gda::SqlBuilder> builder = Gnome::Gda::SqlBuilder::create(Gnome::Gda::SQL_STATEMENT_UPDATE);
  builder->set_table(GLOM_STANDARD_TABLE_AUTOINCREMENTS_TABLE_NAME);
  builder->add_field_value_as_value(GLOM_STANDARD_TABLE_AUTOINCREMENTS_FIELD_NEXT_VALUE, next_value);
  builder_set_where_autoincrement(builder, table_name, field_name);

  const bool test = query_execute(builder);
  if(!test)
    std::cerr << G_STRFUNC << ": Increment failed." << std::endl;

  return result;
}

Gnome::Gda::Value auto_increment_insert_first_if_necessary(const Glib::ustring& table_name, const Glib::ustring& field_name)
{
  if(table_name.empty())
  {
    std::cerr << G_STRFUNC << ": table_name is empty" << std::endl;
    return Gnome::Gda::Value();
  }
  
  if(field_name.empty())
  {
    std::cerr << G_STRFUNC << ": field_name is empty" << std::endl;
    return Gnome::Gda::Value();
  }

  Gnome::Gda::Value value;

  //Check that the user is allowd to view and edit this table:
  Privileges table_privs = Privs::get_current_privs(GLOM_STANDARD_TABLE_AUTOINCREMENTS_TABLE_NAME);
  if(!table_privs.m_view || !table_privs.m_edit)
  {
    //This should not happen:
    std::cerr << G_STRFUNC << ": The current user may not edit the autoincrements table. Any user who has create rights for a table should have edit rights to the autoincrements table." << std::endl;
  }

  Glib::RefPtr<Gnome::Gda::SqlBuilder> builder =
    Gnome::Gda::SqlBuilder::create(Gnome::Gda::SQL_STATEMENT_SELECT);
  builder->select_add_field("next_value", GLOM_STANDARD_TABLE_AUTOINCREMENTS_TABLE_NAME);
  builder->select_add_target(GLOM_STANDARD_TABLE_AUTOINCREMENTS_TABLE_NAME);
  builder_set_where_autoincrement(builder, table_name, field_name);

  const Glib::RefPtr<const Gnome::Gda::DataModel> datamodel = query_execute_select(builder);
  if(!datamodel || (datamodel->get_n_rows() == 0))
  {
    //Start with zero:

    //Insert the row if it's not there.
    builder = Gnome::Gda::SqlBuilder::create(Gnome::Gda::SQL_STATEMENT_INSERT);
    builder->set_table(GLOM_STANDARD_TABLE_AUTOINCREMENTS_TABLE_NAME);
    builder->add_field_value(GLOM_STANDARD_TABLE_AUTOINCREMENTS_FIELD_TABLE_NAME, table_name);
    builder->add_field_value(GLOM_STANDARD_TABLE_AUTOINCREMENTS_FIELD_FIELD_NAME, field_name);
    builder->add_field_value(GLOM_STANDARD_TABLE_AUTOINCREMENTS_FIELD_NEXT_VALUE, 0);

    const bool test = query_execute(builder);
    if(!test)
      std::cerr << G_STRFUNC << ": INSERT of new row failed." << std::endl;

    //GdaNumeric is a pain, so we take a short-cut:
    bool success = false;
    value = Conversions::parse_value(Field::TYPE_NUMERIC, "0", success, true /* iso_format */);
  }
  else
  {
    //Return the value so that a calling function does not need to do a second SELECT.
    const Gnome::Gda::Value actual_value = datamodel->get_value_at(0, 0);
    //But the caller wants a numeric value not a text value
    //(our system_autoincrements table has it as text, for future flexibility):
    const std::string actual_value_text = actual_value.get_string();
    bool success = false;
    value = Conversions::parse_value(Field::TYPE_NUMERIC, actual_value_text, success, true /* iso_format */);
  }

  //std::cout << "auto_increment_insert_first_if_necessary: returning value of type=" << value.get_value_type() << std::endl;
  return value;
}

/** Set the next auto-increment value in the glom system table, by examining all current values.
 * Use this, for instance, after importing rows.
 * Add a row for this field in the system table if it does not exist already.
 */
static void recalculate_next_auto_increment_value(const Glib::ustring& table_name, const Glib::ustring& field_name)
{
  if(table_name.empty())
  {
    std::cerr << G_STRFUNC << ": table_name is empty" << std::endl;
    return;
  }
  
  if(field_name.empty())
  {
    std::cerr << G_STRFUNC << ": field_name is empty" << std::endl;
    return;
  }

  //Make sure that the row exists in the glom system table:
  auto_increment_insert_first_if_necessary(table_name, field_name);

  //Get the max key value in the database:
  Glib::RefPtr<Gnome::Gda::SqlBuilder> builder = Gnome::Gda::SqlBuilder::create(Gnome::Gda::SQL_STATEMENT_SELECT);
  std::vector<guint> args;
  args.push_back(builder->add_field_id(field_name, table_name));
  builder->add_field_value_id(builder->add_function("MAX", args));
  builder->select_add_target(table_name);

  Glib::RefPtr<Gnome::Gda::DataModel> datamodel = query_execute_select(builder);
  if(datamodel && datamodel->get_n_rows() && datamodel->get_n_columns())
  {
    //Increment it:
    const Gnome::Gda::Value value_max = datamodel->get_value_at(0, 0); // A GdaNumeric.
    double num_max = Conversions::get_double_for_gda_value_numeric(value_max);
    ++num_max;

    //Set it in the glom system table:
    const Gnome::Gda::Value next_value = Conversions::parse_value(num_max);

    builder.reset();
    builder = Gnome::Gda::SqlBuilder::create(Gnome::Gda::SQL_STATEMENT_UPDATE);
    builder->set_table(GLOM_STANDARD_TABLE_AUTOINCREMENTS_TABLE_NAME);
    builder->add_field_value_as_value(GLOM_STANDARD_TABLE_AUTOINCREMENTS_FIELD_NEXT_VALUE, next_value);
    builder_set_where_autoincrement(builder, table_name, field_name);

    const bool test = query_execute(builder);
    if(!test)
      std::cerr << G_STRFUNC << ": UPDATE failed." << std::endl;
  }
  else
    std::cerr << G_STRFUNC << ": SELECT MAX() failed." << std::endl;
}

void remove_auto_increment(const Glib::ustring& table_name, const Glib::ustring& field_name)
{
  if(table_name.empty())
  {
    std::cerr << G_STRFUNC << ": table_name is empty" << std::endl;
    return;
  }
  
  if(field_name.empty())
  {
    std::cerr << G_STRFUNC << ": field_name is empty" << std::endl;
    return;
  }
  
  Glib::RefPtr<Gnome::Gda::SqlBuilder> builder =
    Gnome::Gda::SqlBuilder::create(Gnome::Gda::SQL_STATEMENT_DELETE);
  builder->set_table(GLOM_STANDARD_TABLE_AUTOINCREMENTS_TABLE_NAME);
  builder_set_where_autoincrement(builder, table_name, field_name);

  const bool test = query_execute(builder);
  if(!test)
    std::cerr << G_STRFUNC << ": UPDATE failed." << std::endl;
}

bool insert_example_data(Document* document, const Glib::ustring& table_name)
{
  //TODO_Performance: Avoid copying:
  const Document::type_example_rows example_rows = document->get_table_example_data(table_name);
  if(example_rows.empty())
  {
    //std::cout << "debug: " << G_STRFUNC << ": No example data available." << std::endl;
    return true;
  }

  Glib::RefPtr<Gnome::Gda::Connection> gda_connection = get_connection();
  if(!gda_connection)
  {
    std::cerr << G_STRFUNC << ": connection is null" << std::endl;
    return false;
  }

  //std::cout << "debug: inserting example_rows for table: " << table_name << std::endl;

  bool insert_succeeded = true;


  //Get field names:
  Document::type_vec_fields vec_fields = document->get_table_fields(table_name);

  //Actually insert the data:
  //std::cout << "debug: " << G_STRFUNC << ": number of rows of data: " << vec_rows.size() << std::endl;

  //std::cout << "DEBUG: example_row size = " << example_rows.size() << std::endl;

  for(Document::type_example_rows::const_iterator iter = example_rows.begin(); iter != example_rows.end(); ++iter)
  {
    //Check that the row contains the correct number of columns.
    //This check will slow this down, but it seems useful:
    //TODO: This can only work if we can distinguish , inside "" and , outside "":
    const Document::type_row_data& row_data = *iter;
    if(row_data.empty())
      break;

    //std::cout << "DEBUG: row_data size = " << row_data.size() << ", (fields size= " << vec_fields.size() << " )" << std::endl;

    Glib::RefPtr<Gnome::Gda::SqlBuilder> builder = Gnome::Gda::SqlBuilder::create(Gnome::Gda::SQL_STATEMENT_INSERT);
    builder->set_table(table_name);
    for(unsigned int i = 0; i < row_data.size(); ++i) //TODO_Performance: Avoid calling size() so much.
    {
      //std::cout << "  DEBUG: i=" << i << ", row_data.size()=" << row_data.size() << std::endl;

      sharedptr<Field> field = vec_fields[i];
      if(!field)
      {
        std::cerr << G_STRFUNC << ": field was null for field num=" << i << std::endl;
        break;
      }

      builder->add_field_value(field->get_name(), row_data[i]);
    }

    //Create and parse the SQL query:
    //After this, the Parser will know how many SQL parameters there are in
    //the query, and allow us to set their values.

    insert_succeeded = query_execute(builder);
    if(!insert_succeeded)
      break;
  }

  for(Document::type_vec_fields::const_iterator iter = vec_fields.begin(); iter != vec_fields.end(); ++iter)
  {
    if((*iter)->get_auto_increment())
      recalculate_next_auto_increment_value(table_name, (*iter)->get_name());
  }

  return insert_succeeded;
}

//static:
Glib::RefPtr<Gnome::Gda::DataModel> query_execute_select(const Glib::RefPtr<const Gnome::Gda::SqlBuilder>& builder, bool use_cursor)
{
  Glib::RefPtr<Gnome::Gda::DataModel> result;

  //TODO: BusyCursor busy_cursor(get_app_window());

  Glib::RefPtr<Gnome::Gda::Connection> gda_connection = get_connection();
  if(!gda_connection)
  {
    std::cerr << G_STRFUNC << ": No connection yet." << std::endl;
    return result;
  }

  //Debug output:
  if(builder && ConnectionPool::get_instance()->get_show_debug_output())
  {
    const std::string full_query = Utils::sqlbuilder_get_full_query(builder);
    std::cout << "debug: " << G_STRFUNC << ":  " << full_query << std::endl;
  }

  //TODO: Use DbUtils::query_execute().
  try
  {
    if(use_cursor)
    {
      //Specify the STATEMENT_MODEL_CURSOR, so that libgda only gets the rows that we actually use.
      result = gda_connection->statement_execute_select_builder(builder, Gnome::Gda::STATEMENT_MODEL_CURSOR_FORWARD);
    }
    else
      result = gda_connection->statement_execute_select_builder(builder);
  }
  catch(const Gnome::Gda::ConnectionError& ex)
  {
    std::cerr << G_STRFUNC << ": " << ex.what() << std::endl;
  }
  catch(const Gnome::Gda::ServerProviderError& ex)
  {
    if(ex.code() == Gnome::Gda::ServerProviderError::SERVER_PROVIDER_STATEMENT_EXEC_ERROR)
    {
      std::cerr << G_STRFUNC << ": code=SERVER_PROVIDER_STATEMENT_EXEC_ERROR, message=" << ex.what() << std::endl;
    }
    else
    {
      std::cerr << G_STRFUNC << ": code=" << ex.code() << "message=" << ex.what() << std::endl;
    }
  }
  catch(const Gnome::Gda::SqlError& ex) //TODO: Make sure that statement_execute_select_builder() is documented as throwing this.
  {
    std::cerr << G_STRFUNC << ": " << ex.what() << std::endl;
  }
  catch(const Glib::Error& ex)
  {
    std::cerr << G_STRFUNC << ": " << ex.what() << std::endl;
  }


  if(!result)
  {
    const std::string full_query = Utils::sqlbuilder_get_full_query(builder);
    std::cerr << G_STRFUNC << ": Error while executing SQL: "
      << std::endl << "  " << full_query << std::endl << std::endl;
    handle_error();
  }

  return result;
}

bool query_execute_string(const Glib::ustring& strQuery, const Glib::RefPtr<Gnome::Gda::Set>& params)
{
  Glib::RefPtr<Gnome::Gda::Connection> gda_connection = get_connection();
  if(!gda_connection)
  {
    std::cerr << G_STRFUNC << ": No connection yet." << std::endl;
    return false;
  }

  Glib::RefPtr<Gnome::Gda::SqlParser> parser = gda_connection->create_parser();
  Glib::RefPtr<Gnome::Gda::Statement> stmt;
  try
  {
    stmt = parser->parse_string(strQuery);
  }
  catch(const Gnome::Gda::SqlParserError& error)
  {
    std::cerr << G_STRFUNC << ":  SqlParserError: " << error.what() << std::endl;
    return false;
  }


  //Debug output:
  if(stmt && ConnectionPool::get_instance()->get_show_debug_output())
  {
    try
    {
      //TODO: full_query still seems to contain ## parameter names,
      //though it works for our SELECT queries in query_execute_select():
      const Glib::ustring full_query = stmt->to_sql(params);
      std::cerr << G_STRFUNC << ": " << full_query << std::endl;
    }
    catch(const Glib::Exception& ex)
    {
      std::cerr << G_STRFUNC << ": Debug: query string could not be converted to std::cout: " << ex.what() << std::endl;
    }
  }


  int exec_retval = -1;
  try
  {
    exec_retval = gda_connection->statement_execute_non_select(stmt, params);
  }
  catch(const Glib::Error& error)
  {
    std::cerr << G_STRFUNC << ":  ConnectionError: " << error.what() << std::endl;
    const Glib::ustring full_query = stmt->to_sql(params);
    std::cerr << "  full_query: " << full_query << std::endl;
    return false;
  }
  
  //Note that only -1 means an error, not all negative values.
  //For instance, it can return -2 for a successful CREATE TABLE query, to mean that the backend (SQLite) does not report how many rows were affected.
  if(exec_retval == -1)
  {
    const Glib::ustring full_query = stmt->to_sql(params);
    std::cerr << G_STRFUNC << "Gnome::Gda::Connection::statement_execute_non_select() failed with SQL: " << full_query << std::endl;
    return false;
  }
  
  return true;
}

bool query_execute(const Glib::RefPtr<const Gnome::Gda::SqlBuilder>& builder)
{
  Glib::RefPtr<Gnome::Gda::Connection> gda_connection = get_connection();
  if(!gda_connection)
  {
    std::cerr << G_STRFUNC << ": No connection yet." << std::endl;
    return false;
  }

  //Debug output:
  if(builder && ConnectionPool::get_instance()->get_show_debug_output())
  {
    const std::string full_query = Utils::sqlbuilder_get_full_query(builder);
    std::cerr << G_STRFUNC << ": " << full_query << std::endl;
  }


  int exec_retval = -1;
  try
  {
    exec_retval = gda_connection->statement_execute_non_select_builder(builder);
  }
  catch(const Gnome::Gda::ConnectionError& ex)
  {
    std::cerr << G_STRFUNC << ": " << ex.what() << std::endl;
    const std::string full_query = Utils::sqlbuilder_get_full_query(builder);
    std::cerr << "  full_query: " << full_query << std::endl;
    return false;
  }
  catch(const Gnome::Gda::ServerProviderError& ex)
  {
    std::cerr << G_STRFUNC << ": code=" << ex.code() << "message=" << ex.what() << std::endl;
    const std::string full_query = Utils::sqlbuilder_get_full_query(builder);
    std::cerr << "  full_query: " << full_query << std::endl;
    return false;
  }
  catch(const Gnome::Gda::SqlError& ex) //TODO: Make sure that statement_execute_non_select_builder() is documented as throwing this.
  {
    std::cerr << G_STRFUNC << ": " << ex.what() << std::endl;
    const std::string full_query = Utils::sqlbuilder_get_full_query(builder);
    std::cerr << "  full_query: " << full_query << std::endl;
    return false;
  }
  catch(const Glib::Error& ex)
  {
    std::cerr << G_STRFUNC << ": " << ex.what() << std::endl;
    return false;
  }
  return (exec_retval >= 0);
}

void layout_item_fill_field_details(Document* document, const Glib::ustring& parent_table_name, sharedptr<LayoutItem_Field>& layout_item)
{
  if(!document)
  {
    std::cerr << G_STRFUNC << ": document was null." << std::endl;
    return;
  }

  if(!layout_item)
  {
    std::cerr << G_STRFUNC << ": layout_item was null." << std::endl;
  }

  const Glib::ustring table_name = layout_item->get_table_used(parent_table_name);
  layout_item->set_full_field_details( document->get_field(table_name, layout_item->get_name()) );
}

bool layout_field_should_have_navigation(const Glib::ustring& table_name, const sharedptr<const LayoutItem_Field>& layout_item, const Document* document, sharedptr<Relationship>& field_used_in_relationship_to_one)
{
  //Initialize output parameter:
  field_used_in_relationship_to_one = sharedptr<Relationship>();
  
  if(!document)
  {
    std::cerr << G_STRFUNC << ": document was null." << std::endl;
    return false;
  }
  
  if(table_name.empty())
  {
    std::cerr << G_STRFUNC << ": table_name was empty." << std::endl;
    return false;
  } 
  
  if(!layout_item)
  {
    std::cerr << G_STRFUNC << ": layout_item was null." << std::endl;
    return false;
  }

  //Check whether the field controls a relationship,
  //meaning it identifies a record in another table.
  sharedptr<const Relationship> const_relationship =
    document->get_field_used_in_relationship_to_one(table_name, layout_item);
  field_used_in_relationship_to_one = sharedptr<Relationship>::cast_const(const_relationship); //This is just because we can't seem to have a sharedptr<const Relationship>& output parameter.
  // std::cout << "DEBUG: table_name=" << table_name << ", table_used=" << layout_item->get_table_used(table_name) << ", layout_item=" << layout_item->get_name() << ", field_used_in_relationship_to_one=" << field_used_in_relationship_to_one << std::endl;

  //Check whether the field identifies a record in another table
  //just because it is a primary key in that table:
  const sharedptr<const Field> field_info = layout_item->get_full_field_details();
  const bool field_is_related_primary_key =
    layout_item->get_has_relationship_name() &&
    field_info && field_info->get_primary_key();
  // std::cout <<   "DEBUG: layout_item->get_has_relationship_name()=" << layout_item->get_has_relationship_name() << ", field_info->get_primary_key()=" <<  field_info->get_primary_key() << ", field_is_related_primary_key=" << field_is_related_primary_key << std::endl;

  return field_used_in_relationship_to_one || field_is_related_primary_key;
}

Glib::ustring get_unused_database_name(const Glib::ustring& base_name)
{ 
  Glom::ConnectionPool* connection_pool = Glom::ConnectionPool::get_instance();
  if(!connection_pool)
    return Glib::ustring();

  bool keep_trying = true;
  size_t extra_num = 0;
  while(keep_trying)
  {
    Glib::ustring database_name_possible;
    if(extra_num == 0)
    {
      //Try the original name first,
      //removing any characters that are likely to cause problems when used in a SQL identifier name:
      database_name_possible = Utils::trim_whitespace(base_name);
      database_name_possible = Utils::string_replace(database_name_possible, "\"", "");
      database_name_possible = Utils::string_replace(database_name_possible, "'", "");
      database_name_possible = Utils::string_replace(database_name_possible, "\t", "");
      database_name_possible = Utils::string_replace(database_name_possible, "\n", "");
    }
    else
    {
      //Create a new database name by appending a number to the original name:
      const Glib::ustring pchExtraNum = Glib::ustring::compose("%1", extra_num);
      database_name_possible = (base_name + pchExtraNum);
    }
    ++extra_num;
    
    connection_pool->set_database(database_name_possible);
    connection_pool->set_ready_to_connect();

    Glom::sharedptr<Glom::SharedConnection> connection;

    try
    {
      connection = ConnectionPool::get_and_connect();
    }
    catch(const ExceptionConnection& ex)
    {
      if(ex.get_failure_type() == ExceptionConnection::FAILURE_NO_SERVER)
      {
        //We couldn't even connect to the server,
        //regardless of what database we try to connect to:
        std::cerr << G_STRFUNC << ": Could not connect to the server." << std::endl;
        return Glib::ustring();
      }
      else
      {
        //We assume that the connection failed because the database does not exist.
        //std::cout << "debug: " << G_STRFUNC << ": unused database name successfully found: " << database_name_possible << std::endl;
        return database_name_possible;
      }
    }
  }
  
  return Glib::ustring();    
}

int count_rows_returned_by(const Glib::RefPtr<const Gnome::Gda::SqlBuilder>& sql_query)
{
  if(!sql_query)
  {
    std::cerr << G_STRFUNC << ": sql_query was null." << std::endl;
    return 0;
  }

  const Glib::RefPtr<const Gnome::Gda::SqlBuilder> builder =
    Utils::build_sql_select_count_rows(sql_query);

  int result = 0;

  Glib::RefPtr<Gnome::Gda::DataModel> datamodel = DbUtils::query_execute_select(builder);
  if(datamodel && datamodel->get_n_rows() && datamodel->get_n_columns())
  {
    const Gnome::Gda::Value value = datamodel->get_value_at(0, 0);
    //This showed me that this contains a gint64: std::cerr << "DEBUG: value type=" << G_VALUE_TYPE_NAME(value.gobj()) << std::endl;
    //For sqlite, this is an integer
    if(value.get_value_type() == G_TYPE_INT64) //With the PostgreSQL backend.
      result = (int)value.get_int64();
    else if(value.get_value_type() == G_TYPE_INT) //With the PostgreSQL backend.
    {
      result = value.get_int(); //With the SQLite backend.
    }
    else
    {
      std::cerr << G_STRFUNC << ": The COUNT query returned an unexpected value type: " << g_type_name(value.get_value_type()) << std::endl;
      result = -1;
    }
  }

  //std::cout << "debug: " << G_STRFUNC << ": Returning " << result << std::endl;
  return result;
}

bool rename_table(const Glib::ustring& table_name, const Glib::ustring& new_table_name)
{
  //TODO: Escape the table names:
  return query_execute_string( "ALTER TABLE " + escape_sql_id(table_name) + " RENAME TO " + escape_sql_id(new_table_name));
}

bool drop_table(const Glib::ustring& table_name)
{
  //TODO: Escape the table names:
  return DbUtils::query_execute_string( "DROP TABLE " + escape_sql_id(table_name));
}

Glib::ustring escape_sql_id(const Glib::ustring& id)
{
  if(id.empty())
  {
    std::cerr << G_STRFUNC << ": id is empty." << std::endl;
    return id;
  }

  Glib::RefPtr<Gnome::Gda::Connection> gda_connection = get_connection();
  if(!gda_connection)
  {
    std::cerr << G_STRFUNC << ": No gda_connection." << std::endl;
    return id;
  }

  //Always put it in quotes even if 

  return gda_connection->quote_sql_identifier(id);
}

Glib::ustring gda_cnc_string_encode(const Glib::ustring& str)
{
  char* pch = gda_rfc1738_encode(str.c_str());
  if(!pch)
    return Glib::ustring();
  else
    return Glib::ustring(pch);
}

Glib::ustring build_query_create_group(const Glib::ustring& group, bool superuser)
{
  if(group.empty())
  {
    std::cerr << G_STRFUNC << ": group is empty" << std::endl;
  }

  Glib::ustring query = "CREATE GROUP " + escape_sql_id(group);

  //The "SUPERUSER" here has no effect because SUPERUSER is not "inherited" to member users.
  //But let's keep it to make the purpose of this group obvious.
  if(superuser)
    query += " WITH SUPERUSER";

  return query;
}

Glib::ustring build_query_add_user_to_group(const Glib::ustring& group, const Glib::ustring& user)
{
  if(group.empty())
  {
    std::cerr << G_STRFUNC << ": group is empty" << std::endl;
  }

  if(user.empty())
  {
    std::cerr << G_STRFUNC << ": user is empty" << std::endl;
  }

  return "ALTER GROUP " + escape_sql_id(group) + " ADD USER " + escape_sql_id(user);
}

static Glib::ustring build_query_add_user(const Glib::ustring& user, const Glib::ustring& password, bool superuser)
{
  if(user.empty())
  {
    std::cerr << G_STRFUNC << ": user is empty" << std::endl;
  }

  if(password.empty())
  {
    std::cerr << G_STRFUNC << ": password is empty" << std::endl;
  }

  //Note that ' around the user fails, so we use ".
  Glib::ustring strQuery = "CREATE USER " + DbUtils::escape_sql_id(user) + " PASSWORD '" + password + "'" ; //TODO: Escape the password.
  if(superuser)
    strQuery += " SUPERUSER CREATEDB CREATEROLE"; //Because SUPERUSER is not "inherited" from groups to members.

  return strQuery;
}

bool add_user(const Document* document, const Glib::ustring& user, const Glib::ustring& password, const Glib::ustring& group)
{
  if(!document)
  {
    std::cerr << G_STRFUNC << ": document is null." << std::endl;
    return false;
  }

  if(user.empty())
  {
    std::cerr << G_STRFUNC << ": user is empty." << std::endl;
    return false;
  }

  if(password.empty())
  {
    std::cerr << G_STRFUNC << ": password is  empty." << std::endl;
    return false;
  }

  if(group.empty())
  {
    std::cerr << G_STRFUNC << ": group is empty." << std::endl;
    return false;
  }

  //Create the user:
  const bool superuser = (group == GLOM_STANDARD_GROUP_NAME_DEVELOPER);
  const Glib::ustring query_add = build_query_add_user(user, password, superuser);
  bool test = DbUtils::query_execute_string(query_add);
  if(!test)
  {
    std::cerr << G_STRFUNC << ": CREATE USER failed." << std::endl;
    return false;
  }

  //Add it to the group:
  const Glib::ustring query_add_to_group = build_query_add_user_to_group(group, user);
  test = DbUtils::query_execute_string(query_add_to_group);
  if(!test)
  {
    std::cerr << G_STRFUNC << ": ALTER GROUP failed." << std::endl;
    return false;
  }

  //Remove any user rights, so that all rights come from the user's presence in the group:
  Document::type_listTableInfo table_list = document->get_tables();

  for(Document::type_listTableInfo::const_iterator iter = table_list.begin(); iter != table_list.end(); ++iter)
  {
    const Glib::ustring table_name = (*iter)->get_name();
    const Glib::ustring strQuery = "REVOKE ALL PRIVILEGES ON " + DbUtils::escape_sql_id(table_name) + " FROM " + DbUtils::escape_sql_id(user);
    const bool test = DbUtils::query_execute_string(strQuery);
    if(!test)
      std::cerr << G_STRFUNC << ": REVOKE failed." << std::endl;
  }

  return true;
}

bool add_group(const Document* document, const Glib::ustring& group)
{
  if(!document)
  {
    std::cerr << G_STRFUNC << ": document is null." << std::endl;
    return false;
  }

  if(group.empty())
  {
    std::cerr << G_STRFUNC << ": group is empty." << std::endl;
    return false;
  }
 
  const Glib::ustring strQuery = DbUtils::build_query_create_group(group);
  //std::cout << "DEBUGCREATE: " << strQuery << std::endl;
  const bool test = DbUtils::query_execute_string(strQuery);
  if(!test)
  {
    std::cerr << G_STRFUNC << ": CREATE GROUP failed." << std::endl;
    return false;
  }

  //Give the new group some sensible default privileges:
  Privileges priv;
  priv.m_view = true;
  priv.m_edit = true;

  Document::type_listTableInfo table_list = document->get_tables(true /* plus system prefs */);
  for(Document::type_listTableInfo::const_iterator iter = table_list.begin(); iter != table_list.end(); ++iter)
  {
    if(!Privs::set_table_privileges(group, (*iter)->get_name(), priv))
    {
      std::cerr << G_STRFUNC << "Privs::set_table_privileges() failed." << std::endl;
      return false;
    }
  }

  //Let them edit the autoincrements too:
  if(!Privs::set_table_privileges(group, GLOM_STANDARD_TABLE_AUTOINCREMENTS_TABLE_NAME, priv))
  {
    std::cerr << G_STRFUNC << "Privs::set_table_privileges() failed." << std::endl;
    return false;
  }

  return true;
}

bool remove_user(const Glib::ustring& user)
{
  if(user.empty())
    return false;

  const Glib::ustring strQuery = "DROP USER " + DbUtils::escape_sql_id(user);
  const bool test = DbUtils::query_execute_string(strQuery);
  if(!test)
  {
    std::cerr << G_STRFUNC << ": DROP USER failed" << std::endl;
    return false;
  }

  return true;
}

bool remove_user_from_group(const Glib::ustring& user, const Glib::ustring& group)
{
  if(user.empty() || group.empty())
    return false;

  const Glib::ustring strQuery = "ALTER GROUP " + DbUtils::escape_sql_id(group) + " DROP USER " + DbUtils::escape_sql_id(user);
  const bool test = DbUtils::query_execute_string(strQuery);
  if(!test)
  {
    std::cerr << G_STRFUNC << ": ALTER GROUP failed." << std::endl;
    return false;
  }

  return true;
}

void set_fake_connection()
{
  //Allow a fake connection, so sqlbuilder_get_full_query() can work:
  Glom::ConnectionPool* connection_pool = Glom::ConnectionPool::get_instance();
  Glom::ConnectionPoolBackends::Backend* backend = 
    new Glom::ConnectionPoolBackends::PostgresCentralHosted();
  connection_pool->set_backend(std::auto_ptr<Glom::ConnectionPool::Backend>(backend));
  connection_pool->set_fake_connection();
}

} //namespace DbUtils

} //namespace Glom
