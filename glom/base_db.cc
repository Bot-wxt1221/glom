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

#include "config.h" // For GLOM_ENABLE_CLIENT_ONLY

#include <glom/base_db.h>
#include "application.h" //Application.
#include <libglom/appstate.h>
#include <libglom/standard_table_prefs_fields.h>
#include <libglom/document/document.h>
#include <libglom/data_structure/glomconversions.h>
#include <glom/mode_design/layout/dialog_choose_field.h>

//#ifndef GLOM_ENABLE_CLIENT_ONLY
#include <glom/mode_design/layout/layout_item_dialogs/dialog_field_layout.h>
#include <glom/mode_design/layout/layout_item_dialogs/dialog_formatting.h>
#include <glom/mode_design/layout/layout_item_dialogs/dialog_notebook.h>
#include <glom/mode_design/layout/layout_item_dialogs/dialog_textobject.h>
#include <glom/mode_design/layout/layout_item_dialogs/dialog_imageobject.h>
//#endif // !GLOM_ENABLE_CLIENT_ONLY

#include <glom/utils_ui.h>
#include <glom/glade_utils.h>
#include <libglom/data_structure/glomconversions.h>
#include <libglom/data_structure/parameternamegenerator.h>
#include <libglom/data_structure/layout/report_parts/layoutitem_summary.h>
#include <libglom/data_structure/layout/report_parts/layoutitem_fieldsummary.h>
#include <libglom/data_structure/layout/report_parts/layoutitem_verticalgroup.h>
#include <libglom/data_structure/layout/report_parts/layoutitem_header.h>
#include <libglom/data_structure/layout/report_parts/layoutitem_footer.h>
#include <glom/python_embed/glom_python.h>
#include <libglom/privs.h>
#include <libglom/db_utils.h>
#include <glibmm/i18n.h>
//#include <libgnomeui/gnome-app-helper.h>

//#include <libgdamm/metastore.h> //For MetaStoreError
//#undef GDA_DISABLE_DEPRECATED
//#include <sql-parser/gda-statement-struct-util.h> //For gda_sql_identifier_remove_quotes().


#include <sstream> //For stringstream

#include <libgda/libgda.h> // gda_g_type_from_string

namespace Glom
{


template<class T_Element>
class predicate_LayoutItemIsEqual
{
public:
  predicate_LayoutItemIsEqual(const sharedptr<const T_Element>& layout_item)
  : m_layout_item(layout_item)
  {
  }

