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

#include "config.h"
#include "base_db_table_data.h"
#include <libglom/data_structure/glomconversions.h>
#include <glom/application.h>
#include <glom/python_embed/glom_python.h>
#include <glom/utils_ui.h>
#include <libglom/db_utils.h>
#include <sstream>
#include <glibmm/i18n.h>

namespace Glom
{

Base_DB_Table_Data::Base_DB_Table_Data()
{
}

Base_DB_Table_Data::~Base_DB_Table_Data()
{
}

Gnome::Gda::Value Base_DB_Table_Data::get_entered_field_data_field_only(const sharedptr<const Field>& field) const
{
  sharedptr<LayoutItem_Field> layout_item = sharedptr<LayoutItem_Field>::create();
  layout_item->set_full_field_details(field);

  return get_entered_field_data(layout_item);
}

Gnome::Gda::Value Base_DB_Table_Data::get_entered_field_data(const sharedptr<const LayoutItem_Field>& /* field */) const
{
  //Override this to use Field::set_data() too.

  return Gnome::Gda::Value(); //null
}


Gtk::TreeModel::iterator Base_DB_Table_Data::get_row_selected()
{
  //This in meaningless for Details,
  //but is overridden for list views.
  return Gtk::TreeModel::iterator();
}


bool Base_DB_Table_Data::record_new(bool use_entered_data, const Gnome::Gda::Value& primary_key_value)
{
  sharedptr<const Field> fieldPrimaryKey = get_field_primary_key();

  const Glib::ustring primary_key_name = fieldPrimaryKey->get_name();

  type_vecLayoutFields fieldsToAdd = m_FieldsShown;
  if(m_TableFields.empty())
    m_TableFields = get_fields_for_table(m_table_name);

  //Add values for all fields, not just the shown ones:
  //For instance, we must always add the primary key, and fields with default/calculated/lookup values:
  for(type_vec_fields::const_iterator iter = m_TableFields.begin(); iter != m_TableFields.end(); ++iter)
  {
    //TODO: Search for the non-related field with the name, not just the field with the name:
    type_vecLayoutFields::const_iterator iterFind = std::find_if(fieldsToAdd.begin(), fieldsToAdd.end(), predicate_FieldHasName<LayoutItem_Field>((*iter)->get_name()));
    if(iterFind == fieldsToAdd.end())
    {
      sharedptr<LayoutItem_Field> layout_item = sharedptr<LayoutItem_Field>::create();
      layout_item->set_full_field_details(*iter);

      fieldsToAdd.push_back(layout_item);
    }
  }

  Document* document = get_document();

  //Calculate any necessary field values and enter them:
  for(type_vecLayoutFields::const_iterator iter = fieldsToAdd.begin(); iter != fieldsToAdd.end(); ++iter)
  {
    sharedptr<LayoutItem_Field> layout_item = *iter;

    //If the user did not enter something in this field:
    Gnome::Gda::Value value = get_entered_field_data(layout_item);

    if(Conversions::value_is_empty(value)) //This deals with empty strings too.
    {
      const sharedptr<const Field>& field = layout_item->get_full_field_details();
      if(field)
      {
        //If the default value should be calculated, then calculate it:
        if(field->get_has_calculation())
        {
          const Glib::ustring calculation = field->get_calculation();
          const type_map_fields field_values = get_record_field_values_for_calculation(m_table_name, fieldPrimaryKey, primary_key_value);

          //We need the connection when we run the script, so that the script may use it.
          // TODO: Is this function supposed to throw an exception?
          sharedptr<SharedConnection> sharedconnection = connect_to_server(Application::get_application());

            Glib::ustring error_message; //TODO: Check this.
            const Gnome::Gda::Value value =
              glom_evaluate_python_function_implementation(
                field->get_glom_type(),
                calculation,
                field_values,
                document,
                m_table_name,
                fieldPrimaryKey, primary_key_value,
                sharedconnection->get_gda_connection(),
                error_message);
            set_entered_field_data(layout_item, value);
        }

        //Use default values (These are also specified in postgres as part of the field definition,
        //so we could theoretically just not specify it here.)
        //TODO_Performance: Add has_default_value()?
        if(Conversions::value_is_empty(value))
        {
          const Gnome::Gda::Value default_value = field->get_default_value();
          if(!Conversions::value_is_empty(default_value))
          {
            set_entered_field_data(layout_item, default_value);
          }
        }
      }
    }
  }

  //Get all entered field name/value pairs:
  Glib::RefPtr<Gnome::Gda::SqlBuilder> builder = Gnome::Gda::SqlBuilder::create(Gnome::Gda::SQL_STATEMENT_INSERT);
  builder->set_table(m_table_name);

  //Avoid specifying the same field twice:
  typedef std::map<Glib::ustring, bool> type_map_added;
  type_map_added map_added;
  Glib::RefPtr<Gnome::Gda::Set> params = Gnome::Gda::Set::create();

  for(type_vecLayoutFields::const_iterator iter = fieldsToAdd.begin(); iter != fieldsToAdd.end(); ++iter)
  {
    sharedptr<LayoutItem_Field> layout_item = *iter;
    const Glib::ustring field_name = layout_item->get_name();
    if(!layout_item->get_has_relationship_name()) //TODO: Allow people to add a related record also by entering new data in a related field of the related record.
    {
      type_map_added::const_iterator iterFind = map_added.find(field_name);
      if(iterFind == map_added.end()) //If it was not added already
      {
        Gnome::Gda::Value value;

        const sharedptr<const Field>& field = layout_item->get_full_field_details();
        if(field)
        {
          //Use the specified (generated) primary key value, if there is one:
          if(primary_key_name == field_name && !Conversions::value_is_empty(primary_key_value))
          {
            value = primary_key_value;
          }
          else
          {
            if(use_entered_data)
              value = get_entered_field_data(layout_item);
          }

          /* //TODO: This would be too many small queries when adding one record.
          //Check whether the value meets uniqueness constraints:
          if(field->get_primary_key() || field->get_unique_key())
          {
            if(!get_field_value_is_unique(m_table_name, layout_item, value))
            {
              //Ignore this field value. TODO: Warn the user about it.
            }
          }
          */
          if(!value.is_null())
          {
            builder->add_field_value(field_name, value);
            map_added[field_name] = true;
          }
        }
      }
    }
  }

  //Put it all together to create the record with these field values:
  if(!map_added.empty())
  {
    const bool test = DbUtils::query_execute(builder);
    if(!test)
      std::cerr << G_STRFUNC << ": INSERT failed." << std::endl;
    else
    {
      Gtk::TreeModel::iterator row = get_row_selected(); //Null and ignored for details views.
      set_primary_key_value(row, primary_key_value); //Needed by Box_Data_List::on_adddel_user_changed().

      //Update any lookups, related fields, or calculations:
      for(type_vecLayoutFields::const_iterator iter = fieldsToAdd.begin(); iter != fieldsToAdd.end(); ++iter)
      {
        sharedptr<const LayoutItem_Field> layout_item = *iter;

        //TODO_Performance: We just set this with set_entered_field_data() above. Maybe we could just remember it.
        const Gnome::Gda::Value field_value = get_entered_field_data(layout_item);

        const LayoutFieldInRecord field_in_record(layout_item, m_table_name, fieldPrimaryKey, primary_key_value);

        //Get-and-set values for lookup fields, if this field triggers those relationships:
        do_lookups(field_in_record, row, field_value);

        //Update related fields, if this field is used in the relationship:
        refresh_related_fields(field_in_record, row, field_value);

        //TODO: Put the inserted row into result, somehow? murrayc

        return true; //success
      }
    }
  }
  else
    std::cerr << G_STRFUNC << ": Empty field names or values." << std::endl;

  return false; //Failed.
}

bool Base_DB_Table_Data::add_related_record_for_field(const sharedptr<const LayoutItem_Field>& layout_item_parent,
  const sharedptr<const Relationship>& relationship,
  const sharedptr<const Field>& primary_key_field,
  const Gnome::Gda::Value& primary_key_value_provided,
  Gnome::Gda::Value& primary_key_value_used)
{
  Gnome::Gda::Value primary_key_value = primary_key_value_provided;

  const bool related_record_exists = get_related_record_exists(relationship, primary_key_value);
  if(related_record_exists)
  {
    //No problem, the SQL command below will update this value in the related table.
    primary_key_value_used = primary_key_value; //Let the caller know what related record was created.
    return true;
  }


  //To store the entered data in the related field, we would first have to create a related record.
  if(!relationship->get_auto_create())
  {
    //Warn the user:
    //TODO: Make the field insensitive until it can receive data, so people never see this dialog.
    const Glib::ustring message = _("Data may not be entered into this related field, because the related record does not yet exist, and the relationship does not allow automatic creation of new related records.");
#undef GLOM_ENABLE_MAEMO
#ifdef GLOM_ENABLE_MAEMO
    Hildon::Note dialog(Hildon::NOTE_TYPE_INFORMATION, *Application::get_application(), message);
#else
    Gtk::MessageDialog dialog(Utils::bold_message(_("Related Record Does Not Exist")), true);
    dialog.set_secondary_text(message);
    dialog.set_transient_for(*Application::get_application());
#endif
    dialog.run();

    //Clear the field again, discarding the entered data.
    set_entered_field_data(layout_item_parent, Gnome::Gda::Value());

    return false;
  }
  else
  {
    const bool key_is_auto_increment = primary_key_field->get_auto_increment();

    //If we would have to set an otherwise auto-increment key to add the record.
    if( key_is_auto_increment && !Conversions::value_is_empty(primary_key_value) )
    {
      //Warn the user:
      //TODO: Make the field insensitive until it can receive data, so people never see this dialog.
      const Glib::ustring message = _("Data may not be entered into this related field, because the related record does not yet exist, and the key in the related record is auto-generated and therefore can not be created with the key value in this record.");

#ifdef GLOM_ENABLE_MAEMO
      Hildon::Note dialog(Hildon::NOTE_TYPE_INFORMATION, *Application::get_application(), message);
#else
      Gtk::MessageDialog dialog(Utils::bold_message(_("Related Record Cannot Be Created")), true);
      //TODO: This is a very complex error message:
      dialog.set_secondary_text(message);
      dialog.set_transient_for(*Application::get_application());
#endif
      dialog.run();

      //Clear the field again, discarding the entered data.
      set_entered_field_data(layout_item_parent, Gnome::Gda::Value());

      return false;
    }
    else
    {
      //TODO: Calculate values, and do lookups?

      //Create the related record:
      if(key_is_auto_increment)
      {
        primary_key_value = DbUtils::get_next_auto_increment_value(relationship->get_to_table(), primary_key_field->get_name());

        //Generate the new key value;
      }

      Glib::RefPtr<Gnome::Gda::SqlBuilder> builder = Gnome::Gda::SqlBuilder::create(Gnome::Gda::SQL_STATEMENT_INSERT);
      builder->set_table(relationship->get_to_table());
      builder->add_field_value(primary_key_field->get_name(), primary_key_value);
      const bool test = DbUtils::query_execute(builder);
      if(!test)
      {
        std::cerr << G_STRFUNC << ": INSERT failed." << std::endl;
        return false;
      }

      if(key_is_auto_increment)
      {
        //Set the key in the parent table
        sharedptr<LayoutItem_Field> item_from_key = sharedptr<LayoutItem_Field>::create();
        item_from_key->set_name(relationship->get_from_field());

        //Show the new from key in the parent table's layout:
        set_entered_field_data(item_from_key, primary_key_value);

        //Set it in the database too:
        sharedptr<Field> field_from_key = get_fields_for_table_one_field(relationship->get_from_table(), relationship->get_from_field()); //TODO_Performance.
        if(!field_from_key)
        {
          std::cerr << G_STRFUNC << ": get_fields_for_table_one_field() failed." << std::endl;
          return false;
        }

        sharedptr<Field> parent_primary_key_field = get_field_primary_key();
        if(!parent_primary_key_field)
        {
          g_warning("Base_DB_Table_Data::add_related_record_for_field(): get_field_primary_key() failed. table = %s", get_table_name().c_str());
          return false;
        }
        else
        {
          const Gnome::Gda::Value parent_primary_key_value = get_primary_key_value_selected();
          if(parent_primary_key_value.is_null())
          {
            g_warning("Base_DB_Table_Data::add_related_record_for_field(): get_primary_key_value_selected() failed. table = %s", get_table_name().c_str());
            return false;
          }
          else
          {
            const Glib::ustring target_table = relationship->get_from_table();             
            Glib::RefPtr<Gnome::Gda::SqlBuilder> builder = 
              Gnome::Gda::SqlBuilder::create(Gnome::Gda::SQL_STATEMENT_UPDATE);
            builder->set_table(target_table);
            builder->add_field_value_as_value(relationship->get_from_field(), primary_key_value);
            builder->set_where(
              builder->add_cond(Gnome::Gda::SQL_OPERATOR_TYPE_EQ,
                builder->add_field_id(parent_primary_key_field->get_name(), target_table),
                builder->add_expr(parent_primary_key_value)) );
          
            const bool test = DbUtils::query_execute(builder);
            if(!test)
            {
              std::cerr << G_STRFUNC << ": UPDATE failed." << std::endl;
              return false;
            }
          }
        }
      }

      primary_key_value_used = primary_key_value; //Let the caller know what related record was created.
      return true;
    }
  }
}

void Base_DB_Table_Data::on_record_added(const Gnome::Gda::Value& /* primary_key_value */, const Gtk::TreeModel::iterator& /* row */)
{
   //Overridden by some derived classes.

   signal_record_changed().emit();
}

void Base_DB_Table_Data::on_record_deleted(const Gnome::Gda::Value& /* primary_key_value */)
{
  //Overridden by some derived classes.

  signal_record_changed().emit();
}

bool Base_DB_Table_Data::confirm_delete_record()
{
  //Ask the user for confirmation:
  const Glib::ustring message = _("Are you sure that you would like to delete this record? The data in this record will then be permanently lost.");
#ifdef GLOM_ENABLE_MAEMO
  Hildon::Note dialog(Hildon::NOTE_TYPE_CONFIRMATION_BUTTON, *get_app_window(), message);
#else
  Gtk::MessageDialog dialog(Utils::bold_message(_("Delete record")), true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE);
  dialog.set_secondary_text(message);
  dialog.set_transient_for(*Application::get_application());
#endif
  dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  dialog.add_button(Gtk::Stock::DELETE, Gtk::RESPONSE_OK);

  const int response = dialog.run();
  return (response == Gtk::RESPONSE_OK);
}

bool Base_DB_Table_Data::record_delete(const Gnome::Gda::Value& primary_key_value)
{
  sharedptr<Field> field_primary_key = get_field_primary_key();
  if(field_primary_key && !Conversions::value_is_empty(primary_key_value))
  {
    Glib::RefPtr<Gnome::Gda::SqlBuilder> builder = 
      Gnome::Gda::SqlBuilder::create(Gnome::Gda::SQL_STATEMENT_DELETE);
    builder->set_table(m_table_name);
    builder->set_where(
      builder->add_cond(Gnome::Gda::SQL_OPERATOR_TYPE_EQ,
        builder->add_field_id(field_primary_key->get_name(), m_table_name),
        builder->add_expr(primary_key_value)) );
    return DbUtils::query_execute(builder);
  }
  else
  {
    return false; //no primary key
  }
}

Base_DB_Table_Data::type_signal_record_changed Base_DB_Table_Data::signal_record_changed()
{
  return m_signal_record_changed;
}


bool Base_DB_Table_Data::get_related_record_exists(const sharedptr<const Relationship>& relationship, const Gnome::Gda::Value& key_value)
{
  BusyCursor cursor(Application::get_application());

  bool result = false;

  //Don't try doing a NULL=NULL or ""="" relationship:
  if(Glom::Conversions::value_is_empty(key_value))
    return false;

  //TODO_Performance: It's very possible that this is slow.
  //We don't care how many records there are, only whether there are more than zero.
  const Glib::ustring to_field = relationship->get_to_field();
  const Glib::ustring related_table = relationship->get_to_table();

  //TODO_Performance: Is this the best way to just find out whether there is one record that meets this criteria? 
  Glib::RefPtr<Gnome::Gda::SqlBuilder> builder =
      Gnome::Gda::SqlBuilder::create(Gnome::Gda::SQL_STATEMENT_SELECT);
  builder->select_add_field(to_field, related_table);
  builder->select_add_target(related_table);
  builder->set_where(
    builder->add_cond(Gnome::Gda::SQL_OPERATOR_TYPE_EQ,
      builder->add_field_id(to_field, related_table),
      builder->add_expr(key_value)));
                                               
  Glib::RefPtr<Gnome::Gda::DataModel> records = DbUtils::query_execute_select(builder);
  if(!records)
    handle_error();
  else
  {
    //Field contents:
    if(records->get_n_rows())
      result = true;
  }

  return result;
}


/** Get the shown fields that are in related tables, via a relationship using @a field_name changes.
 */
Base_DB_Table_Data::type_vecLayoutFields Base_DB_Table_Data::get_related_fields(const sharedptr<const LayoutItem_Field>& field) const
{
  type_vecLayoutFields result;

  const Document* document = dynamic_cast<const Document*>(get_document());
  if(document)
  {
    const Glib::ustring field_name = field->get_name(); //At the moment, relationships can not be based on related fields on the from side.
    for(type_vecLayoutFields::const_iterator iter = m_FieldsShown.begin(); iter != m_FieldsShown.end();  ++iter)
    {
      sharedptr<LayoutItem_Field> layout_field = *iter;
      //Examine each field that looks up its data from a relationship:
      if(layout_field->get_has_relationship_name())
      {
        //Get the relationship information:
        sharedptr<const Relationship> relationship = document->get_relationship(m_table_name, layout_field->get_relationship_name());
        if(relationship)
        {
          //If the relationship uses the specified field:
          if(relationship->get_from_field() == field_name)
          {
            //Add it:
            result.push_back(layout_field);
          }
        }
      }
    }
  }

  return result;
}

void Base_DB_Table_Data::refresh_related_fields(const LayoutFieldInRecord& field_in_record_changed, const Gtk::TreeModel::iterator& row, const Gnome::Gda::Value& /* field_value */)
{
  //std::cout << "DEBUG: Base_DB_Table_Data::refresh_related_fields()" << std::endl;

  if(field_in_record_changed.m_table_name != m_table_name)
    return; //TODO: Handle these too?

  //Get values for lookup fields, if this field triggers those relationships:
  //TODO_performance: There is a LOT of iterating and copying here.
  //const Glib::ustring strFieldName = field_in_record_changed.m_field->get_name();
  type_vecLayoutFields fieldsToGet = get_related_fields(field_in_record_changed.m_field);

  if(!fieldsToGet.empty())
  {
    Glib::RefPtr<Gnome::Gda::SqlBuilder> query = Utils::build_sql_select_with_key(field_in_record_changed.m_table_name, fieldsToGet, field_in_record_changed.m_key, field_in_record_changed.m_key_value);
    //std::cout << "debug: " << G_STRFUNC << ": query=" << query << std::endl;

    Glib::RefPtr<const Gnome::Gda::DataModel> result = DbUtils::query_execute_select(query);
    if(!result)
    {
      std::cerr << G_STRFUNC << ": no result." << std::endl;
      handle_error();
    }
    else
    {
      //Field contents:
      if(result->get_n_rows())
      {
        type_vecLayoutFields::const_iterator iterFields = fieldsToGet.begin();

        const guint cols_count = result->get_n_columns();
        if(cols_count <= 0)
        {
          std::cerr << G_STRFUNC << ": The result had 0 columns" << std::endl;
        }

        for(guint uiCol = 0; uiCol < cols_count; ++uiCol)
        {
          const Gnome::Gda::Value value = result->get_value_at(uiCol, 0 /* row */);
          sharedptr<LayoutItem_Field> layout_item = *iterFields;
          if(!layout_item)
            std::cerr << G_STRFUNC << ": The layout_item was null." << std::endl;
          else
          {
            //std::cout << "debug: " << G_STRFUNC << ": field_name=" << layout_item->get_name() << std::endl;
            //std::cout << "debug: " << G_STRFUNC << ": value_as_string=" << value.to_string()  << std::endl;

            //m_AddDel.set_value(row, layout_item, value);
            set_entered_field_data(row, layout_item, value);
            //g_warning("addedel size=%d", m_AddDel.get_count());
          }

          ++iterFields;
        }
      }
      else
        std::cerr << G_STRFUNC << ": no records found." << std::endl;
    }
  }
}

} //namespace Glom
