/* Glom
 *
 * Copyright (C) 2001-2009 Murray Cumming
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
#include <glom/mode_data/box_data_details.h>
#include <glom/frame_glom.h> //For show_ok_dialog().
#include <libglom/data_structure/field.h>
#include <libglom/data_structure/relationship.h>
#include <libglom/data_structure/glomconversions.h>
#include <libglom/db_utils.h>
#include <glom/mode_design/layout/dialog_layout_details.h>
#include <glom/glade_utils.h>
#include <glom/utils_ui.h>
#include <libglom/privs.h>
#include <glom/python_embed/glom_python.h>
#include <glom/print_layout/print_layout_utils.h>
#include <glom/application.h>
#include <gtkmm/stock.h>
#include <gtkmm/viewport.h>
#include <sstream> //For stringstream
#include <glibmm/i18n.h>

namespace Glom
{

Box_Data_Details::Box_Data_Details(bool bWithNavButtons /* = true */)
: m_hbox_content(Gtk::ORIENTATION_HORIZONTAL, Utils::DEFAULT_SPACING_SMALL),
  m_hbox_buttons(Gtk::ORIENTATION_HORIZONTAL),
  m_Button_New(Gtk::Stock::ADD),
  m_Button_Del(Gtk::Stock::DELETE),
  m_Button_Nav_First(Gtk::Stock::GOTO_FIRST),
  m_Button_Nav_Prev(Gtk::Stock::GO_BACK),
  m_Button_Nav_Next(Gtk::Stock::GO_FORWARD),
  m_Button_Nav_Last(Gtk::Stock::GOTO_LAST),
  m_bDoNotRefreshRelated(false),
  m_ignore_signals(true)
#ifndef GLOM_ENABLE_CLIENT_ONLY
  , m_design_mode(false)
