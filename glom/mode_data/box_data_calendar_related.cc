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

#include <glom/mode_data/box_data_calendar_related.h>
#include <glom/mode_design/layout/dialog_layout_calendar_related.h>
#include <glom/utils_ui.h>
#include <glom/application.h>
#include <libglom/data_structure/glomconversions.h>
#include <glom/frame_glom.h> //For show_ok_dialog()
#include <glom/glade_utils.h>
#include <glibmm/i18n.h>

namespace Glom
{

Box_Data_Calendar_Related::Box_Data_Calendar_Related()
: m_pMenuPopup(0),
  m_query_column_date_field(-1)
{
  set_size_request(400, -1); //An arbitrary default.

  m_Alignment.add(m_calendar);
  m_calendar.show();

  //m_calendar.set_show_details();
  m_calendar.set_detail_width_chars(7);
  m_calendar.set_detail_height_rows(2);

  //Tell the calendar how to get the record details to show:
  m_calendar.set_detail_func( sigc::mem_fun(*this, &Box_Data_Calendar_Related::on_calendar_details) );

  m_calendar.signal_month_changed().connect( sigc::mem_fun(*this, &Box_Data_Calendar_Related::on_calendar_month_changed) );

  setup_menu();
  //m_calendar.add_events(Gdk::BUTTON_PRESS_MASK); //Allow us to catch button_press_event and button_release_event
  m_calendar.signal_button_press_event().connect_notify( sigc::mem_fun(*this, &Box_Data_Calendar_Related::on_calendar_button_press_event) );

  m_layout_name = "list_related_calendar"; //TODO: We need a unique name when 2 portals use the same table.
}

Box_Data_Calendar_Related::~Box_Data_Calendar_Related()
{
  clear_cached_database_values();
}

void Box_Data_Calendar_Related::enable_buttons()
{
  //const bool view_details_possible = get_has_suitable_record_to_view_details();
  //m_calendar.set_allow_view_details(view_details_possible); //Don't allow the user to go to a record in a hidden table.
}

bool Box_Data_Calendar_Related::init_db_details(const sharedptr<const LayoutItem_Portal>& portal, bool show_title)
{
  //This calls the other method overload:
  return Box_Data_Portal::init_db_details(portal, show_title);
}

bool Box_Data_Calendar_Related::init_db_details(const Glib::ustring& parent_table, bool show_title)
{
  //std::cout << "DEBUG: Box_Data_Calendar_Related::init_db_details(): " << parent_table << std::endl;

  m_parent_table = parent_table;

  if(m_portal)
    LayoutWidgetBase::m_table_name = m_portal->get_table_used(Glib::ustring() /* parent table_name, not used. */);
  else
    LayoutWidgetBase::m_table_name = Glib::ustring();

  Base_DB_Table::m_table_name = LayoutWidgetBase::m_table_name;

  //TODO: This is duplicated in box_data_related_list.cc and box_data_portal.cc. Just use code from the base class?
  if(show_title)
  {
    Glib::ustring relationship_title;
    if(m_portal && m_portal->get_has_relationship_name())
      relationship_title = m_portal->get_title_used(Glib::ustring() /* parent title - not relevant */);
    else
    {
      //Note to translators: This text is shown instead of a table title, when the table has not yet been chosen.
      relationship_title = _("Undefined Table");
    }

    m_Label.set_markup(Utils::bold_message(relationship_title));
    m_Label.show();

    m_Alignment.set_padding(Utils::DEFAULT_SPACING_SMALL /* top */, 0, Utils::DEFAULT_SPACING_LARGE /* left */, 0);
  }
  else
  {
    m_Label.set_markup(Glib::ustring());
    m_Label.hide();

    m_Alignment.set_padding(0, 0, 0, 0); //The box itself has padding of 6.
  }

  if(m_portal)
    m_key_field = get_fields_for_table_one_field(LayoutWidgetBase::m_table_name, m_portal->get_to_field_used());
  else
    m_key_field.clear();

  enable_buttons();

  FoundSet found_set;
  found_set.m_table_name = LayoutWidgetBase::m_table_name;
  return Box_Data::init_db_details(found_set, "" /* layout_platform */); //Calls create_layout() and fill_from_database().
}

bool Box_Data_Calendar_Related::fill_from_database()
{
  if(!m_portal)
    return false;

  bool result = false;

  if(m_key_field && m_found_set.m_where_clause.empty()) //There's a key field, but no value.
  {
    //No Foreign Key value, so just show the field names:

    result = Base_DB_Table_Data::fill_from_database();

    //create_layout();
  }
  else
  {
    if(m_query_column_date_field == -1)
      return false; //This is useless without the date in the result.

    //Create a date range from the beginning to end of the selected month:
    Glib::Date calendar_date;
    m_calendar.get_date(calendar_date);
    const Glib::Date date_start(1, calendar_date.get_month(), calendar_date.get_year());
    Glib::Date date_end = date_start;
    date_end.add_months(1);

    Gnome::Gda::Value date_start_value(date_start);
    Gnome::Gda::Value date_end_value(date_end);

    //Add a WHERE clause for this date range:
    sharedptr<Relationship> relationship = m_portal->get_relationship();
    Glib::ustring where_clause_to_table_name = relationship->get_to_table();

    sharedptr<LayoutItem_CalendarPortal> derived_portal = sharedptr<LayoutItem_CalendarPortal>::cast_dynamic(m_portal);
    const Glib::ustring date_field_name = derived_portal->get_date_field()->get_name();

    sharedptr<const Relationship> relationship_related = m_portal->get_related_relationship();
    if(relationship_related)
    {
      //Adjust the WHERE clause appropriately for the extra JOIN:
      sharedptr<UsesRelationship> uses_rel_temp = sharedptr<UsesRelationship>::create();
      uses_rel_temp->set_relationship(relationship);
      where_clause_to_table_name = uses_rel_temp->get_sql_join_alias_name();
    }

    //Add an AND to the existing where clause, to get only records within these dates, if any:
    sharedptr<const Field> date_field = derived_portal->get_date_field();
    //TODO: Use a SQL parameter instead of using sql().

    Glib::RefPtr<Gnome::Gda::SqlBuilder> builder =
      Gnome::Gda::SqlBuilder::create(Gnome::Gda::SQL_STATEMENT_SELECT);
    const guint cond = builder->add_cond(Gnome::Gda::SQL_OPERATOR_TYPE_BETWEEN,
       builder->add_id(date_field->get_name()),
       builder->add_expr_as_value(date_start_value),
       builder->add_expr_as_value(date_end_value));
    builder->set_where(cond); //Might be unnecessary.
    const Gnome::Gda::SqlExpr extra_where_clause = builder->export_expression(cond);

    Gnome::Gda::SqlExpr where_clause;
    if(m_found_set.m_where_clause.empty())
    {
      where_clause = extra_where_clause;
    }
    else
    {
      where_clause = Utils::build_combined_where_expression(
        m_found_set.m_where_clause, extra_where_clause,
        Gnome::Gda::SQL_OPERATOR_TYPE_AND);
    }

    //Do one SQL query for the whole month and store the cached values here:
    clear_cached_database_values();

    Glib::RefPtr<Gnome::Gda::SqlBuilder> sql_query = Utils::build_sql_select_with_where_clause(m_found_set.m_table_name, m_FieldsShown, where_clause, m_found_set.m_extra_join, m_found_set.m_sort_clause, m_found_set.m_extra_group_by);
    //std::cout << "DEBUG: sql_query=" << sql_query << std::endl;
    Glib::RefPtr<const Gnome::Gda::DataModel> datamodel = query_execute_select(sql_query);
    if(!(datamodel))
      return true;

    const int rows_count = datamodel->get_n_rows();
    if(!(rows_count > 0))
      return true;

    //Get the data:
    for(int row_index = 0; row_index < rows_count; ++row_index)
    {
      const int columns_count = datamodel->get_n_columns();
      if(m_query_column_date_field > columns_count)
       continue;

      //Get the date value for this row:
#ifdef GLIBMM_EXCEPTIONS_ENABLED
      Gnome::Gda::Value value_date = datamodel->get_value_at(m_query_column_date_field, row_index);
#else
      std::auto_ptr<Glib::Error> error;
      Gnome::Gda::Value value_date = datamodel->get_value_at(m_query_column_date_field, row_index, error);
#endif
      const Glib::Date date = value_date.get_date();

      //Get all the values for this row:
      type_vector_values* pVector = new type_vector_values(m_FieldsShown.size());
      for(int column_index = 0; column_index < columns_count; ++column_index)
      {
#ifdef GLIBMM_EXCEPTIONS_ENABLED
        (*pVector)[column_index] = datamodel->get_value_at(column_index, row_index);
#else
        std::auto_ptr<Glib::Error> error;
          (*pVector)[column_index] = datamodel->get_value_at(column_index, row_index, error);
        if (error.get())
          break;
#endif
      }

      m_map_values[date].push_back(pVector);
    }
  }


  return result;
}

void Box_Data_Calendar_Related::clear_cached_database_values()
{
  for(type_map_values::iterator iter = m_map_values.begin(); iter != m_map_values.end(); ++iter)
  {
    type_list_vectors vec = iter->second;
    for(type_list_vectors::iterator iter = vec.begin(); iter != vec.end(); ++iter)
    {
      type_vector_values* pValues = *iter;
      if(pValues)
        delete pValues;
    }
  }

  m_map_values.clear();
}

//TODO: Make this generic in Box_Data_Portal:
void Box_Data_Calendar_Related::on_record_added(const Gnome::Gda::Value& primary_key_value, const Gtk::TreeModel::iterator& row)
{
  //primary_key_value is a new autogenerated or human-entered key for the row.
  //It has already been added to the database.

  if(!row)
    return;

  Gnome::Gda::Value key_value;

  if(m_key_field)
  {
    //m_key_field is the field in this table that must match another field in the parent table.
    sharedptr<LayoutItem_Field> layout_item = sharedptr<LayoutItem_Field>::create();
    layout_item->set_full_field_details(m_key_field);
    //TODO: key_value = m_calendar.get_value(row, layout_item);
  }

  //Make sure that the new related record is related,
  //by setting the foreign key:
  //If it's not auto-generated.
  if(!Conversions::value_is_empty(key_value)) //If there is already a value.
  {
    //It was auto-generated. Tell the parent about it, so it can make a link.
    signal_record_added.emit(key_value);
  }
  else if(Conversions::value_is_empty(m_key_value))
  {
    g_warning("Box_Data_Calendar_Related::on_record_added(): m_key_value is NULL.");
  }
  else
  {
    sharedptr<Field> field_primary_key; //TODO: = m_calendar.get_key_field();

    //Create the link by setting the foreign key
    if(m_key_field && m_portal)
    {
      Glib::RefPtr<Gnome::Gda::SqlBuilder> builder = Gnome::Gda::SqlBuilder::create(Gnome::Gda::SQL_STATEMENT_UPDATE);
      builder->set_table(m_portal->get_table_used(Glib::ustring() /* not relevant */));
      builder->add_field_value_as_value(m_key_field->get_name(), m_key_value);
      builder->set_where(
        builder->add_cond(Gnome::Gda::SQL_OPERATOR_TYPE_EQ,
          builder->add_id(field_primary_key->get_name()),
          builder->add_expr_as_value(primary_key_value)));

      const bool test = query_execute(builder);
      if(test)
      {
        //Show it on the view, if it's visible:
        sharedptr<LayoutItem_Field> layout_item = sharedptr<LayoutItem_Field>::create();
        layout_item->set_full_field_details(field_primary_key);

        //TODO: m_calendar.set_value(row, layout_item, m_key_value);
      }
    }

    //on_adddel_user_changed(row, iKey); //Update the database.
  }
}

Box_Data_Calendar_Related::type_vecLayoutFields Box_Data_Calendar_Related::get_fields_to_show() const
{
  type_vecLayoutFields layout_fields = Box_Data_Portal::get_fields_to_show();

  sharedptr<LayoutItem_CalendarPortal> derived_portal = sharedptr<LayoutItem_CalendarPortal>::cast_dynamic(m_portal);
  if(!derived_portal)
    return layout_fields;

  sharedptr<const Field> date_field = derived_portal->get_date_field();
  if(!date_field)
    return layout_fields;

  //Add it to the list to ensure that we request the date (though it will not really be shown in the calendar):
  sharedptr<LayoutItem_Field> layout_item_date_field = sharedptr<LayoutItem_Field>::create();
  layout_item_date_field->set_full_field_details(date_field);
  layout_fields.push_back(layout_item_date_field);
  m_query_column_date_field = layout_fields.size() - 1;
  return layout_fields;
}

#ifndef GLOM_ENABLE_CLIENT_ONLY
void Box_Data_Calendar_Related::on_dialog_layout_hide()
{
  Dialog_Layout_Calendar_Related* dialog_related = dynamic_cast<Dialog_Layout_Calendar_Related*>(m_pDialogLayout);
  g_assert(dialog_related);
  m_portal = dialog_related->get_portal_layout();


  //Update the UI:
  sharedptr<LayoutItem_CalendarPortal> derived_portal = sharedptr<LayoutItem_CalendarPortal>::cast_dynamic(m_portal);
  init_db_details(derived_portal);

  Box_Data::on_dialog_layout_hide();

  sharedptr<LayoutItem_CalendarPortal> pLayoutItem = sharedptr<LayoutItem_CalendarPortal>::cast_dynamic(get_layout_item());
  if(pLayoutItem)
  {
    sharedptr<LayoutItem_CalendarPortal> derived_portal = sharedptr<LayoutItem_CalendarPortal>::cast_dynamic(m_portal);
    if(derived_portal)
      *pLayoutItem = *derived_portal;

    signal_layout_changed().emit(); //TODO: Check whether it has really changed.
  }
}
#endif // !GLOM_ENABLE_CLIENT_ONLY

#ifndef GLOM_ENABLE_CLIENT_ONLY
Dialog_Layout* Box_Data_Calendar_Related::create_layout_dialog() const
{
  Dialog_Layout_Calendar_Related* dialog = 0;
  Glom::Utils::get_glade_widget_derived_with_warning(dialog);
  return dialog;
}

void Box_Data_Calendar_Related::prepare_layout_dialog(Dialog_Layout* dialog)
{
  Dialog_Layout_Calendar_Related* related_dialog = dynamic_cast<Dialog_Layout_Calendar_Related*>(dialog);
  g_assert(related_dialog);

  sharedptr<LayoutItem_CalendarPortal> derived_portal = sharedptr<LayoutItem_CalendarPortal>::cast_dynamic(m_portal);
  if(derived_portal && derived_portal->get_has_relationship_name())
  {
    related_dialog->set_document(m_layout_name, m_layout_platform, get_document(), derived_portal);
  }
  else
  {
    related_dialog->set_document(m_layout_name, m_layout_platform, get_document(), m_parent_table);
  }
}
#endif // !GLOM_ENABLE_CLIENT_ONLY

void Box_Data_Calendar_Related::on_calendar_month_changed()
{
  //Update the cached values for the new month:
  fill_from_database();
}

Glib::ustring Box_Data_Calendar_Related::on_calendar_details(guint year, guint month, guint day)
{
  sharedptr<LayoutItem_CalendarPortal> derived_portal = sharedptr<LayoutItem_CalendarPortal>::cast_dynamic(m_portal);
  if(!derived_portal)
  {
    //std::cout << "DEBUG: Box_Data_Calendar_Related::on_calendar_details(): date_field is NULL" << std::endl;
    return Glib::ustring();
  }

  sharedptr<const Field> date_field = derived_portal->get_date_field();
  if(!date_field)
    return Glib::ustring();

  //TODO: month seems to be 143710360 sometimes, which seems to be a GtkCalendar bug:
  //std::cout << "Box_Data_Calendar_Related::on_calendar_details(): year=" << year << ", month=" << month << " day=" << day << std::endl;

  //Glib::Date is 1-indexed:
  Glib::Date::Month datemonth = (Glib::Date::Month)(month +1);
  if(datemonth > Glib::Date::DECEMBER)
    datemonth = Glib::Date::JANUARY;
  Glib::Date date(day, datemonth, year);

  //Examine the cached data:
  type_map_values::const_iterator iter_find = m_map_values.find(date);
  if(iter_find == m_map_values.end())
    return Glib::ustring(); //No data was found for this date.


  Glib::ustring result;

  //Look at each row for this date:
  const type_list_vectors& rows = iter_find->second;
  for(type_list_vectors::const_iterator iter = rows.begin(); iter != rows.end(); ++iter)
  {
    type_vector_values* pRow = *iter;
    if(!pRow)
      continue;

    //Get the data for each column in the row:
    Glib::ustring row_text;
    int column_index = 0;

    //We iterate over the original list of items from the portal,
    //instead of the ones used by the query (m_FieldsShown),
    //because we really don't want to show the extra fields (at the end) to the user:
    LayoutGroup::type_list_items items = m_portal->get_items();
    for(LayoutGroup::type_list_items::const_iterator iter = items.begin(); iter != items.end(); ++iter)
    {
      sharedptr<const LayoutItem> layout_item = *iter;
      if(!layout_item)
        continue;

      Glib::ustring text;

      //Text for a text item:
      sharedptr<const LayoutItem_Text> layout_item_text = sharedptr<const LayoutItem_Text>::cast_dynamic(layout_item);
      if(layout_item_text)
        text = layout_item_text->get_text();
      else
      {
        //Text for a field:
        sharedptr<const LayoutItem_Field> layout_item_field = sharedptr<const LayoutItem_Field>::cast_dynamic(layout_item);

        const Gnome::Gda::Value value = (*pRow)[column_index];
        text = Conversions::get_text_for_gda_value(layout_item_field->get_glom_type(), value, layout_item_field->get_formatting_used().m_numeric_format);

        ++column_index;
      }

      //Add the field text to the row:
      if(!text.empty())
      {
        if(!row_text.empty())
          row_text += ", "; //TODO: Internationalization?

        row_text += text;
      }
    }

    //Add the row text to the result:
    if(!row_text.empty())
    {
      if(!result.empty())
        result += '\n';

      result += row_text;
    }
  }

  return result;
}

void Box_Data_Calendar_Related::setup_menu()
{
  m_refActionGroup = Gtk::ActionGroup::create();

  m_refActionGroup->add(Gtk::Action::create("ContextMenu", "Context Menu") );

  m_refContextEdit =  Gtk::Action::create("ContextEdit", Gtk::Stock::EDIT);

  m_refActionGroup->add(m_refContextEdit,
    sigc::mem_fun(*this, &Box_Data_Calendar_Related::on_MenuPopup_activate_Edit) );

#ifndef GLOM_ENABLE_CLIENT_ONLY
  // Don't add ContextLayout in client only mode because it would never
  // be sensitive anyway
  m_refContextLayout =  Gtk::Action::create("ContextLayout", _("Layout"));
  m_refActionGroup->add(m_refContextLayout,
    sigc::mem_fun(*this, &Box_Data_Calendar_Related::on_MenuPopup_activate_layout) );

  //TODO: This does not work until this widget is in a container in the window:
  Application* pApp = get_application();
  if(pApp)
  {
    pApp->add_developer_action(m_refContextLayout); //So that it can be disabled when not in developer mode.
    pApp->update_userlevel_ui(); //Update our action's sensitivity.
  }
#endif // !GLOM_ENABLE_CLIENT_ONLY

  m_refUIManager = Gtk::UIManager::create();

  m_refUIManager->insert_action_group(m_refActionGroup);

  //TODO: add_accel_group(m_refUIManager->get_accel_group());

#ifdef GLIBMM_EXCEPTIONS_ENABLED
  try
  {
#endif
    Glib::ustring ui_info =
        "<ui>"
        "  <popup name='ContextMenu'>"
        "    <menuitem action='ContextEdit'/>"
#ifndef GLOM_ENABLE_CLIENT_ONLY
        "    <menuitem action='ContextLayout'/>"
#endif
        "  </popup>"
        "</ui>";

#ifdef GLIBMM_EXCEPTIONS_ENABLED
    m_refUIManager->add_ui_from_string(ui_info);
  }
  catch(const Glib::Error& ex)
  {
    std::cerr << "building menus failed: " <<  ex.what();
  }
#else
  std::auto_ptr<Glib::Error> error;
  m_refUIManager->add_ui_from_string(ui_info, error);
  if(error.get())
  {
    std::cerr << "building menus failed: " << error->what();
  }
#endif //GLIBMM_EXCEPTIONS_ENABLED

  //Get the menu:
  m_pMenuPopup = dynamic_cast<Gtk::Menu*>( m_refUIManager->get_widget("/ContextMenu") );
  if(!m_pMenuPopup)
    g_warning("menu not found");


#ifndef GLOM_ENABLE_CLIENT_ONLY
  if(pApp)
    m_refContextLayout->set_sensitive(pApp->get_userlevel() == AppState::USERLEVEL_DEVELOPER);
#endif // !GLOM_ENABLE_CLIENT_ONLY
}

void Box_Data_Calendar_Related::on_calendar_button_press_event(GdkEventButton *event)
{
#ifndef GLOM_ENABLE_CLIENT_ONLY
  //Enable/Disable items.
  //We did this earlier, but get_application is more likely to work now:
  Application* pApp = get_application();
  if(pApp)
  {
    pApp->add_developer_action(m_refContextLayout); //So that it can be disabled when not in developer mode.
    pApp->update_userlevel_ui(); //Update our action's sensitivity.
  }
#endif

  GdkModifierType mods;
  gdk_window_get_pointer( gtk_widget_get_window(Gtk::Widget::gobj()), 0, 0, &mods );
  if(mods & GDK_BUTTON3_MASK)
  {
    //Give user choices of actions on this item:
    m_pMenuPopup->popup(event->button, event->time);
    return; //handled.
  }
  else
  {
    if(event->type == GDK_2BUTTON_PRESS)
    {
      //Double-click means edit.
      //Don't do this usually, because users sometimes double-click by accident when they just want to edit a cell.

      //TODO: If the cell is not editable, handle the double-click as an edit/selection.
      //on_MenuPopup_activate_Edit();
      return; //Not handled.
    }
  }

  return; //Not handled. TODO: Call base class?
}

void
Box_Data_Calendar_Related::on_MenuPopup_activate_Edit()
{
  const Gnome::Gda::Value primary_key_value; //TODO: = m_AddDel.get_value_key(row); //The primary key is in the key.

  signal_user_requested_details().emit(primary_key_value);
}

#ifndef GLOM_ENABLE_CLIENT_ONLY
void Box_Data_Calendar_Related::on_MenuPopup_activate_layout()
{
  show_layout_dialog();
}
#endif // !GLOM_ENABLE_CLIENT_ONLY

Gnome::Gda::Value Box_Data_Calendar_Related::get_primary_key_value(const Gtk::TreeModel::iterator& /* row */) const
{
  return Gnome::Gda::Value(); //TODO: m_AddDel.get_value_key(row);
}

Gnome::Gda::Value Box_Data_Calendar_Related::get_primary_key_value_selected() const
{
  return Gnome::Gda::Value(); //TODO: m_AddDel.get_value_key_selected();
}

void Box_Data_Calendar_Related::set_primary_key_value(const Gtk::TreeModel::iterator& /* row */, const Gnome::Gda::Value& /* value */)
{
  //TODO
}

} //namespace Glom