  bool operator() (const sharedptr<const T_Element>& layout_item) const
  {
    if(!m_layout_item && !layout_item)
      return true;

    if(layout_item && m_layout_item)
    {
      return m_layout_item->is_same_field(layout_item);
      //std::cout << "          debug: name1=" << m_layout_item->get_name() << ", name2=" << layout_item->get_name() << ", result=" << result << std::endl;
      //return result;
    }
    else
      return false;
  }

private:
  sharedptr<const T_Element> m_layout_item;
};


Base_DB::Base_DB()
{
  //m_pDocument = 0;
}

Base_DB::~Base_DB()
{
}

bool Base_DB::init_db_details()
{
  return fill_from_database();
}

//TODO: Remove this?
bool Base_DB::fill_from_database()
{
  //m_AddDel.remove_all();
  return true;
}

//static:
sharedptr<SharedConnection> Base_DB::connect_to_server(Gtk::Window* parent_window)
{
  BusyCursor busy_cursor(parent_window);

  return ConnectionPool::get_and_connect();
}

void Base_DB::handle_error(const Glib::Exception& ex)
{
  std::cerr << "Internal Error (Base_DB::handle_error()): exception type=" << typeid(ex).name() << ", ex.what()=" << ex.what() << std::endl;

  Gtk::MessageDialog dialog(Utils::bold_message(_("Internal error")), true, Gtk::MESSAGE_WARNING );
  dialog.set_secondary_text(ex.what());
  //TODO: dialog.set_transient_for(*get_application());
  dialog.run();
}

void Base_DB::handle_error(const std::exception& ex)
{
  std::cerr << "Internal Error (Base_DB::handle_error()): exception type=" << typeid(ex).name() << ", ex.what()=" << ex.what() << std::endl;

  Gtk::MessageDialog dialog(Utils::bold_message(_("Internal error")), true, Gtk::MESSAGE_WARNING );
  dialog.set_secondary_text(ex.what());
  //TODO: dialog.set_transient_for(*get_application());

  dialog.run();
}

bool Base_DB::handle_error()
{
  return ConnectionPool::handle_error_cerr_only();
}

void Base_DB::load_from_document()
{
  if(get_document())
  {
    // TODO: When is it *ever* correct to call fill_from_database() from here?
    if(ConnectionPool::get_instance()->get_ready_to_connect())
      fill_from_database(); //virtual.

    //Call base class:
    View_Composite_Glom::load_from_document();
  }
}

AppState::userlevels Base_DB::get_userlevel() const
{
  const Document* document = dynamic_cast<const Document*>(get_document());
  if(document)
  {
    return document->get_userlevel();
  }
  else
  {
    g_warning("Base_DB::get_userlevel(): document not found.");
    return AppState::USERLEVEL_OPERATOR;
  }
}

void Base_DB::set_userlevel(AppState::userlevels value)
{
  Document* document = get_document();
  if(document)
  {
    document->set_userlevel(value);
  }
}

void Base_DB::on_userlevel_changed(AppState::userlevels /* userlevel */)
{
  //Override this in derived classes.
}



void Base_DB::set_document(Document* pDocument)
{
  View_Composite_Glom::set_document(pDocument);

  //Connect to a signal that is only on the derived document class:
  Document* document = get_document();
  if(document)
  {
    document->signal_userlevel_changed().connect( sigc::mem_fun(*this, &Base_DB::on_userlevel_changed) );

    //Show the appropriate UI for the user level that is specified by this new document:
    on_userlevel_changed(document->get_userlevel());
  }


}

//static:
Base_DB::type_vec_strings Base_DB::util_vecStrings_from_Fields(const type_vec_fields& fields)
{
  //Get vector of field names, suitable for a combo box:

  type_vec_strings vecNames;
  for(type_vec_fields::size_type i = 0; i < fields.size(); ++i)
  {
    vecNames.push_back(fields[i]->get_name());
  }

  return vecNames;
}

Base_DB::type_vec_fields Base_DB::get_fields_for_table(const Glib::ustring& table_name, bool including_system_fields) const
{
  //Get field definitions from the database:
  type_vec_fields fieldsDatabase = DbUtils::get_fields_for_table_from_database(table_name, including_system_fields);

  const Document* pDoc = dynamic_cast<const Document*>(get_document());
  if(!pDoc)
    return fieldsDatabase; //This should never happen.
  else
  {
    type_vec_fields result;

    type_vec_fields fieldsDocument = pDoc->get_table_fields(table_name);

    //Look at each field in the database:
    for(type_vec_fields::iterator iter = fieldsDocument.begin(); iter != fieldsDocument.end(); ++iter)
    {
      sharedptr<Field> field = *iter;
      const Glib::ustring field_name = field->get_name();

      //Get the field info from the database:
      //This is in the document as well, but it _might_ have changed.
      type_vec_fields::const_iterator iterFindDatabase = std::find_if(fieldsDatabase.begin(), fieldsDatabase.end(), predicate_FieldHasName<Field>(field_name));

      if(iterFindDatabase != fieldsDatabase.end() ) //Ignore fields that don't exist in the database anymore.
      {
        Glib::RefPtr<Gnome::Gda::Column> field_info_document = field->get_field_info();

        //Update the Field information that _might_ have changed in the database.
        Glib::RefPtr<Gnome::Gda::Column> field_info = (*iterFindDatabase)->get_field_info();

        //libgda does not tell us whether the field is auto_incremented, so we need to get that from the document.
        field_info->set_auto_increment( field_info_document->get_auto_increment() );

        //libgda does not tell us whether the field is auto_incremented, so we need to get that from the document.
        //TODO_gda:field_info->set_primary_key( field_info_document->get_primary_key() );

        //libgda does yet tell us correct default_value information so we need to get that from the document.
        field_info->set_default_value( field_info_document->get_default_value() );

        field->set_field_info(field_info);

        result.push_back(*iter);
      }
    }

    //Add any fields that are in the database, but not in the document:
    for(type_vec_fields::iterator iter = fieldsDatabase.begin(); iter != fieldsDatabase.end(); ++iter)
    {
      Glib::ustring field_name = (*iter)->get_name();

       //Look in the result so far:
       type_vec_fields::const_iterator iterFind = std::find_if(result.begin(), result.end(), predicate_FieldHasName<Field>(field_name));

       //Add it if it is not there:
       if(iterFind == result.end() )
         result.push_back(*iter);
    }

    return result;
  }
}

#ifndef GLOM_ENABLE_CLIENT_ONLY

namespace
{
  // Check primary key and uniqueness constraints when changing a column
  sharedptr<Field> check_field_change_constraints(const sharedptr<const Field>& field_old, const sharedptr<const Field>& field)
  {
    sharedptr<Field> result = glom_sharedptr_clone(field);
    bool primary_key_was_unset = false;
    if(field_old->get_primary_key() != field->get_primary_key())
    {
      //TODO: Check that there is only one primary key
      //When unsetting a primary key, ask which one should replace it.

      if(field->get_primary_key())
      {
        result->set_unique_key();
      }
      else
      {
        //Make sure the caller knows that a fields stop being unique when it
        //stops being a primary key, because its uniqueness was just a
        //side-effect of it being a primary key.
        result->set_unique_key(false);
        primary_key_was_unset = true;
      }
    }

    if(field_old->get_unique_key() != field->get_unique_key())
    {
      if(!primary_key_was_unset && !field->get_unique_key())
      {
        if(field->get_primary_key())
          result->set_unique_key();
      }
    }

    return result;
  }
}

sharedptr<Field> Base_DB::change_column(const Glib::ustring& table_name, const sharedptr<const Field>& field_old, const sharedptr<const Field>& field, Gtk::Window* /* parent_window */) const
{
  ConnectionPool* connection_pool = ConnectionPool::get_instance();
  sharedptr<Field> result = check_field_change_constraints(field_old, field);

  try
  {
    connection_pool->change_column(table_name, field_old, result);
  }
  catch(const Glib::Error& ex)
  {
    handle_error(ex);
//    Gtk::MessageDialog window(*parent_window, Utils::bold_message(ex.what()), true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
//    window.run();
    return sharedptr<Field>();
  }

  return result;
}

bool Base_DB::change_columns(const Glib::ustring& table_name, const type_vec_const_fields& old_fields, type_vec_fields& fields, Gtk::Window* /* parent_window */) const
{
  g_assert(old_fields.size() == fields.size());

  type_vec_const_fields pass_fields(fields.size());
  for(unsigned int i = 0; i < fields.size(); ++i)
  {
    fields[i] = check_field_change_constraints(old_fields[i], fields[i]);
    pass_fields[i] = fields[i];
  }

  ConnectionPool* connection_pool = ConnectionPool::get_instance();

  try
  {
    connection_pool->change_columns(table_name, old_fields, pass_fields);
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

#endif //GLOM_ENABLE_CLIENT_ONLY

Glib::RefPtr<Gnome::Gda::Connection> Base_DB::get_connection()
{
  sharedptr<SharedConnection> sharedconnection;
  try
  {
     sharedconnection = connect_to_server();
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


#ifndef GLOM_ENABLE_CLIENT_ONLY
sharedptr<LayoutItem_Field> Base_DB::offer_field_list_select_one_field(const Glib::ustring& table_name, Gtk::Window* transient_for)
{
  return offer_field_list_select_one_field(sharedptr<LayoutItem_Field>(), table_name, transient_for);
}

sharedptr<LayoutItem_Field> Base_DB::offer_field_list_select_one_field(const sharedptr<const LayoutItem_Field>& start_field, const Glib::ustring& table_name, Gtk::Window* transient_for)
{
  sharedptr<LayoutItem_Field> result;

  Dialog_ChooseField* dialog = 0;
  Utils::get_glade_widget_derived_with_warning(dialog);
  if(!dialog) //Unlikely and it already warns on stderr.
    return result;

  if(dialog)
  {
    if(transient_for)
      dialog->set_transient_for(*transient_for);

    dialog->set_document(get_document(), table_name, start_field);
    const int response = dialog->run();
    if(response == Gtk::RESPONSE_OK)
    {
      //Get the chosen field:
      result = dialog->get_field_chosen();
    }
    else if(start_field) //Cancel means use the old one:
      result = glom_sharedptr_clone(start_field);

    delete dialog;
  }

  return result;
}

Base_DB::type_list_field_items Base_DB::offer_field_list(const Glib::ustring& table_name, Gtk::Window* transient_for)
{
  type_list_field_items result;

  Dialog_ChooseField* dialog = 0;
  Utils::get_glade_widget_derived_with_warning(dialog);
  if(!dialog) //Unlikely and it already warns on stderr.
    return result;

  if(dialog)
  {
    if(transient_for)
      dialog->set_transient_for(*transient_for);

    dialog->set_document(get_document(), table_name);
    const int response = dialog->run();
    if(response == Gtk::RESPONSE_OK)
    {
      //Get the chosen field:
      result = dialog->get_fields_chosen();
    }

    delete dialog;
  }

  return result;
}

bool Base_DB::offer_non_field_item_formatting(const sharedptr<LayoutItem_WithFormatting>& layout_item, Gtk::Window* transient_for)
{
  bool result = false;

  Dialog_Formatting dialog;
  if(transient_for)
    dialog.set_transient_for(*transient_for);

  add_view(&dialog);

  dialog.set_item(layout_item, false);

  const int response = dialog.run();
  if(response == Gtk::RESPONSE_OK)
  {
    //Get the chosen formatting:
     dialog.use_item_chosen(layout_item);
     result = true;
  }
  //Cancel means use the old one:

  remove_view(&dialog);

  return result;
}

sharedptr<LayoutItem_Field> Base_DB::offer_field_formatting(const sharedptr<const LayoutItem_Field>& start_field, const Glib::ustring& table_name, Gtk::Window* transient_for, bool show_editable_options)
{
  sharedptr<LayoutItem_Field> result;

  Dialog_FieldLayout* dialog = 0;
  Utils::get_glade_widget_derived_with_warning(dialog);
  if(!dialog) //Unlikely and it already warns on stderr.
    return result;

  if(transient_for)
    dialog->set_transient_for(*transient_for);

  add_view(dialog);

  dialog->set_field(start_field, table_name, show_editable_options);

  const int response = dialog->run();
  if(response == Gtk::RESPONSE_OK)
  {
    //Get the chosen field:
    result = dialog->get_field_chosen();
  }
  else if(start_field) //Cancel means use the old one:
    result = glom_sharedptr_clone(start_field);

  remove_view(dialog);
  delete dialog;

  return result;
}

sharedptr<LayoutItem_Text> Base_DB::offer_textobject(const sharedptr<LayoutItem_Text>& start_textobject, Gtk::Window* transient_for, bool show_title)
{
  sharedptr<LayoutItem_Text> result = start_textobject;

  Dialog_TextObject* dialog = 0;
  Utils::get_glade_widget_derived_with_warning(dialog);
  if(!dialog) //Unlikely and it already warns on stderr.
    return result;

  if(transient_for)
    dialog->set_transient_for(*transient_for);

  dialog->set_textobject(start_textobject, Glib::ustring(), show_title);
  const int response = Glom::Utils::dialog_run_with_help(dialog);
  dialog->hide();
  if(response == Gtk::RESPONSE_OK)
  {
    //Get the chosen relationship:
    result = dialog->get_textobject();
  }

  delete dialog;

  return result;
}

sharedptr<LayoutItem_Image> Base_DB::offer_imageobject(const sharedptr<LayoutItem_Image>& start_imageobject, Gtk::Window* transient_for, bool show_title)
{
  sharedptr<LayoutItem_Image> result = start_imageobject;

  Dialog_ImageObject* dialog = 0;
  Utils::get_glade_widget_derived_with_warning(dialog);
  if(!dialog) //Unlikely and it already warns on stderr.
    return result;

  if(transient_for)
    dialog->set_transient_for(*transient_for);

  dialog->set_imageobject(start_imageobject, Glib::ustring(), show_title);
  const int response = Glom::Utils::dialog_run_with_help(dialog);
  dialog->hide();
  if(response == Gtk::RESPONSE_OK)
  {
    //Get the chosen relationship:
    result = dialog->get_imageobject();
  }

  delete dialog;

  return result;
}

sharedptr<LayoutItem_Notebook> Base_DB::offer_notebook(const sharedptr<LayoutItem_Notebook>& start_notebook, Gtk::Window* transient_for)
{
  sharedptr<LayoutItem_Notebook> result = start_notebook;

  Dialog_Notebook* dialog = 0;
  Utils::get_glade_widget_derived_with_warning(dialog);
  if(!dialog) //Unlikely and it already warns on stderr.
    return result;

  if(transient_for)
    dialog->set_transient_for(*transient_for);

  dialog->set_notebook(start_notebook);
  //dialog->set_transient_for(*this);
  const int response = Glom::Utils::dialog_run_with_help(dialog);
  dialog->hide();
  if(response == Gtk::RESPONSE_OK)
  {
    //Get the chosen relationship:
    result = dialog->get_notebook();
  }

  delete dialog;

  return result;
}
#endif // !GLOM_ENABLE_CLIENT_ONLY

sharedptr<Field> Base_DB::get_fields_for_table_one_field(const Glib::ustring& table_name, const Glib::ustring& field_name) const
{
  //Initialize output parameter:
  sharedptr<Field> result;

  if(field_name.empty() || table_name.empty())
    return result;

  type_vec_fields fields = get_fields_for_table(table_name);
  type_vec_fields::iterator iter = std::find_if(fields.begin(), fields.end(), predicate_FieldHasName<Field>(field_name));
  if(iter != fields.end()) //TODO: Handle error?
  {
    return *iter;
  }

  return sharedptr<Field>();
}

//static:
bool Base_DB::get_field_primary_key_index_for_fields(const type_vec_fields& fields, guint& field_column)
{
  //Initialize input parameter:
  field_column = 0;

  //TODO_performance: Cache the primary key?
  guint col = 0;
  guint cols_count = fields.size();
  while(col < cols_count)
  {
    if(fields[col]->get_primary_key())
    {
      field_column = col;
      return true;
    }
    else
    {
      ++col;
    }
  }

  return false; //Not found.
}

//static:
bool Base_DB::get_field_primary_key_index_for_fields(const type_vecLayoutFields& fields, guint& field_column)
{
  //Initialize input parameter:
  field_column = 0;

  //TODO_performance: Cache the primary key?
  guint col = 0;
  guint cols_count = fields.size();
  while(col < cols_count)
  {
    if(fields[col]->get_full_field_details()->get_primary_key())
    {
      field_column = col;
      return true;
    }
    else
    {
      ++col;
    }
  }

  return false; //Not found.
}

sharedptr<Field> Base_DB::get_field_primary_key_for_table(const Glib::ustring& table_name) const
{
  const Document* document = get_document();
  if(document)
  {
    //TODO_Performance: Cache this result?
    Document::type_vec_fields fields = document->get_table_fields(table_name);
    //std::cout << "debug: " << G_STRFUNC << ": table=" << table_name << ", fields count=" << fields.size() << std::endl;
    for(Document::type_vec_fields::iterator iter = fields.begin(); iter != fields.end(); ++iter)
    {
      sharedptr<Field> field = *iter;
      if(!field)
        continue;

      //std::cout << "  field=" << field->get_name() << std::endl;

      if(field->get_primary_key())
        return *iter;
    }
  }

  return sharedptr<Field>();
}

void Base_DB::get_table_fields_to_show_for_sequence_add_group(const Glib::ustring& table_name, const Privileges& table_privs, const type_vec_fields& all_db_fields, const sharedptr<LayoutGroup>& group, Base_DB::type_vecConstLayoutFields& vecFields) const
{
  //g_warning("Box_Data::get_table_fields_to_show_for_sequence_add_group(): table_name=%s, all_db_fields.size()=%d, group->name=%s", table_name.c_str(), all_db_fields.size(), group->get_name().c_str());

  LayoutGroup::type_list_items items = group->get_items();
  for(LayoutGroup::type_list_items::iterator iterItems = items.begin(); iterItems != items.end(); ++iterItems)
  {
    sharedptr<LayoutItem> item = *iterItems;

    sharedptr<LayoutItem_Field> item_field = sharedptr<LayoutItem_Field>::cast_dynamic(item);
    if(item_field)
    {
      //Get the field info:
      const Glib::ustring field_name = item->get_name();

      if(item_field->get_has_relationship_name()) //If it's a field in a related table.
      {
        //TODO_Performance: get_fields_for_table_one_field() is probably very inefficient
        sharedptr<Field> field = get_fields_for_table_one_field(item_field->get_table_used(table_name), item->get_name());
        if(field)
        {
          sharedptr<LayoutItem_Field> layout_item = item_field;
          layout_item->set_full_field_details(field); //Fill in the full field information for later.


          //TODO_Performance: We do this once for each related field, even if there are 2 from the same table:
          const Privileges privs_related = Privs::get_current_privs(item_field->get_table_used(table_name));
          layout_item->m_priv_view = privs_related.m_view;
          layout_item->m_priv_edit = privs_related.m_edit;

          vecFields.push_back(layout_item);
        }
        else
        {
          std::cerr << G_STRFUNC << ": related field not found: field=" << item->get_layout_display_name() << std::endl;
        }
      }
      else //It's a regular field in the table:
      {
        type_vec_fields::const_iterator iterFind = std::find_if(all_db_fields.begin(), all_db_fields.end(), predicate_FieldHasName<Field>(field_name));

        //If the field does not exist anymore then we won't try to show it:
        if(iterFind != all_db_fields.end() )
        {
          sharedptr<LayoutItem_Field> layout_item = item_field;
          layout_item->set_full_field_details(*iterFind); //Fill the LayoutItem with the full field information.

          //std::cout << "debug: " << G_STRFUNC << ": name=" << layout_item->get_name() << std::endl;

          //Prevent editing of the field if the user may not edit this table:
          layout_item->m_priv_view = table_privs.m_view;
          layout_item->m_priv_edit = table_privs.m_edit;

          vecFields.push_back(layout_item);
        }
      }
    }
    else
    {
      sharedptr<LayoutGroup> item_group = sharedptr<LayoutGroup>::cast_dynamic(item);
      if(item_group)
      {
        sharedptr<LayoutItem_Portal> item_portal = sharedptr<LayoutItem_Portal>::cast_dynamic(item);
        if(!item_portal) //Do not recurse into portals. They are filled by means of a separate SQL query.
        {
          //Recurse:
          get_table_fields_to_show_for_sequence_add_group(table_name, table_privs, all_db_fields, item_group, vecFields);
        }
      }
    }
  }

  if(vecFields.empty())
  {
    //g_warning("Box_Data::get_table_fields_to_show_for_sequence_add_group(): Returning empty list.");
  }
}

Base_DB::type_vecConstLayoutFields Base_DB::get_table_fields_to_show_for_sequence(const Glib::ustring& table_name, const Document::type_list_layout_groups& mapGroupSequence) const
{
  //Get field definitions from the database, with corrections from the document:
  type_vec_fields all_fields = get_fields_for_table(table_name);

  const Privileges table_privs = Privs::get_current_privs(table_name);

  //Get fields that the document says we should show:
  type_vecConstLayoutFields result;
  const Document* pDoc = dynamic_cast<const Document*>(get_document());
  if(pDoc)
  {
    if(mapGroupSequence.empty())
    {
      //No field sequence has been saved in the document, so we use all fields by default, so we start with something visible:

      //Start with the Primary Key as the first field:
      guint iPrimaryKey = 0;
      bool bPrimaryKeyFound = get_field_primary_key_index_for_fields(all_fields, iPrimaryKey);
      Glib::ustring primary_key_field_name;
      if(bPrimaryKeyFound)
      {
        sharedptr<LayoutItem_Field> layout_item = sharedptr<LayoutItem_Field>::create();
        layout_item->set_full_field_details(all_fields[iPrimaryKey]);

        //Don't use thousands separators with ID numbers:
        layout_item->m_formatting.m_numeric_format.m_use_thousands_separator = false;

        layout_item->set_editable(true); //A sensible default.

        //Prevent editing of the field if the user may not edit this table:
        layout_item->m_priv_view = table_privs.m_view;
        layout_item->m_priv_edit = table_privs.m_edit;

        result.push_back(layout_item);
      }

      //Add the rest:
      for(type_vec_fields::const_iterator iter = all_fields.begin(); iter != all_fields.end(); ++iter)
      {
        sharedptr<Field> field_info = *iter;

        if((*iter)->get_name() != primary_key_field_name) //We already added the primary key.
        {
          sharedptr<LayoutItem_Field> layout_item = sharedptr<LayoutItem_Field>::create();
          layout_item->set_full_field_details(field_info);

          layout_item->set_editable(true); //A sensible default.

          //Prevent editing of the field if the user may not edit this table:
          layout_item->m_priv_view = table_privs.m_view;
          layout_item->m_priv_edit = table_privs.m_edit;

          result.push_back(layout_item);
        }
      }
    }
    else
    {
      type_vec_fields vecFieldsInDocument = pDoc->get_table_fields(table_name);

      //We will show the fields that the document says we should:
      for(Document::type_list_layout_groups::const_iterator iter = mapGroupSequence.begin(); iter != mapGroupSequence.end(); ++iter)
      {
        sharedptr<LayoutGroup> group = *iter;

        if(true) //!group->m_hidden)
        {
          //Get the fields:
          get_table_fields_to_show_for_sequence_add_group(table_name, table_privs, all_fields, group, result);
        }
      }
    }
  }

  if(result.empty())
  {
    //g_warning("Box_Data::get_table_fields_to_show_for_sequence_add_group(): Returning empty list.");
  }

  return result;
}

void Base_DB::calculate_field_in_all_records(const Glib::ustring& table_name, const sharedptr<const Field>& field)
{
  sharedptr<const Field> primary_key = get_field_primary_key_for_table(table_name);
  calculate_field_in_all_records(table_name, field, primary_key);
}

void Base_DB::calculate_field_in_all_records(const Glib::ustring& table_name, const sharedptr<const Field>& field, const sharedptr<const Field>& primary_key)
{

  //Get primary key values for every record:
  Glib::RefPtr<Gnome::Gda::SqlBuilder> builder = Gnome::Gda::SqlBuilder::create(Gnome::Gda::SQL_STATEMENT_SELECT);
  builder->select_add_field(primary_key->get_name(), table_name);
  builder->select_add_target(table_name);

  Glib::RefPtr<Gnome::Gda::DataModel> data_model = DbUtils::query_execute_select(builder);
  if(!data_model || !data_model->get_n_rows() || !data_model->get_n_columns())
  {
    //HandleError();
    return;
  }

  LayoutFieldInRecord field_in_record;
  field_in_record.m_table_name = table_name;

  sharedptr<LayoutItem_Field> layoutitem_field = sharedptr<LayoutItem_Field>::create();
  layoutitem_field->set_full_field_details(field);
  field_in_record.m_field = layoutitem_field;
  field_in_record.m_key = primary_key;

  //Calculate the value for the field in every record:
  const int rows_count = data_model->get_n_rows();
  for(int row = 0; row < rows_count; ++row)
  {
    const Gnome::Gda::Value primary_key_value = data_model->get_value_at(0, row);

    if(!Conversions::value_is_empty(primary_key_value))
    {
      field_in_record.m_key_value = primary_key_value;

      m_FieldsCalculationInProgress.clear();
      calculate_field(field_in_record);
    }
  }
}

void Base_DB::calculate_field(const LayoutFieldInRecord& field_in_record)
{
  const Glib::ustring field_name = field_in_record.m_field->get_name();
  //g_warning("Box_Data::calculate_field(): field_name=%s", field_name.c_str());

  //Do we already have this in our list?
  type_field_calcs::iterator iterFind = m_FieldsCalculationInProgress.find(field_name);
  if(iterFind == m_FieldsCalculationInProgress.end()) //If it was not found.
  {
    //Add it:
    CalcInProgress item;
    item.m_field = field_in_record.m_field->get_full_field_details();
    m_FieldsCalculationInProgress[field_name] = item;

    iterFind = m_FieldsCalculationInProgress.find(field_name); //Always succeeds.
  }

  CalcInProgress& refCalcProgress = iterFind->second;

  //Use the previously-calculated value if possible:
  if(refCalcProgress.m_calc_in_progress)
  {
    //g_warning("  Box_Data::calculate_field(): Circular calculation detected. field_name=%s", field_name.c_str());
    //refCalcProgress.m_value = Conversions::get_empty_value(field->get_glom_type()); //Give up.
  }
  else if(refCalcProgress.m_calc_finished)
  {
    //g_warning("  Box_Data::calculate_field(): Already calculated.");

    //Don't bother calculating it again. The correct value is already in the database and layout.
  }
  else
  {
    //g_warning("  Box_Data::calculate_field(): setting calc_in_progress: field_name=%s", field_name.c_str());

    refCalcProgress.m_calc_in_progress = true; //Let the recursive calls to calculate_field() check this.

    sharedptr<LayoutItem_Field> layout_item = sharedptr<LayoutItem_Field>::create();
    layout_item->set_full_field_details(refCalcProgress.m_field);

    //Calculate dependencies first:
    //TODO: Prevent unncessary recalculations?
    const type_list_const_field_items fields_needed = get_calculation_fields(field_in_record.m_table_name, field_in_record.m_field);
    for(type_list_const_field_items::const_iterator iterNeeded = fields_needed.begin(); iterNeeded != fields_needed.end(); ++iterNeeded)
    {
      sharedptr<const LayoutItem_Field> field_item_needed = *iterNeeded;

      if(field_item_needed->get_has_relationship_name())
      {
        //TOOD: Handle related fields? We already handle whole relationships.
      }
      else
      {
        sharedptr<const Field> field_needed = field_item_needed->get_full_field_details();
        if(field_needed)
        {
          if(field_needed->get_has_calculation())
          {
            //g_warning("  calling calculate_field() for %s", iterNeeded->c_str());
            //TODO: What if the field is in a different table?

            LayoutFieldInRecord needed_field_in_record(field_item_needed, field_in_record.m_table_name, field_in_record.m_key, field_in_record.m_key_value);
            calculate_field(needed_field_in_record);
          }
          else
          {
            //g_warning("  not a calculated field->");
          }
        }
      }
    }


    //m_FieldsCalculationInProgress has changed, probably invalidating our iter, so get it again:
    iterFind = m_FieldsCalculationInProgress.find(field_name); //Always succeeds.
    CalcInProgress& refCalcProgress = iterFind->second;

    //Check again, because the value miight have been calculated during the dependencies.
    if(refCalcProgress.m_calc_finished)
    {
      //We recently calculated this value, and set it in the database and layout, so don't waste time doing it again:
    }
    else
    {
      //recalculate:
      //TODO_Performance: We don't know what fields the python calculation will use, so we give it all of them:
      const type_map_fields field_values = get_record_field_values_for_calculation(field_in_record.m_table_name, field_in_record.m_key, field_in_record.m_key_value);
      if(!field_values.empty())
      {
        sharedptr<const Field> field = refCalcProgress.m_field;
        if(field)
        {
          //We need the connection when we run the script, so that the script may use it.
          sharedptr<SharedConnection> sharedconnection = connect_to_server(0 /* parent window */);

          g_assert(sharedconnection);

          Glib::ustring error_message; //TODO: Check this.
          refCalcProgress.m_value =
            glom_evaluate_python_function_implementation(field->get_glom_type(),
              field->get_calculation(),
              field_values,
              get_document(),
              field_in_record.m_table_name,
              field_in_record.m_key, field_in_record.m_key_value,
              sharedconnection->get_gda_connection(),
              error_message);

          refCalcProgress.m_calc_finished = true;
          refCalcProgress.m_calc_in_progress = false;

          sharedptr<LayoutItem_Field> layout_item = sharedptr<LayoutItem_Field>::create();
          layout_item->set_full_field_details(field);

          //show it:
          set_entered_field_data(layout_item, refCalcProgress.m_value ); //TODO: If this record is shown.

          //Add it to the database (even if it is not shown in the view)
          //Using true for the last parameter means we use existing calculations where possible,
          //instead of recalculating a field that is being calculated already, and for which this dependent field is being calculated anyway.
          Document* document = get_document();
          if(document)
          {
            LayoutFieldInRecord field_in_record_layout(layout_item, field_in_record.m_table_name /* parent */, field_in_record.m_key, field_in_record.m_key_value);

            set_field_value_in_database(field_in_record_layout, refCalcProgress.m_value, true); //This triggers other recalculations/lookups.
          }
        }
      }
    }
  }

}

Base_DB::type_map_fields Base_DB::get_record_field_values_for_calculation(const Glib::ustring& table_name, const sharedptr<const Field> primary_key, const Gnome::Gda::Value& primary_key_value)
{
  type_map_fields field_values;

  Document* document = get_document();
  if(document)
  {
    //TODO: Cache the list of all fields, as well as caching (m_Fields) the list of all visible fields:
    const Document::type_vec_fields fields = document->get_table_fields(table_name);

    //TODO: This seems silly. We should just have a build_sql_select() that can take this container:
    type_vecLayoutFields fieldsToGet;
    for(Document::type_vec_fields::const_iterator iter = fields.begin(); iter != fields.end(); ++iter)
    {
      sharedptr<LayoutItem_Field> layout_item = sharedptr<LayoutItem_Field>::create();
      layout_item->set_full_field_details(*iter);

      fieldsToGet.push_back(layout_item);
    }

    if(!Conversions::value_is_empty(primary_key_value))
    {
      //sharedptr<const Field> fieldPrimaryKey = get_field_primary_key();

      Glib::RefPtr<Gnome::Gda::SqlBuilder> query = Utils::build_sql_select_with_key(table_name, fieldsToGet, primary_key, primary_key_value);

      Glib::RefPtr<const Gnome::Gda::DataModel> data_model;
      try
      {
        data_model = DbUtils::query_execute_select(query);
      }
      catch(const Glib::Exception& ex)
      {
        std::cerr << G_STRFUNC << ": Exception while executing SQL: " << query << std::endl;
        handle_error(ex);
        return field_values;
      }
      if(data_model && data_model->get_n_rows())
      {
        int col_index = 0;
        for(Document::type_vec_fields::const_iterator iter = fields.begin(); iter != fields.end(); ++iter)
        {
          //There should be only 1 row. Well, there could be more but we will ignore them.
          sharedptr<const Field> field = *iter;
          Gnome::Gda::Value value = data_model->get_value_at(col_index, 0);
          //Never give a NULL-type value to the python calculation for types that don't use them:
          //to prevent errors:
          if(value.is_null())
            value = Conversions::get_empty_value(field->get_glom_type());

          field_values[field->get_name()] = value;
          ++col_index;
        }
      }
      else
      {
        //Maybe the record does not exist yet
        //(Maybe we need the field values so we can calculate default values for some fields when creating the record.)
        //So we create appropriate empty values below.
      }
    }

    if(field_values.empty()) //Maybe there was no primary key, or maybe the record is not yet in the database.
    {
      //Create appropriate empty values:
      for(Document::type_vec_fields::const_iterator iter = fields.begin(); iter != fields.end(); ++iter)
      {
        sharedptr<const Field> field = *iter;
        field_values[field->get_name()] = Conversions::get_empty_value(field->get_glom_type());
      }
    }
  }

  return field_values;
}

void Base_DB::set_entered_field_data(const sharedptr<const LayoutItem_Field>& /* field */, const Gnome::Gda::Value& /* value */)
{
  //Override this.
}


void Base_DB::set_entered_field_data(const Gtk::TreeModel::iterator& /* row */, const sharedptr<const LayoutItem_Field>& /* field */, const Gnome::Gda::Value& /* value */)
{
  //Override this.
}

bool Base_DB::set_field_value_in_database(const LayoutFieldInRecord& field_in_record, const Gnome::Gda::Value& field_value, bool use_current_calculations, Gtk::Window* parent_window)
{
  return set_field_value_in_database(field_in_record, Gtk::TreeModel::iterator(), field_value, use_current_calculations, parent_window);
}

bool Base_DB::set_field_value_in_database(const LayoutFieldInRecord& layoutfield_in_record, const Gtk::TreeModel::iterator& row, const Gnome::Gda::Value& field_value, bool use_current_calculations, Gtk::Window* /* parent_window */)
{
  Document* document = get_document();
  g_assert(document);

  const FieldInRecord field_in_record = layoutfield_in_record.get_fieldinrecord(*document);

  //row is invalid, and ignored, for Box_Data_Details.
  if(!(field_in_record.m_field))
  {
    std::cerr << G_STRFUNC << ": field_in_record.m_field is empty." << std::endl;
    return false;
  }

  if(!(field_in_record.m_key))
  {
    std::cerr << G_STRFUNC << ": field_in_record.m_key is empty." << std::endl;
    return false;
  }

  const Glib::ustring field_name = field_in_record.m_field->get_name();
  if(!field_name.empty()) //This should not happen.
  {
    Glib::RefPtr<Gnome::Gda::SqlBuilder> builder = Gnome::Gda::SqlBuilder::create(Gnome::Gda::SQL_STATEMENT_UPDATE);
    builder->set_table(field_in_record.m_table_name);
    builder->add_field_value_as_value(field_in_record.m_field->get_name(), field_value);
    builder->set_where(
      builder->add_cond(Gnome::Gda::SQL_OPERATOR_TYPE_EQ,
        builder->add_field_id(field_in_record.m_key->get_name(), field_in_record.m_table_name),
        builder->add_expr_as_value(field_in_record.m_key_value)));

    try //TODO: The exceptions are probably already handled by query_execute(
    {
      const bool test = DbUtils::query_execute(builder); //TODO: Respond to failure.
      if(!test)
      {
        std::cerr << G_STRFUNC << ": UPDATE failed." << std::endl;
        return false; //failed.
      }
    }
    catch(const Glib::Exception& ex)
    {
      handle_error(ex);
      return false;
    }
    catch(const std::exception& ex)
    {
      handle_error(ex);
      return false;
    }

    //Get-and-set values for lookup fields, if this field triggers those relationships:
    do_lookups(layoutfield_in_record, row, field_value);

    //Update related fields, if this field is used in the relationship:
    refresh_related_fields(layoutfield_in_record, row, field_value);

    //Calculate any dependent fields.
    //TODO: Make lookups part of this?
    //Prevent circular calculations during the recursive do_calculations:
    {
      //Recalculate any calculated fields that depend on this calculated field.
      //g_warning("Box_Data::set_field_value_in_database(): calling do_calculations");

      do_calculations(layoutfield_in_record, !use_current_calculations);
    }
  }

  return true;
}

Gnome::Gda::Value Base_DB::get_field_value_in_database(const LayoutFieldInRecord& field_in_record, Gtk::Window* /* parent_window */)
{
  Gnome::Gda::Value result;  //TODO: Return suitable empty value for the field when failing?

  //row is invalid, and ignored, for Box_Data_Details.
  if(!(field_in_record.m_field))
  {
    std::cerr << G_STRFUNC << ": field_in_record.m_field is empty." << std::endl;
    return result;
  }

  //Check that there is a key value, if there should be one:
  //System Preferences, for instance, should not need a key to identify the record:
  if(!(field_in_record.m_key))
  {
    Glib::ustring to_field;
    if(field_in_record.m_field &&
      field_in_record.m_field->get_relationship())
    {
      to_field = field_in_record.m_field->get_relationship()->get_to_field();
    }
      
    if(!to_field.empty())
    {
      std::cerr << G_STRFUNC << ": field_in_record.m_key is empty." << std::endl;
      return result;
    }
  }
  

  type_vecConstLayoutFields list_fields;
  sharedptr<const LayoutItem_Field> layout_item = field_in_record.m_field;
  list_fields.push_back(layout_item);
  Glib::RefPtr<Gnome::Gda::SqlBuilder> sql_query = Utils::build_sql_select_with_key(field_in_record.m_table_name,
    list_fields, field_in_record.m_key, field_in_record.m_key_value, type_sort_clause(), 1);

  Glib::RefPtr<const Gnome::Gda::DataModel> data_model = DbUtils::query_execute_select(sql_query);
  if(data_model)
  {
    if(data_model->get_n_rows())
    {
      result = data_model->get_value_at(0, 0);
    }
  }
  else
  {
    handle_error();
  }

  return result;
}

Gnome::Gda::Value Base_DB::get_field_value_in_database(const sharedptr<Field>& field, const FoundSet& found_set, Gtk::Window* /* parent_window */)
{
  Gnome::Gda::Value result;  //TODO: Return suitable empty value for the field when failing?

  //row is invalid, and ignored, for Box_Data_Details.
  if(!field)
  {
    std::cerr << G_STRFUNC << ": field is empty." << std::endl;
    return result;
  }

  if(found_set.m_where_clause.empty())
  {
    std::cerr << G_STRFUNC << ": where_clause is empty." << std::endl;
    return result;
  }

  type_vecConstLayoutFields list_fields;
  sharedptr<LayoutItem_Field> layout_item = sharedptr<LayoutItem_Field>::create();
  layout_item->set_full_field_details(field);
  list_fields.push_back(layout_item);
  Glib::RefPtr<Gnome::Gda::SqlBuilder> sql_query = Utils::build_sql_select_with_where_clause(found_set.m_table_name,
    list_fields,
    found_set.m_where_clause,
    sharedptr<const Relationship>() /* extra_join */, type_sort_clause(),
    1 /* limit */);

  Glib::RefPtr<const Gnome::Gda::DataModel> data_model = DbUtils::query_execute_select(sql_query);
  if(data_model)
  {
    if(data_model->get_n_rows())
    {
      result = data_model->get_value_at(0, 0);
    }
  }
  else
  {
    handle_error();
  }

  return result;
}


void Base_DB::do_calculations(const LayoutFieldInRecord& field_changed, bool first_calc_field)
{
  //std::cout << "debug: " << G_STRFUNC << ": field_changed=" << field_changed.m_field->get_name() << std::endl;

  if(first_calc_field)
  {
    //g_warning("  clearing m_FieldsCalculationInProgress");
    m_FieldsCalculationInProgress.clear();
  }

  //Recalculate fields that are triggered by a change of this field's value, not including calculations that these calculations use.

  type_list_const_field_items calculated_fields = get_calculated_fields(field_changed.m_table_name, field_changed.m_field);
  //std::cout << "  debug: calculated_field.size()=" << calculated_fields.size() << std::endl;
  for(type_list_const_field_items::const_iterator iter = calculated_fields.begin(); iter != calculated_fields.end(); ++iter)
  {
    sharedptr<const LayoutItem_Field> field = *iter;
    if(field)
    {
      //std::cout << "debug: recalcing field: " << field->get_name() << std::endl;
      //TODO: What if the field is in another table?
      LayoutFieldInRecord triggered_field(field, field_changed.m_table_name, field_changed.m_key, field_changed.m_key_value);
      calculate_field(triggered_field); //And any dependencies.

      //Calculate anything that depends on this.
      do_calculations(triggered_field, false /* recurse, reusing m_FieldsCalculationInProgress */);
    }
  }

  if(first_calc_field)
    m_FieldsCalculationInProgress.clear();
}

Base_DB::type_list_const_field_items Base_DB::get_calculated_fields(const Glib::ustring& table_name, const sharedptr<const LayoutItem_Field>& field)
{
  //std::cout << "debug: Base_DB::get_calculated_fields field=" << field->get_name() << std::endl;

  type_list_const_field_items result;

  const Document* document = dynamic_cast<const Document*>(get_document());
  if(document)
  {
    //Look at each field in the table, and get lists of what fields trigger their calculations,
    //so we can see if our field is in any of those lists:

    const type_vec_fields fields = document->get_table_fields(table_name); //TODO_Performance: Cache this?
    //Examine all fields, not just the the shown fields.
    //TODO: How do we trigger relcalculation of related fields if necessary?
    for(type_vec_fields::const_iterator iter = fields.begin(); iter != fields.end();  ++iter)
    {
      sharedptr<Field> field_to_examine = *iter;
      sharedptr<LayoutItem_Field> layoutitem_field_to_examine = sharedptr<LayoutItem_Field>::create();
      layoutitem_field_to_examine->set_full_field_details(field_to_examine);

      //std::cout << "  debug: examining field=" << field_to_examine->get_name() << std::endl;

      //Does this field's calculation use the field?
      const type_list_const_field_items fields_triggered = get_calculation_fields(table_name, layoutitem_field_to_examine);
      //std::cout << "    debug: field_triggered.size()=" << fields_triggered.size() << std::endl;
      type_list_const_field_items::const_iterator iterFind = std::find_if(fields_triggered.begin(), fields_triggered.end(), predicate_LayoutItemIsEqual<LayoutItem_Field>(field));
      if(iterFind != fields_triggered.end())
      {
        //std::cout << "      debug: FOUND: name=" << layoutitem_field_to_examine->get_name() << std::endl;
        //Tell the caller that this field is triggered by the specified field:
        //TODO: Test related fields too?

        result.push_back(layoutitem_field_to_examine);
      }
    }
  }

  return result;
}

Base_DB::type_list_const_field_items Base_DB::get_calculation_fields(const Glib::ustring& table_name, const sharedptr<const LayoutItem_Field>& layoutitem_field)
{
  //TODO: Use regex, for instance with pcre here?
  //TODO: Better?: Run the calculation on some example data, and record the touched fields? But this could not exercise every code path.
  //TODO_Performance: Just cache the result whenever m_calculation changes.

  type_list_const_field_items result;

  sharedptr<const Field> field = layoutitem_field->get_full_field_details();
  if(!field)
    return result;

  Glib::ustring::size_type index = 0;
  const Glib::ustring calculation = field->get_calculation();
  if(calculation.empty())
    return result;

  Document* document = get_document();
  if(!document)
    return result;

  const Glib::ustring::size_type count = calculation.size();
  const Glib::ustring prefix = "record[\"";
  const Glib::ustring::size_type prefix_size = prefix.size();

  while(index < count)
  {
    Glib::ustring::size_type pos_find = calculation.find(prefix, index);
    if(pos_find != Glib::ustring::npos)
    {
      Glib::ustring::size_type pos_find_end = calculation.find("\"]", pos_find);
      if(pos_find_end  != Glib::ustring::npos)
      {
        Glib::ustring::size_type pos_start = pos_find + prefix_size;
        const Glib::ustring field_name = calculation.substr(pos_start, pos_find_end - pos_start);

        sharedptr<Field> field_found = document->get_field(table_name, field_name);
        if(field)
        {
          sharedptr<LayoutItem_Field> layout_item = sharedptr<LayoutItem_Field>::create();
          layout_item->set_full_field_details(field_found);

          result.push_back(layout_item);
        }

        index = pos_find_end + 1;
      }
    }

    ++index;
  }

  //Check the use of related records too:
  const Field::type_list_strings relationships_used = field->get_calculation_relationships();
  for(Field::type_list_strings::const_iterator iter = relationships_used.begin(); iter != relationships_used.end(); ++iter)
  {
    sharedptr<Relationship> relationship = document->get_relationship(table_name, *iter);
    if(relationship)
    {
      //If the field uses this relationship then it should be triggered by a change in the key that specifies which record the relationship points to:
      const Glib::ustring field_from_name = relationship->get_from_field();
      sharedptr<Field> field_from = document->get_field(table_name, field_from_name);
      if(field_from)
      {
        sharedptr<LayoutItem_Field> layout_item = sharedptr<LayoutItem_Field>::create();
        layout_item->set_full_field_details(field_from);

        result.push_back(layout_item);
      }
    }
  }

  return result;
}


void Base_DB::do_lookups(const LayoutFieldInRecord& field_in_record, const Gtk::TreeModel::iterator& row, const Gnome::Gda::Value& field_value)
{
   //Get values for lookup fields, if this field triggers those relationships:
   //TODO_performance: There is a LOT of iterating and copying here.
   const Glib::ustring strFieldName = field_in_record.m_field->get_name();
   const type_list_lookups lookups = get_lookup_fields(field_in_record.m_table_name, strFieldName);
   //std::cout << "debug: " << G_STRFUNC << ": lookups size=" << lookups.size() << std::endl;
   for(type_list_lookups::const_iterator iter = lookups.begin(); iter != lookups.end(); ++iter)
   {
     sharedptr<const LayoutItem_Field> layout_item = iter->first;

     //std::cout << "debug: " << G_STRFUNC << ": item=" << layout_item->get_name() << std::endl;

     sharedptr<const Relationship> relationship = iter->second;
     const sharedptr<const Field> field_lookup = layout_item->get_full_field_details();
     if(field_lookup)
     {
      sharedptr<const Field> field_source = get_fields_for_table_one_field(relationship->get_to_table(), field_lookup->get_lookup_field());
      if(field_source)
      {
        const Gnome::Gda::Value value = get_lookup_value(field_in_record.m_table_name, iter->second /* relationship */,  field_source /* the field to look in to get the value */, field_value /* Value of to and from fields */);

        const Gnome::Gda::Value value_converted = Conversions::convert_value(value, layout_item->get_glom_type());

        LayoutFieldInRecord field_in_record_to_set(layout_item, field_in_record.m_table_name /* parent table */, field_in_record.m_key, field_in_record.m_key_value);

        //Add it to the view:
        set_entered_field_data(row, layout_item, value_converted);
        //m_AddDel.set_value(row, layout_item, value_converted);

        //Add it to the database (even if it is not shown in the view)
        set_field_value_in_database(field_in_record_to_set, row, value_converted); //Also does dependent lookups/recalcs.

        //TODO: Handle lookups triggered by these fields (recursively)? TODO: Check for infinitely looping lookups.
      }
    }
  }
}


/** Get the fields whose values should be looked up when @a field_name changes, with
 * the relationship used to lookup the value.
 */
Base_DB::type_list_lookups Base_DB::get_lookup_fields(const Glib::ustring& table_name, const Glib::ustring& field_name) const
{
  type_list_lookups result;

  const Document* document = dynamic_cast<const Document*>(get_document());
  if(document)
  {
    //Examine all fields, not just the the shown fields (m_Fields):
    const type_vec_fields fields = document->get_table_fields(table_name); //TODO_Performance: Cache this?
    //Examine all fields, not just the the shown fields.
    for(type_vec_fields::const_iterator iter = fields.begin(); iter != fields.end();  ++iter)
    {
      sharedptr<Field> field = *iter;

      //Examine each field that looks up its data from a relationship:
      if(field && field->get_is_lookup())
      {
        //Get the relationship information:
        sharedptr<Relationship> relationship = field->get_lookup_relationship();
        if(relationship)
        {
          //If the relationship is triggererd by the specified field:
          if(relationship->get_from_field() == field_name)
          {
            //Add it:
            sharedptr<LayoutItem_Field> item = sharedptr<LayoutItem_Field>::create();
            item->set_full_field_details(field);
            result.push_back( type_pairFieldTrigger(item, relationship) );
          }
        }
      }
    }
  }

  return result;
}

void Base_DB::refresh_related_fields(const LayoutFieldInRecord& /* field_in_record_changed */, const Gtk::TreeModel::iterator& /* row */, const Gnome::Gda::Value& /* field_value */)
{
  //overridden in Box_Data.
}

Gnome::Gda::Value Base_DB::get_lookup_value(const Glib::ustring& /* table_name */, const sharedptr<const Relationship>& relationship, const sharedptr<const Field>& source_field, const Gnome::Gda::Value& key_value)
{
  Gnome::Gda::Value result;

  sharedptr<Field> to_key_field = get_fields_for_table_one_field(relationship->get_to_table(), relationship->get_to_field());
  if(to_key_field)
  {
    //Convert the value, in case the from and to fields have different types:
    const Gnome::Gda::Value value_to_key_field = Conversions::convert_value(key_value, to_key_field->get_glom_type());

    const Glib::ustring target_table = relationship->get_to_table();
    Glib::RefPtr<Gnome::Gda::SqlBuilder> builder =
      Gnome::Gda::SqlBuilder::create(Gnome::Gda::SQL_STATEMENT_SELECT);
    builder->select_add_field(source_field->get_name(), target_table );
    builder->select_add_target(target_table );
    builder->set_where(
      builder->add_cond(Gnome::Gda::SQL_OPERATOR_TYPE_EQ,
        builder->add_field_id(to_key_field->get_name(), target_table),
        builder->add_expr(value_to_key_field)));

    Glib::RefPtr<Gnome::Gda::DataModel> data_model = DbUtils::query_execute_select(builder);
    if(data_model && data_model->get_n_rows())
    {
      //There should be only 1 row. Well, there could be more but we will ignore them.
      result = data_model->get_value_at(0, 0);
    }
    else
    {
      handle_error();
    }
  }

  return result;
}

bool Base_DB::get_field_value_is_unique(const Glib::ustring& table_name, const sharedptr<const LayoutItem_Field>& field, const Gnome::Gda::Value& value)
{
  bool result = true;  //Arbitrarily default to saying it's unique if we can't get any result.

  const Glib::ustring table_name_used = field->get_table_used(table_name);

  Glib::RefPtr<Gnome::Gda::SqlBuilder> builder =
    Gnome::Gda::SqlBuilder::create(Gnome::Gda::SQL_STATEMENT_SELECT);
  builder->select_add_field(field->get_name(), table_name_used);
  builder->select_add_target(table_name_used);
  builder->set_where(
    builder->add_cond(Gnome::Gda::SQL_OPERATOR_TYPE_EQ,
      builder->add_field_id(field->get_name(), table_name_used),
      builder->add_expr(value)));

  Glib::RefPtr<const Gnome::Gda::DataModel> data_model = DbUtils::query_execute_select(builder);
  if(data_model)
  {
    //std::cout << "debug: " << G_STRFUNC << ": table_name=" << table_name << ", field name=" << field->get_name() << ", value=" << value.to_string() << ", rows count=" << data_model->get_n_rows() << std::endl;
    //The value is unique for this field, if the query returned no existing rows:

    result = (data_model->get_n_rows() == 0);
  }
  else
  {
    handle_error();
  }

  return result;
}

bool Base_DB::check_entered_value_for_uniqueness(const Glib::ustring& table_name, const sharedptr<const LayoutItem_Field>& layout_field, const Gnome::Gda::Value& field_value, Gtk::Window* parent_window)
{
  return check_entered_value_for_uniqueness(table_name, Gtk::TreeModel::iterator(), layout_field, field_value, parent_window);
}

bool Base_DB::check_entered_value_for_uniqueness(const Glib::ustring& table_name, const Gtk::TreeModel::iterator& /* row */,  const sharedptr<const LayoutItem_Field>& layout_field, const Gnome::Gda::Value& field_value, Gtk::Window* parent_window)
{
  //Check whether the value meets uniqueness constraints, if any:
  const sharedptr<const Field>& field = layout_field->get_full_field_details();
  if(field && (field->get_primary_key() || field->get_unique_key()))
  {
    if(!get_field_value_is_unique(table_name, layout_field, field_value))
    {
      //std::cout << "debug: " << G_STRFUNC << ": field=" << layout_field->get_name() << ", value is not unique: " << field_value.to_string() << std::endl;

      //Warn the user and revert the value:
      if(parent_window)
        Frame_Glom::show_ok_dialog(_("Value Is Not Unique"), _("The field's value must be unique, but a record with this value already exists."), *parent_window);

      return false; //Failed.
    }
    else
      return true; //Succeed, because the value is unique.
  }
  else
    return true; //Succeed, because the value does not need to be unique.
}

bool Base_DB::get_relationship_exists(const Glib::ustring& table_name, const Glib::ustring& relationship_name)
{
  Document* document = get_document();
  if(document)
  {
    sharedptr<Relationship> relationship = document->get_relationship(table_name, relationship_name);
    if(relationship)
      return true;
  }

  return false;
}

bool Base_DB::get_primary_key_is_in_foundset(const FoundSet& found_set, const Gnome::Gda::Value& primary_key_value)
{
  //TODO_Performance: This is probably called too often, when we should know that the key is in the found set.
  sharedptr<const Field> primary_key = get_field_primary_key_for_table(found_set.m_table_name);
  if(!primary_key)
  {
    std::cerr << G_STRFUNC << ": No primary key found for table: " << found_set.m_table_name << std::endl;
    return false;
  }

  type_vecLayoutFields fieldsToGet;

  sharedptr<LayoutItem_Field> layout_item = sharedptr<LayoutItem_Field>::create();
  layout_item->set_full_field_details(primary_key);
  fieldsToGet.push_back(layout_item);

  Glib::RefPtr<Gnome::Gda::SqlBuilder> builder = Gnome::Gda::SqlBuilder::create(Gnome::Gda::SQL_STATEMENT_SELECT);
  builder->select_add_target(found_set.m_table_name);

  const guint eq_id = builder->add_cond(Gnome::Gda::SQL_OPERATOR_TYPE_EQ,
        builder->add_field_id(primary_key->get_name(), found_set.m_table_name),
        builder->add_expr_as_value(primary_key_value));
  guint cond_id = 0;
  if(found_set.m_where_clause.empty())
  {
    cond_id = eq_id;
  }
  else
  {
    cond_id = builder->add_cond(Gnome::Gda::SQL_OPERATOR_TYPE_AND,
      builder->import_expression(found_set.m_where_clause),
      eq_id);
  }

  builder->set_where(cond_id); //This might be unnecessary.
    cond_id = eq_id;

  Glib::RefPtr<Gnome::Gda::SqlBuilder> query =
    Utils::build_sql_select_with_where_clause(found_set.m_table_name, fieldsToGet,
      builder->export_expression(cond_id));
  Glib::RefPtr<const Gnome::Gda::DataModel> data_model = DbUtils::query_execute_select(query);

  if(data_model && data_model->get_n_rows())
  {
    //std::cout << "debug: Record found: " << query << std::endl;
    return true; //A record was found in the record set with this value.
  }
  else
    return false;
}

int Base_DB::count_rows_returned_by(const Glib::RefPtr<const Gnome::Gda::SqlBuilder>& sql_query)
{
  if(!sql_query)
  {
    std::cerr << G_STRFUNC << ": sql_query was null." << std::endl;
    return 0;
  }

  Glib::RefPtr<Gnome::Gda::SqlBuilder> builder =
    Utils::build_sql_select_count_rows(sql_query);

  int result = 0;

  Glib::RefPtr<Gnome::Gda::DataModel> datamodel = DbUtils::query_execute_select(builder);
  if(datamodel && datamodel->get_n_rows() && datamodel->get_n_columns())
  {
    const Gnome::Gda::Value value = datamodel->get_value_at(0, 0);
    //This showed me that this contains a gint64: std::cerr << "DEBUG: value type=" << G_VALUE_TYPE_NAME(value.gobj()) << std::endl;
    //For sqlite, this is an integer
    if(value.get_value_type() == G_TYPE_INT64)
      result = (int)value.get_int64();
    else
      result = value.get_int();
  }

  //std::cout << "debug: " << G_STRFUNC << ": Returning " << result << std::endl;
  return result;
}


void Base_DB::set_found_set_where_clause_for_portal(FoundSet& found_set, const sharedptr<LayoutItem_Portal>& portal, const Gnome::Gda::Value& foreign_key_value)
{
  found_set.m_table_name = Glib::ustring();
  found_set.m_where_clause = Gnome::Gda::SqlExpr();
  found_set.m_extra_join = sharedptr<const Relationship>();

  if( !portal
      || Conversions::value_is_empty(foreign_key_value) )
  {
    return;
  }


  sharedptr<const Relationship> relationship = portal->get_relationship();

  // Notice that, in the case that this is a portal to doubly-related records,
  // The WHERE clause mentions the first-related table (though by the alias defined in extra_join)
  // and we add an extra JOIN to mention the second-related table.

  Glib::ustring where_clause_to_table_name = relationship->get_to_table();
  sharedptr<Field> where_clause_to_key_field = get_fields_for_table_one_field(relationship->get_to_table(), relationship->get_to_field());

  found_set.m_table_name = portal->get_table_used(Glib::ustring() /* parent table - not relevant */);

  sharedptr<const Relationship> relationship_related = portal->get_related_relationship();
  if(relationship_related)
  {
    //Add the extra JOIN:
    sharedptr<UsesRelationship> uses_rel_temp = sharedptr<UsesRelationship>::create();
    uses_rel_temp->set_relationship(relationship);
    found_set.m_extra_join = relationship;

    //Adjust the WHERE clause appropriately for the extra JOIN:
    where_clause_to_table_name = uses_rel_temp->get_sql_join_alias_name();

    const Glib::ustring to_field_name = uses_rel_temp->get_to_field_used();
    where_clause_to_key_field = get_fields_for_table_one_field(relationship->get_to_table(), to_field_name);
    //std::cout << "extra_join=" << found_set.m_extra_join << std::endl;

    //std::cout << "extra_join where_clause_to_key_field=" << where_clause_to_key_field->get_name() << std::endl;
  }

  if(where_clause_to_key_field)
  {
    found_set.m_where_clause =
      Utils::build_simple_where_expression(where_clause_to_table_name, where_clause_to_key_field, foreign_key_value);
  }
}

bool Base_DB::add_user(const Glib::ustring& user, const Glib::ustring& password, const Glib::ustring& group)
{
  if(user.empty() || password.empty() || group.empty())
    return false;

  //Create the user:
  //Note that ' around the user fails, so we use ".
  Glib::ustring strQuery = "CREATE USER \"" + user + "\" PASSWORD '" + password + "'" ; //TODO: Escape the password.
  if(group == GLOM_STANDARD_GROUP_NAME_DEVELOPER)
    strQuery += " SUPERUSER CREATEDB CREATEROLE"; //Because SUPERUSER is not "inherited" from groups to members.


  //Glib::ustring strQuery = "CREATE USER \"" + user + "\"";
  //if(group == GLOM_STANDARD_GROUP_NAME_DEVELOPER)
  //  strQuery += " WITH SUPERUSER"; //Because SUPERUSER is not "inherited" from groups to members.
  //strQuery +=  " PASSWORD '" + password + "'" ; //TODO: Escape the password.


  bool test = DbUtils::query_execute_string(strQuery);
  if(!test)
  {
    std::cerr << G_STRFUNC << ": CREATE USER failed." << std::endl;
    return false;
  }

  //Add it to the group:
  strQuery = "ALTER GROUP \"" + group + "\" ADD USER \"" + user + "\"";
  test = DbUtils::query_execute_string(strQuery);
  if(!test)
  {
    std::cerr << G_STRFUNC << ": ALTER GROUP failed." << std::endl;
    return false;
  }

  //Remove any user rights, so that all rights come from the user's presence in the group:
  Document* document = get_document();
  if(!document)
    return true;

  Document::type_listTableInfo table_list = document->get_tables();

  for(Document::type_listTableInfo::const_iterator iter = table_list.begin(); iter != table_list.end(); ++iter)
  {
    const Glib::ustring strQuery = "REVOKE ALL PRIVILEGES ON \"" + (*iter)->get_name() + "\" FROM \"" + user + "\"";
    const bool test = DbUtils::query_execute_string(strQuery);
    if(!test)
      std::cerr << G_STRFUNC << ": REVOKE failed." << std::endl;
  }

  return true;
}


bool Base_DB::remove_user(const Glib::ustring& user)
{
  if(user.empty())
    return false;

  const Glib::ustring strQuery = "DROP USER \"" + user + "\"";
  const bool test = DbUtils::query_execute_string(strQuery);
  if(!test)
  {
    std::cerr << G_STRFUNC << ": DROP USER failed" << std::endl;
    return false;
  }

  return true;
}

bool Base_DB::remove_user_from_group(const Glib::ustring& user, const Glib::ustring& group)
{
  if(user.empty() || group.empty())
    return false;

  const Glib::ustring strQuery = "ALTER GROUP \"" + group + "\" DROP USER \"" + user + "\"";
  const bool test = DbUtils::query_execute_string(strQuery);
  if(!test)
  {
    std::cerr << G_STRFUNC << ": ALTER GROUP failed." << std::endl;
    return false;
  }

  return true;
}

bool Base_DB::set_database_owner_user(const Glib::ustring& user)
{
  if(user.empty())
    return false;

  ConnectionPool* connectionpool = ConnectionPool::get_instance();
  const Glib::ustring database_name = connectionpool->get_database();
  if(database_name.empty())
    return false;

  const Glib::ustring strQuery = "ALTER DATABASE \"" + database_name + "\" OWNER TO \"" + user + "\"";
  const bool test = DbUtils::query_execute_string(strQuery);
  if(!test)
  {
    std::cerr << G_STRFUNC << ": ALTER DATABASE failed." << std::endl;
    return false;
  }

  return true;
}


bool Base_DB::disable_user(const Glib::ustring& user)
{
  if(user.empty())
    return false;

  type_vec_strings vecGroups = Privs::get_groups_of_user(user);
  for(type_vec_strings::const_iterator iter = vecGroups.begin(); iter != vecGroups.end(); ++iter)
  {
    const Glib::ustring group = *iter;
    remove_user_from_group(user, group);
  }

  const Glib::ustring strQuery = "ALTER ROLE \"" + user + "\" NOLOGIN NOSUPERUSER NOCREATEDB NOCREATEROLE";
  const bool test = DbUtils::query_execute_string(strQuery);
  if(!test)
  {
    std::cerr << G_STRFUNC << ": DROP USER failed" << std::endl;
    return false;
  }

  return true;
}

Glib::ustring Base_DB::get_active_layout_platform(Document* document)
{
  Glib::ustring result;
  if(document)
    result = document->get_active_layout_platform();

  return result;
}



} //namespace Glom