#endif
{
  m_layout_name = "details";

  m_hbox_buttons.set_layout(Gtk::BUTTONBOX_END);
  m_hbox_buttons.set_spacing(Utils::DEFAULT_SPACING_SMALL);

  add_view(&m_FlowTable); //Allow this to access the document too.

  m_FlowTable.set_lines(1); //Sub-groups will have multiple columns (by default, there is one sub-group, with 2 columns).

  m_FlowTable.set_horizontal_spacing(Utils::DEFAULT_SPACING_SMALL); //The default anyway.
  m_FlowTable.set_vertical_spacing(Utils::DEFAULT_SPACING_SMALL); //The default anyway.

  //m_strHint = _("When you change the data in a field the database is updated immediately.\n Click [New] to add a new record.\n Leave automatic ID fields empty - they will be filled for you.");


  //m_ScrolledWindow.set_border_width(Utils::DEFAULT_SPACING_SMALL);

  // Allow vertical scrolling, but never scroll horizontally:
  m_ScrolledWindow.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
  m_ScrolledWindow.set_shadow_type(Gtk::SHADOW_NONE); //SHADOW_IN is Recommended by the GNOME HIG, but looks odd.

#ifndef GLOM_ENABLE_CLIENT_ONLY
  m_hbox_content.pack_start(m_Dragbar, Gtk::PACK_SHRINK);
  m_Dragbar.hide();
#endif

  m_hbox_content.pack_start(m_ScrolledWindow);
  m_ScrolledWindow.add(m_FlowTable);
  // The FlowTable does not support native scrolling, so gtkmm adds it to a
  // viewport first that also has some shadow we do not want.
  Gtk::Viewport* viewport = dynamic_cast<Gtk::Viewport*>(m_FlowTable.get_parent());
  if(viewport)
    viewport->set_shadow_type(Gtk::SHADOW_NONE);


  pack_start(m_hbox_content);

  m_FlowTable.signal_field_edited().connect( sigc::mem_fun(*this,  &Box_Data_Details::on_flowtable_field_edited) );
  m_FlowTable.signal_field_open_details_requested().connect( sigc::mem_fun(*this,  &Box_Data_Details::on_flowtable_field_open_details_requested) );
  show_all();

  m_FlowTable.signal_related_record_changed().connect( sigc::mem_fun(*this, &Box_Data_Details::on_flowtable_related_record_changed) );


#ifndef GLOM_ENABLE_CLIENT_ONLY
  m_FlowTable.signal_layout_changed().connect( sigc::mem_fun(*this, &Box_Data_Details::on_flowtable_layout_changed) );
#endif // !GLOM_ENABLE_CLIENT_ONLY

  m_FlowTable.signal_requested_related_details().connect( sigc::mem_fun(*this, &Box_Data_Details::on_flowtable_requested_related_details) );

  m_FlowTable.signal_script_button_clicked().connect( sigc::mem_fun(*this, &Box_Data_Details::on_flowtable_script_button_clicked) );


  m_Button_New.set_tooltip_text(_("Create a new record."));
  m_Button_Del.set_tooltip_text(_("Remove this record."));
  m_Button_Nav_First.set_tooltip_text(_("View the first record in the list."));
  m_Button_Nav_Prev.set_tooltip_text(_("View the previous record in the list."));
  m_Button_Nav_Next.set_tooltip_text(_("View the next record in the list."));
  m_Button_Nav_Last.set_tooltip_text(_("View the last record in the list."));

  //Add or delete record:
  m_hbox_buttons.pack_start(m_Button_New, Gtk::PACK_SHRINK);
  m_hbox_buttons.pack_start(m_Button_Del,  Gtk::PACK_SHRINK);

  m_hbox_buttons.set_child_secondary(m_Button_New, true);
  m_hbox_buttons.set_child_secondary(m_Button_Del, true);

  //Link buttons to handlers:
  m_Button_New.signal_clicked().connect(sigc::mem_fun(*this, &Box_Data_Details::on_button_new));
  m_Button_Del.signal_clicked().connect(sigc::mem_fun(*this, &Box_Data_Details::on_button_del));

  //Navigation:
  if(bWithNavButtons)
  {
    m_hbox_buttons.pack_start(m_Button_Nav_First, Gtk::PACK_SHRINK);
    m_hbox_buttons.pack_start(m_Button_Nav_Prev, Gtk::PACK_SHRINK);
    m_hbox_buttons.pack_start(m_Button_Nav_Next, Gtk::PACK_SHRINK);
    m_hbox_buttons.pack_start(m_Button_Nav_Last, Gtk::PACK_SHRINK);
  }

  //Link buttons to handlers:
  m_Button_Nav_First.signal_clicked().connect(sigc::mem_fun(*this, &Box_Data_Details::on_button_nav_first));
  m_Button_Nav_Prev.signal_clicked().connect(sigc::mem_fun(*this, &Box_Data_Details::on_button_nav_prev));
  m_Button_Nav_Next.signal_clicked().connect(sigc::mem_fun(*this, &Box_Data_Details::on_button_nav_next));
  m_Button_Nav_Last.signal_clicked().connect(sigc::mem_fun(*this, &Box_Data_Details::on_button_nav_last));

  pack_start(m_hbox_buttons, Gtk::PACK_SHRINK);

  m_ignore_signals = false;
}

Box_Data_Details::~Box_Data_Details()
{
  remove_view(&m_FlowTable);
}

Gnome::Gda::Value Box_Data_Details::get_primary_key_value(const Gtk::TreeModel::iterator& /* row */) const
{
  return get_primary_key_value_selected();
}

void Box_Data_Details::set_primary_key_value(const Gtk::TreeModel::iterator& /* row */, const Gnome::Gda::Value& value)
{
  m_primary_key_value = value;
  set_found_set_from_primary_key_value();
}

void Box_Data_Details::set_found_set_from_primary_key_value()
{
  if(!m_field_primary_key)
    return;

  if(!Conversions::value_is_empty(m_primary_key_value))
  {
    m_found_set.m_where_clause = Utils::build_simple_where_expression(
       m_table_name, m_field_primary_key, m_primary_key_value);
    //std::cout << "debug: " << G_STRFUNC << ": m_found_set.m_where_clause = " << m_found_set.m_where_clause << std::endl;
  }
}

bool Box_Data_Details::init_db_details(const FoundSet& found_set, const Glib::ustring& layout_platform, const Gnome::Gda::Value& primary_key_value)
{
  //std::cout << "debug: " << G_STRFUNC << ": primary_key_value=" << primary_key_value.to_string() << std::endl;

  m_primary_key_value = primary_key_value;
  m_field_primary_key = get_field_primary_key_for_table(found_set.m_table_name);

  const bool result = Box_Data::init_db_details(found_set, layout_platform); //Calls create_layout(), then fill_from_database()

  //This is not used much, but we create it anyway:
  m_found_set = found_set; //Not used much.
  set_found_set_from_primary_key_value();

  return result;
}

bool Box_Data_Details::refresh_data_from_database_with_primary_key(const Gnome::Gda::Value& primary_key_value)
{
  m_primary_key_value = primary_key_value;
  set_found_set_from_primary_key_value();
  return fill_from_database();
}

bool Box_Data_Details::refresh_data_from_database_blank()
{
  return refresh_data_from_database_with_primary_key( Gnome::Gda::Value() );
}

void Box_Data_Details::create_layout()
{
  BusyCursor busy_cursor(get_app_window());

  Box_Data::create_layout(); //Fills m_TableFields.

  //Remove existing child widgets:
  m_FlowTable.remove_all();

  Document* document = dynamic_cast<Document*>(get_document());
  if(document)
  {
    m_FlowTable.set_table(m_table_name); //This allows portals to get full Relationship information

    //This map of layout groups will also contain the field information from the database:
    Document::type_list_layout_groups layout_groups = get_data_layout_groups(m_layout_name, m_layout_platform);

    for(Document::type_list_layout_groups::const_iterator iter = layout_groups.begin(); iter != layout_groups.end(); ++iter)
    {
      m_FlowTable.add_layout_group(*iter, false /* no indent at this top level */);
    }

    m_FlowTable.align_child_group_labels();
  }

#ifndef GLOM_ENABLE_CLIENT_ONLY
  m_FlowTable.set_design_mode(m_design_mode);
#endif
}

bool Box_Data_Details::fill_from_database()
{
  //std::cout << "debug: " << G_STRFUNC << ": m_primary_key_value=" << m_primary_key_value.to_string() << std::endl;

  //Don't try to open a connection if there is no document,
  //for instance, during application destruction.
  if(!get_document())
    return false;

  bool bResult = false;

  BusyCursor busy_cursor(get_app_window());

  const bool primary_key_is_empty = Conversions::value_is_empty(m_primary_key_value);
  if(!primary_key_is_empty)
    get_document()->set_layout_record_viewed(m_table_name, m_layout_name, m_primary_key_value);

  if(!m_field_primary_key)
  {
    //refresh_data_from_database_blank(); //shows blank record
    return false;
  }

  //TODO: This should keep the connection open, so we don't need to
  //reconnect many times..
  sharedptr<SharedConnection> sharedconnection;

  try
  {
    sharedconnection = connect_to_server(get_app_window());
  }
  catch(const Glib::Exception& ex)
  {
    handle_error(ex);
    bResult = false;
  }
  catch(const std::exception& ex)
  {
    handle_error(ex);
    bResult = false;
  }

  if(sharedconnection)
  {
    // TODO: Can this throw?
    bResult = Box_Data::fill_from_database();

    m_FieldsShown = get_fields_to_show();
    type_vecConstLayoutFields fieldsToGet = m_FieldsShown;

    if(!fieldsToGet.empty())
    {
      //Do not try to show the data if the user may not view it:
      Privileges table_privs = Privs::get_current_privs(m_table_name);

      //Enable/Disable record creation and deletion:
      m_Button_New.set_sensitive(table_privs.m_create);
      m_Button_Del.set_sensitive(table_privs.m_delete);

      if(table_privs.m_view)
      {
        //Add extra possibly-non-visible columns that we need:
        sharedptr<LayoutItem_Field> layout_item = sharedptr<LayoutItem_Field>::create();
        layout_item->set_full_field_details(m_field_primary_key);

        //Get the primary key index, adding the primary key if necessary:
        //TODO_Performance: Do this for create_layout() only, instead of repeating it for each refresh?:
        int index_primary_key = -1; //Arbitrary default.
        //g_warning("primary_key name = %s", m_field_primary_key->get_name().c_str());
        const type_vecConstLayoutFields::const_iterator iterFind = std::find_if(fieldsToGet.begin(), fieldsToGet.end(), predicate_LayoutItem_Field_IsSameField<LayoutItem_Field>(layout_item));
        if(iterFind == fieldsToGet.end())
        {
          fieldsToGet.push_back(layout_item);
          index_primary_key = fieldsToGet.size() - 1;
        }
        else
        {
          //TODO_Performance: Is there any quick way to get the index from iterFind?
          //TODO_Performance: If not, then just use this instead of the predicate.
          const type_vecLayoutFields::size_type count = fieldsToGet.size();
          for(type_vecLayoutFields::size_type i = 0; i < count; ++i)
          {
            sharedptr<const LayoutItem_Field> element = fieldsToGet[i];
            if(!element)
              continue;

            if(element->get_name() != layout_item->get_name())
              continue;

            //Compare the relationship and related relationship:
            sharedptr<const UsesRelationship> uses_a = layout_item;
            sharedptr<const UsesRelationship> uses_b = element;
            if(*uses_a == *uses_b)
            {
              index_primary_key = i;
              break;
            }
          }
        }

        Glib::RefPtr<Gnome::Gda::SqlBuilder> query = Utils::build_sql_select_with_key(m_table_name, fieldsToGet, m_field_primary_key, m_primary_key_value);
        Glib::RefPtr<Gnome::Gda::DataModel> result;

        if(!primary_key_is_empty)
          result = DbUtils::query_execute_select(query);

        if((result && result->get_n_rows()) || primary_key_is_empty) //either a working result or no result needed.
        {
          const Document* pDoc = dynamic_cast<const Document*>(get_document());
          if(pDoc)
          {
            //Get glom-specific field info:
            //Document::type_vec_fields vecFields = pDoc->get_table_fields(m_table_name);

            const int row_number = 0; //The only row.
            int cols_count = 0;
            if(!primary_key_is_empty)
              cols_count = result->get_n_columns();
            else
              cols_count = fieldsToGet.size();

            //Get special possibly-non-visible field values:
            if(!primary_key_is_empty)
            {
              if(index_primary_key < cols_count)
              {
                m_primary_key_value = result->get_value_at(index_primary_key, row_number);
                set_found_set_from_primary_key_value();
              }
            }

            //Get field values to show:
            for(int i = 0; i < cols_count; ++i)
            {
              sharedptr<const LayoutItem_Field> layout_item = fieldsToGet[i];

              //Field value:
              Gnome::Gda::Value value;

              if(!primary_key_is_empty)
                value = result->get_value_at(i, row_number);
              else
              {
                value = Conversions::get_empty_value(layout_item->get_glom_type());
              }

              m_FlowTable.set_field_value(layout_item, value);
            }
          }
        }
        else
        {
          bResult = false; //There were no records.
        }
      }
    } //if(!fieldsToGet.empty())

    //fill_related();

    set_unstored_data(false);
  }

  return bResult;
}

void Box_Data_Details::on_button_new()
{
  if(!confirm_discard_unstored_data())
    return;

  //Don't try to add a record to a list with no fields.
  if(m_FieldsShown.empty())
  {
    //Warn the user that they won't see anything if there are no fields on the layout,
    //doing an extra check:
    Document* document = get_document();
    if( document && !(document->get_data_layout_groups_have_any_fields(m_layout_name, m_table_name, m_layout_platform)) )
    {
      Gtk::Window* parent_window = get_app_window();
      if(parent_window)
        Utils::show_ok_dialog(_("Layout Contains No Fields"), _("There are no fields on the layout, so there is no way to enter data in a new record."), *parent_window, Gtk::MESSAGE_ERROR);
    }

    return;
  }

  if(m_field_primary_key && m_field_primary_key->get_auto_increment()) //If the primary key is an auto-increment:
  {
    //Just make a new record, and show it:
    const Gnome::Gda::Value primary_key_value = DbUtils::get_next_auto_increment_value(m_table_name, m_field_primary_key->get_name()); //TODO: This should return a Gda::Value

    record_new(false /* use entered field data */, primary_key_value);
    refresh_data_from_database_with_primary_key(primary_key_value);
  }
  else
  {
    //It's not an auto-increment primary key,
    //so just blank the fields ready for a primary key later.
    refresh_data_from_database_blank(); //shows blank record.
  }
}

void Box_Data_Details::on_button_del()
{
  if( Conversions::value_is_empty(get_primary_key_value_selected()) )
  {
    //Tell user that a primary key is needed to delete a record:
    Gtk::MessageDialog dialog(Utils::bold_message(_("No primary key value.")), true);
    dialog.set_secondary_text(_("This record cannot be deleted because there is no primary key."));
    dialog.set_transient_for(*get_app_window());
    dialog.run();
  }
  else
  {
    if(confirm_delete_record())
    {
      const bool bTest = record_delete(m_primary_key_value);

      if(bTest)
      {
        //Tell the list that it has been deleted:
        //It will go to the next (or last) record,
        signal_record_deleted().emit(m_primary_key_value);
      }
    }
  }
}

void Box_Data_Details::on_button_nav_first()
{
  if(confirm_discard_unstored_data())
    signal_nav_first().emit();
}

void Box_Data_Details::on_button_nav_prev()
{
  if(confirm_discard_unstored_data())
    signal_nav_prev().emit();
}

void Box_Data_Details::on_button_nav_next()
{
  if(confirm_discard_unstored_data())
    signal_nav_next().emit();
}

void Box_Data_Details::on_button_nav_last()
{
  if(confirm_discard_unstored_data())
    signal_nav_last().emit();
}

Gnome::Gda::Value Box_Data_Details::get_entered_field_data(const sharedptr<const LayoutItem_Field>& field) const
{
  return m_FlowTable.get_field_value(field);
}

void Box_Data_Details::set_entered_field_data(const sharedptr<const LayoutItem_Field>& field, const Gnome::Gda::Value& value)
{
  m_FlowTable.set_field_value(field, value);
}

void Box_Data_Details::set_entered_field_data(const Gtk::TreeModel::iterator& /* row */, const sharedptr<const LayoutItem_Field>& field, const Gnome::Gda::Value& value)
{
  set_entered_field_data(field, value);
}

Gnome::Gda::Value Box_Data_Details::get_primary_key_value_selected() const
{
  return m_primary_key_value;
}

void Box_Data_Details::recalculate_fields_for_related_records(const Glib::ustring& relationship_name)
{
  m_FieldsCalculationInProgress.clear();

  //Check all fields in the parent table:
  const Gnome::Gda::Value primary_key_value = get_primary_key_value_selected();
  for(type_vec_fields::iterator iter = m_TableFields.begin(); iter != m_TableFields.end(); ++iter)
  {
    const sharedptr<const Field> field = *iter;

    //Is this field triggered by this relationship?
    const Field::type_list_strings triggered_by = field->get_calculation_relationships();
    Field::type_list_strings::const_iterator iterFind = std::find(triggered_by.begin(), triggered_by.end(), relationship_name);
    if(iterFind != triggered_by.end()) //If it was found
    {
      sharedptr<Field> field = *iter;
      if(field)
      {
        sharedptr<LayoutItem_Field> layoutitem_field = sharedptr<LayoutItem_Field>::create();
        layoutitem_field->set_full_field_details(field);
        LayoutFieldInRecord field_in_record(layoutitem_field, m_table_name, m_field_primary_key, primary_key_value);
        calculate_field(field_in_record); //And any dependencies.

        //Calculate anything that depends on this.
        //sharedptr<LayoutItem_Field> layout_item = sharedptr<LayoutItem_Field>::create();
        //layout_item->set_full_field_details(field);

        do_calculations(field_in_record, false /* recurse, reusing m_FieldsCalculationInProgress */);
      }
    }
  }

   m_FieldsCalculationInProgress.clear();
}

void Box_Data_Details::on_related_record_added(Gnome::Gda::Value /* strKeyValue */, Glib::ustring /* strFromKeyName */)
{
  //Prevent deletion of Related boxes.
  //One of them emitted this signal, and is probably still being edited.
  //This prevents a crash.
  bool bDoNotRefreshRelated = m_bDoNotRefreshRelated;
  m_bDoNotRefreshRelated = true;

  //std::cout << "debug: " << G_STRFUNC << ": " << strKeyValue << ", " << strFromKeyName << std::endl;
  //Get current FromKey value:

 /* TODO_port
  guint iKey = 0;
  bool bTest = get_field_index(strFromKeyName, iKey);
  Glib::ustring strFromKeyValue = get_entered_field_data(iKey).get_data();

  if(strFromKeyValue.size() == 0)
  {
    //Set the From key value, to link the new related record (the first one so far) with the parent record.
    m_AddDel.set_value(iKey, m_ColumnValue, strKeyValue);
    on_adddel_user_changed(iKey, m_ColumnValue); //Update the database.
  }
  */



  //Restore value:
  m_bDoNotRefreshRelated = bDoNotRefreshRelated;
}

Box_Data_Details::type_signal_void Box_Data_Details::signal_nav_first()
{
  return m_signal_nav_first;
}

Box_Data_Details::type_signal_void Box_Data_Details::signal_nav_prev()
{
  return m_signal_nav_prev;
}

Box_Data_Details::type_signal_void Box_Data_Details::signal_nav_next()
{
  return m_signal_nav_next;
}

Box_Data_Details::type_signal_void Box_Data_Details::signal_nav_last()
{
  return m_signal_nav_last;
}

Box_Data_Details::type_signal_record_deleted Box_Data_Details::signal_record_deleted()
{
  return m_signal_record_deleted;
}

Box_Data_Details::type_signal_requested_related_details Box_Data_Details::signal_requested_related_details()
{
  return m_signal_requested_related_details;
}

#ifndef GLOM_ENABLE_CLIENT_ONLY
void Box_Data_Details::on_flowtable_layout_changed()
{
  //Get new layout:
#if 0
  Document::type_list_layout_groups layout_groups;
  m_FlowTable.get_layout_groups(layout_groups);

  //Store it in the document:
  Document* document = get_document();
  if(document)
    document->set_data_layout_groups(m_layout_name, m_table_name, m_layout_platform, layout_groups);
  //Build the view again from the new layout:
#endif
  create_layout();

  //Store it in the document:
  Document* document = get_document();
  if(document)
    document->set_modified();


  //And fill it with data:
  fill_from_database();
}
#endif // !GLOM_ENABLE_CLIENT_ONLY

void Box_Data_Details::on_flowtable_requested_related_details(const Glib::ustring& table_name, Gnome::Gda::Value primary_key_value)
{
  if(Conversions::value_is_empty(primary_key_value))
    return; //Ignore empty ID fields.

  signal_requested_related_details().emit(table_name, primary_key_value);
}

void Box_Data_Details::on_flowtable_related_record_changed(const Glib::ustring& relationship_name)
{
  recalculate_fields_for_related_records(relationship_name);
}

void Box_Data_Details::on_flowtable_field_open_details_requested(const sharedptr<const LayoutItem_Field>& layout_field, const Gnome::Gda::Value& field_value)
{
  if(Conversions::value_is_empty(field_value))
    return; //Ignore empty ID fields.

  //Updating doesn't seem necessary. The field details seem to be full already.
  //Update the field details from the document:
  ////sharedptr<LayoutItem_Field> unconst_field = sharedptr<LayoutItem_Field>::cast_const(layout_field); //A hack, because layout_field_should_have_navigation() needs to get full field details.
  //unconst_field->set_full_field_details(
  //  document->get_field(field->get_table_used(table_name), field->get_name()) ); //Otherwise get_primary_key() returns false always.
      
  sharedptr<Relationship> field_used_in_relationship_to_one;
  const bool has_open_button = 
    DbUtils::layout_field_should_have_navigation(m_table_name, layout_field, get_document(), 
    field_used_in_relationship_to_one);
         
  //If it's a simple field that is part of a relationship,
  //identifying a related record.
  if(field_used_in_relationship_to_one)
  {
    signal_requested_related_details().emit(field_used_in_relationship_to_one->get_to_table(), field_value);
    return;
  }

  //If it is a related field that is a primary key,
  //meaning it identifies a record in another table:
  if(has_open_button)
  {
    signal_requested_related_details().emit(layout_field->get_table_used(m_table_name), field_value);
  }
}

void Box_Data_Details::on_flowtable_script_button_clicked(const sharedptr<const LayoutItem_Button>& layout_item)
{
  if(layout_item)
  {
    const Gnome::Gda::Value primary_key_value = get_primary_key_value_selected();
    const Glib::ustring table_name_before = m_table_name;
    execute_button_script(layout_item, primary_key_value);

    //Refresh the view, in case the script changed any data,
    //but not if the script navigated away:
    if(m_table_name != table_name_before)
      return;

    //(m_primary_key_value seems to be NULL here. We can use primary_key_value instead, but it's a bit strange. murrayc.)
    if(get_primary_key_is_in_foundset(m_found_set, primary_key_value)) //Check, because maybe the script deleted the current record, or changed something so that it should no longer be shown in the found set.
    {
      refresh_data_from_database_with_primary_key(primary_key_value);
    }
    else
    {
      //Tell the parent to do something appropriate, such as show another record:
      signal_record_deleted().emit(primary_key_value);
    }
  }
}

void Box_Data_Details::on_flowtable_field_edited(const sharedptr<const LayoutItem_Field>& layout_field, const Gnome::Gda::Value& field_value)
{
  if(m_ignore_signals)
    return;

  const Glib::ustring strFieldName = layout_field->get_name();

  Gtk::Window* window = get_app_window();

  Document* document = dynamic_cast<Document*>(get_document());

  Gnome::Gda::Value primary_key_value = get_primary_key_value_selected();
  //std::cout << "debug: " << G_STRFUNC << ": primary_key_value=" << primary_key_value.to_string() << std::endl;
  if(!Conversions::value_is_empty(primary_key_value)) //If there is not a stored primary key value yet:
  {
    Glib::ustring table_name;
    sharedptr<Field> primary_key_field;
    Gnome::Gda::Value primary_key_value;

    if(!layout_field->get_has_relationship_name())
    {
      table_name = get_table_name();
      primary_key_field = m_field_primary_key;
      primary_key_value = get_primary_key_value_selected();
    }
    else
    {
      //If it's a related field then discover the actual table that it's in,
      //plus how to identify the record in that table.
      const Glib::ustring relationship_name = layout_field->get_relationship_name();

      sharedptr<Relationship> relationship = document->get_relationship(get_table_name(), relationship_name);
      if(relationship)
      {
        table_name = relationship->get_to_table();
        const Glib::ustring to_field_name = relationship->get_to_field();
        //Get the key field in the other table (the table that we will change)
        primary_key_field = DbUtils::get_fields_for_table_one_field(document, table_name, to_field_name); //TODO_Performance.
        if(primary_key_field)
        {
          //Get the value of the corresponding key in the current table (that identifies the record in the table that we will change)
          sharedptr<LayoutItem_Field> layout_item = sharedptr<LayoutItem_Field>::create();
          layout_item->set_full_field_details( document->get_field(relationship->get_from_table(), relationship->get_from_field()) );

          primary_key_value = get_entered_field_data(layout_item);

          //Note: This just uses an existing record if one already exists:
          Gnome::Gda::Value primary_key_value_used;
          const bool test = add_related_record_for_field(layout_field, relationship, primary_key_field, primary_key_value, primary_key_value_used);
          if(!test)
            return;

          //Get the new primary_key_value if it has been created:
          primary_key_value = primary_key_value_used;

          //Now that the related record exists, the following code to set the value of the other field in the related field can succeed.
        }
        else
        {
          g_warning("Box_Data_Details::on_flowtable_field_edited(): key not found for edited related field.");
        }
      }
    }


    LayoutFieldInRecord field_in_record(layout_field, m_table_name /* parent table */, primary_key_field, primary_key_value);

    //Check whether the value meets uniqueness constraints:
    if(!check_entered_value_for_uniqueness(m_table_name, layout_field, field_value, window))
    {
      //Revert to the value in the database:
      const Gnome::Gda::Value value_old = get_field_value_in_database(field_in_record, window);
      set_entered_field_data(layout_field, value_old);

      return;
    }

    //Set the value in all instances of this field in the layout (The field might be on the layout more than once):
    //We don't need to set the value in the layout_field itself, as this is where the value comes from.
    m_FlowTable.set_other_field_value(layout_field, field_value);

    //Update the field in the record (the record with this primary key):

    bool bTest = false;
    try
    {
      bTest = set_field_value_in_database(field_in_record, field_value, false /* don't use current calculations */, get_app_window());
    }
    catch(const Glib::Exception& ex)
    {
      handle_error(ex);
    }
    catch(const std::exception& ex)
    {
      handle_error(ex);
    }

    try
    {
      if(!bTest)
      {
        //Update failed.
        //Replace with correct values.
        const Gnome::Gda::Value value_old = get_field_value_in_database(field_in_record, window);
        set_entered_field_data(layout_field, value_old);
      }
      else
      {
        //TODO: Display new values for related fields.

        //If this is a foreign key then refresh the related records:
        /*
        bool bIsForeignKey = false;
        Document::type_vec_relationships vecRelationships = get_document()->get_relationships(m_table_name);
        for(Document::type_vec_relationships::iterator iter = vecRelationships.begin(); iter != vecRelationships.end(); ++iter)
        {
          const Relationship& relationship = *iter;

          if(relationship->get_from_field() == strFieldName)
          {
            bIsForeignKey = true;
            break;
          }
        }

        if(bIsForeignKey)
          fill_related();
        */
      }

    }
    catch(const Glib::Exception& ex)
    {
      handle_error(ex);
    }
    catch(const std::exception& ex)
    {
      handle_error(ex);
    }
  }
  else
  {
    //There is no current stored primary key value yet:

    if(m_field_primary_key && m_field_primary_key->get_auto_increment()) //If the primary key is an auto-increment:
    {
      if(strFieldName == m_field_primary_key->get_name()) //If edited field is the primary key.
      {
        //Warn user that they can't choose their own primary key:
        Gtk::MessageDialog dialog(Utils::bold_message(_("Primary key auto increments")), true);
        dialog.set_secondary_text(_("The primary key is auto-incremented.\n You may not enter your own primary key value."));
        dialog.set_transient_for(*get_app_window());
        dialog.run();
      }
      else
      {
        //Make a new record, and show it:
        const Gnome::Gda::Value primary_key_value = DbUtils::get_next_auto_increment_value(m_table_name, m_field_primary_key->get_name());

        record_new(true /* use entered field data */, primary_key_value);
        refresh_data_from_database_with_primary_key(primary_key_value);
      }
    }
    else
    {
      //It is not auto-generated:
      if(m_field_primary_key && strFieldName == m_field_primary_key->get_name()) //if it is the primary key that is being edited.
      {
        if(!check_entered_value_for_uniqueness(m_table_name, layout_field, field_value, window))
        {
          //Revert to a blank value:
          const Gnome::Gda::Value value_old = Conversions::get_empty_value(layout_field->get_full_field_details()->get_glom_type());
          set_entered_field_data(layout_field, value_old);
        }
        else
        {
          //Create new record with this primary key,
          //and all the other field values too.
          //see comments after 'else':
          record_new(true /* use entered field data */);
        }
      }
      else
      {
        //The record does not exist yet.
        //The values in the other fields will have to wait
        //until the primary key is set by the user.

        set_unstored_data(true); //Cause a warning if this is never put into the database.
      }
    }
  } //if(get_primary_key_value_selected().size())
}

void Box_Data_Details::on_userlevel_changed(AppState::userlevels user_level)
{
#ifndef GLOM_ENABLE_CLIENT_ONLY
  m_design_mode = ( user_level == AppState::USERLEVEL_DEVELOPER );
  m_FlowTable.set_design_mode(m_design_mode);

  // Recreate the layout to correctly set the size of empty flowtables:
  init_db_details(m_found_set, m_layout_platform, m_primary_key_value);
#endif
}

sharedptr<Field> Box_Data_Details::get_field_primary_key() const
{
  return m_field_primary_key;
}

void Box_Data_Details::print_layout()
{
  const Privileges table_privs = Privs::get_current_privs(m_table_name);

  //Don't try to print tables that the user can't view.
  if(!table_privs.m_view)
    return;  //TODO: Warn the user.
   
  const Document* document = dynamic_cast<const Document*>(get_document());
  if(!document)
  {
    std::cerr << G_STRFUNC << ": document was null" << std::endl;
    return;
  }

  Glib::RefPtr<Gtk::PageSetup> page_setup = Gtk::PageSetup::create(); //TODO: m_canvas.get_page_setup();
  if(!page_setup)
  {
    std::cerr << G_STRFUNC << ": page_setup was null" << std::endl;
    return;
  }

  //Note that we initially create the page layout without spaces for page 
  //breaks because those spaces would be empty space on the page after
  //we have moved items down when expanding:
  //TODO: Squash that space when expanding custom layouts.
  sharedptr<PrintLayout> print_layout = 
    PrintLayoutUtils::create_standard(page_setup, m_table_name, document,
      false /* do not avoid page margins */);
  
  //Show the print preview window:
  Application* app = Application::get_application();
  PrintLayoutUtils::do_print_layout(print_layout, m_found_set,
    false /* not preview */, document, true /* avoid page margins */, app);
}

#ifndef GLOM_ENABLE_CLIENT_ONLY
Dialog_Layout* Box_Data_Details::create_layout_dialog() const
{
  Dialog_Layout_Details* dialog = 0;
  Glom::Utils::get_glade_widget_derived_with_warning(dialog);
  return dialog;
}

void Box_Data_Details::prepare_layout_dialog(Dialog_Layout* dialog)
{
  if(dialog)
    dialog->set_document(m_layout_name, m_layout_platform, get_document(), m_table_name, m_FieldsShown); //TODO: Use m_TableFields?
}

void Box_Data_Details::show_layout_toolbar(bool show)
{
  if(show)
    m_Dragbar.show();
  else
    m_Dragbar.hide();
}

#endif // !GLOM_ENABLE_CLIENT_ONLY

void Box_Data_Details::do_new_record()
{
  on_button_new();
}

void Box_Data_Details::set_enable_drag_and_drop(bool enabled)
{
  m_FlowTable.set_enable_drag_and_drop(enabled);
}

} //namespace Glom
