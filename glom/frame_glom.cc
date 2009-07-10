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

#include <libglom/libglom_config.h> // For GLOM_ENABLE_SQLITE, GLOM_ENABLE_CLIENT_ONLY

#include "frame_glom.h"
#include "application.h"
#include "dialog_import_csv.h"
#include "dialog_import_csv_progress.h"
#include <libglom/appstate.h>

#include <libglom/connectionpool.h>

#ifdef GLOM_ENABLE_POSTGRESQL
#include <libglom/connectionpool_backends/postgres_central.h>
#include <libglom/connectionpool_backends/postgres_self.h>
#endif

#ifdef GLOM_ENABLE_SQLITE
# include <libglom/connectionpool_backends/sqlite.h>
#endif

#ifndef GLOM_ENABLE_CLIENT_ONLY
#include "mode_design/users/dialog_groups_list.h"
#include "dialog_database_preferences.h"
#include "reports/dialog_layout_report.h"
#include <glom/mode_design/print_layouts/window_print_layout_edit.h>
#include <glom/mode_design/dialog_add_related_table.h>
#include <glom/mode_design/script_library/dialog_script_library.h>
#include <glom/dialog_new_self_hosted_connection.h>
#include "relationships_overview/dialog_relationships_overview.h"
#endif // !GLOM_ENABLE_CLIENT_ONLY

#include <libglom/utils.h>
#include <libglom/glade_utils.h>
#include <libglom/data_structure/glomconversions.h>
#include <libglom/data_structure/layout/report_parts/layoutitem_summary.h>
#include <libglom/data_structure/layout/report_parts/layoutitem_fieldsummary.h>

#include <glom/reports/report_builder.h>
#ifndef GLOM_ENABLE_CLIENT_ONLY
#include <glom/mode_design/dialog_add_related_table.h>
#include <glom/mode_design/script_library/dialog_script_library.h>
#include <glom/printoperation_printlayout.h>
#endif // !GLOM_ENABLE_CLIENT_ONLY

#ifdef GLOM_ENABLE_MAEMO
#include <hildonmm/note.h>
#endif

#include "filechooser_export.h"
#include <glom/glom_privs.h>
#include <sstream> //For stringstream.
#include <fstream>
#include <glibmm/i18n.h>

namespace Glom
{

Frame_Glom::Frame_Glom(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& refGlade)
: PlaceHolder(cobject, refGlade),
  m_pLabel_Table(0),
  m_box_footer(0),
  m_pLabel_Mode(0),
  m_pLabel_userlevel(0),
  m_pBox_QuickFind(0),
  m_pEntry_QuickFind(0),
  m_pButton_QuickFind(0),
  m_pBox_RecordsCount(0),
  m_pLabel_RecordsCount(0),
  m_pLabel_FoundCount(0),
  m_pButton_FindAll(0),
  m_pBox_Mode(0),
  m_pBox_Tables(0),
  m_pDialog_Tables(0),
#ifndef GLOM_ENABLE_CLIENT_ONLY
  m_pDialog_Reports(0),
  m_pDialogLayoutReport(0),
  m_pBox_Reports(0),
  m_pDialog_PrintLayouts(0),
  m_pDialogLayoutPrint(0),
  m_pBox_PrintLayouts(0),
  m_pDialog_Fields(0),
  m_pDialog_Relationships(0),
  m_dialog_addrelatedtable(0),
  m_dialog_relationships_overview(0),
#endif // !GLOM_ENABLE_CLIENT_ONLY
  m_pDialogConnection(0)
{
  //Load widgets from glade file:
  refGlade->get_widget("label_table_name", m_pLabel_Table);

  refGlade->get_widget("hbox_footer", m_box_footer);
  refGlade->get_widget("label_mode", m_pLabel_Mode);
  refGlade->get_widget("label_user_level", m_pLabel_userlevel);
  
  //Hide the footer on maemo (It takes too much space),
  //and reduce the border width:
  #ifdef GLOM_ENABLE_MAEMO
  m_box_footer->hide();
  set_border_width(Glom::Utils::DEFAULT_SPACING_LARGE);
  #endif

  refGlade->get_widget("hbox_quickfind", m_pBox_QuickFind);
  m_pBox_QuickFind->hide();

  refGlade->get_widget("entry_quickfind", m_pEntry_QuickFind);
  m_pEntry_QuickFind->signal_activate().connect(
   sigc::mem_fun(*this, &Frame_Glom::on_button_quickfind) ); //Pressing Enter here is like pressing Find.

  refGlade->get_widget("button_quickfind", m_pButton_QuickFind);
  m_pButton_QuickFind->signal_clicked().connect(
    sigc::mem_fun(*this, &Frame_Glom::on_button_quickfind) );

  refGlade->get_widget("hbox_records_count", m_pBox_RecordsCount);
  refGlade->get_widget("label_records_count", m_pLabel_RecordsCount);
  refGlade->get_widget("label_records_found_count", m_pLabel_FoundCount);
  refGlade->get_widget("button_find_all", m_pButton_FindAll);
  m_pButton_FindAll->signal_clicked().connect(
    sigc::mem_fun(*this, &Frame_Glom::on_button_find_all) );

  refGlade->get_widget_derived("vbox_mode", m_pBox_Mode);

  m_Mode = MODE_None;
  m_Mode_Previous = MODE_None;


  m_Notebook_Find.signal_find_criteria.connect(sigc::mem_fun(*this, &Frame_Glom::on_notebook_find_criteria));
  m_Notebook_Find.show();

  m_Notebook_Data.signal_record_details_requested().connect(sigc::mem_fun(*this, &Frame_Glom::on_notebook_data_record_details_requested));
  m_Notebook_Data.signal_switch_page().connect(sigc::mem_fun(*this, &Frame_Glom::on_notebook_data_switch_page));
  m_Notebook_Data.show();

  //Fill Composite View:
  //This means that set_document and load/save are delegated to these children:
  add_view(&m_Notebook_Data); //Also a composite view.
  add_view(&m_Notebook_Find); //Also a composite view.

  on_userlevel_changed(AppState::USERLEVEL_OPERATOR); //A default to show before a document is created or loaded.
}

Frame_Glom::~Frame_Glom()
{
  if(m_pBox_Tables)
    remove_view(m_pBox_Tables);

  remove_view(&m_Notebook_Data); //Also a composite view.
  remove_view(&m_Notebook_Find); //Also a composite view.

  if(m_pDialog_Tables)
  {
    delete m_pDialog_Tables;
    m_pDialog_Tables = 0;
  }

  if(m_pDialogConnection)
  {
    remove_view(m_pDialogConnection);
    delete m_pDialogConnection;
    m_pDialogConnection = 0;
  }

#ifndef GLOM_ENABLE_CLIENT_ONLY
  if(m_pBox_Reports)
    remove_view(m_pBox_Reports);

  if(m_pBox_PrintLayouts)
    remove_view(m_pBox_PrintLayouts);

  if(m_pDialog_Relationships)
  {
    remove_view(m_pDialog_Relationships);
    delete m_pDialog_Relationships;
    m_pDialog_Relationships = 0;
  }

  if(m_pDialogLayoutReport)
  {
    remove_view(m_pDialogLayoutReport);
    delete m_pDialogLayoutReport;
    m_pDialogLayoutReport = 0;
  }

  if(m_pDialogLayoutPrint)
  {
    remove_view(m_pDialogLayoutPrint);
    delete m_pDialogLayoutPrint;
    m_pDialogLayoutPrint = 0;
  }

  if(m_pDialog_Fields)
  {
    remove_view(m_pDialog_Fields);
    delete m_pDialog_Fields;
    m_pDialog_Fields = 0;
  }

  if(m_dialog_addrelatedtable)
  {
    remove_view(m_dialog_addrelatedtable);
    delete m_dialog_addrelatedtable;
    m_dialog_addrelatedtable = 0;
  }

  if(m_dialog_relationships_overview)
  {
    remove_view(m_dialog_relationships_overview);
    delete m_dialog_relationships_overview;
    m_dialog_relationships_overview = 0;
  }
#endif // !GLOM_ENABLE_CLIENT_ONLY
}

void Frame_Glom::set_databases_selected(const Glib::ustring& strName)
{
  //m_pDialog_Databases->hide(); //cause_close();

  get_document()->set_connection_database(strName);

  //show_system_name();

  do_menu_Navigate_Table(true /* open default */);
}

void Frame_Glom::on_box_tables_selected(const Glib::ustring& strName)
{
  if(m_pDialog_Tables)
    m_pDialog_Tables->hide();

  show_table(strName);
}

void Frame_Glom::set_mode_widget(Gtk::Widget& widget)
{
  //Remove current contents.
  //I wish that there was a better way to do this:
  //Trying to remove all of them leads to warnings,
  //and I don't see a way to get a list of children.

  App_Glom* pApp = dynamic_cast<App_Glom*>(get_app_window());
  if(pApp)
  {
    Glib::RefPtr<Gtk::UIManager> ui_manager = pApp->get_ui_manager();

    Notebook_Glom* notebook_current = dynamic_cast<Notebook_Glom*>(m_pBox_Mode->get_child());
    if(notebook_current)
    {
      m_pBox_Mode->remove();
    }

    m_pBox_Mode->add(widget);
    widget.show();

    //Show help text:
    //Notebook_Glom* pNotebook = dynamic_cast<Notebook_Glom*>(&widget);
    //if(pNotebook)
   // {
   //   pNotebook->show_hint();
   // }
  }
}

bool Frame_Glom::set_mode(enumModes mode)
{
  //TODO: This seems to be called twice when changing mode.
  const bool changed = (m_Mode != mode);

  //Choose a default mode, if necessary:
  if(mode == MODE_None)
    mode = MODE_Data;

  m_Mode_Previous = m_Mode;
  m_Mode = mode;

  //Hide the Quick Find widgets if we are not in Find mode.
  const bool show_quickfind = (m_Mode == MODE_Find);
  if(show_quickfind)
  {
    m_pBox_QuickFind->show();

    //Clear the quick-find entry, ready for a new Find.
    if(changed)
    {
      m_pEntry_QuickFind->set_text(Glib::ustring());

      //Put the cursor in the quick find entry:
      m_pEntry_QuickFind->grab_focus();
      //m_pButton_QuickFind->grab_default();
    }

    m_pBox_RecordsCount->hide();
  }
  else
  {
    m_pBox_QuickFind->hide();

    m_pBox_RecordsCount->show();
  }

  return changed;
}

void Frame_Glom::alert_no_table()
{
  //Ask user to choose a table first:
  Gtk::Window* pWindowApp = get_app_window();
  if(pWindowApp)
  {
    //TODO: Obviously this document should have been deleted when the database-creation was cancelled.
    /* Note that "canceled" is the correct US spelling. */
    show_ok_dialog(_("No table"), _("This database has no tables yet."), *pWindowApp, Gtk::MESSAGE_WARNING);
  }
}

void Frame_Glom::show_table_refresh()
{
  show_table(m_table_name);
}

void Frame_Glom::show_table_allow_empty(const Glib::ustring& table_name, const Gnome::Gda::Value& primary_key_value_for_details)
{
  App_Glom* pApp = dynamic_cast<App_Glom*>(get_app_window());

  //This can take quite a long time, so we show the busy cursor while it's working:
  Bakery::BusyCursor busy_cursor(pApp);

  //Choose a default mode, if necessary:
  if(m_Mode == MODE_None)
    set_mode(m_Mode);

  //Show the table:
  m_table_name = table_name;
  Glib::ustring strMode;
#ifndef GLOM_ENABLE_CLIENT_ONLY
  //Update the document with any new information in the database if necessary (though the database _should never have changed information)
  update_table_in_document_from_database();
#endif // !GLOM_ENABLE_CLIENT_ONLY

  //Update user-level dependent UI:
  if(pApp)
    on_userlevel_changed(pApp->get_userlevel());

  switch(m_Mode)
  {
    case(MODE_Data):
    {
      strMode = _("Data");
      FoundSet found_set;

      //Start with the last-used found set (sort order and where clause)
      //for this layout:
      //(This would be ignored anyway if a details primary key is specified.)
      Document_Glom* document = get_document(); 
      if(document)
        found_set = document->get_criteria_current(m_table_name);

      //Make sure that this is set:
      found_set.m_table_name = m_table_name;

      //If there is no saved sort clause, 
      //then sort by the ID, just so we sort by something, so that the order is predictable:
      if(found_set.m_sort_clause.empty())
      {
        sharedptr<Field> field_primary_key = get_field_primary_key_for_table(m_table_name);
        if(field_primary_key)
      {
          sharedptr<LayoutItem_Field> layout_item_sort = sharedptr<LayoutItem_Field>::create();
          layout_item_sort->set_full_field_details(field_primary_key);

          found_set.m_sort_clause.clear();

          //Avoid the sort clause if the found set will include too many records, 
          //because that would be too slow.
          //The user can explicitly request a sort later, by clicking on a column header.
          //TODO_Performance: This causes an almost-duplicate COUNT query (we do it in the treemodel too), but it's not that slow. 
          sharedptr<LayoutItem_Field> layout_item_temp = sharedptr<LayoutItem_Field>::create();
          layout_item_temp->set_full_field_details(field_primary_key);
          type_vecLayoutFields layout_fields;
          layout_fields.push_back(layout_item_temp);
          const Glib::ustring sql_query_without_sort = Utils::build_sql_select_with_where_clause(found_set.m_table_name, layout_fields, found_set.m_where_clause, found_set.m_extra_join, type_sort_clause(), found_set.m_extra_group_by);
          const int count = Base_DB::count_rows_returned_by(sql_query_without_sort);
          if(count < 10000) //Arbitrary large number.
            found_set.m_sort_clause.push_back( type_pair_sort_field(layout_item_sort, true /* ascending */) );
        }
      }

      //Show the wanted records in the notebook, showing details for a particular record if wanted:
      m_Notebook_Data.init_db_details(found_set, primary_key_value_for_details);
      set_mode_widget(m_Notebook_Data);

      //Show how many records were found:
      update_records_count();

      break;
    }
    case(MODE_Find):
    {
      strMode = _("Find");
      m_Notebook_Find.init_db_details(m_table_name, get_document()->get_active_layout_platform());
      set_mode_widget(m_Notebook_Find);
      break;
    }
    default:
    {
      std::cout << "Frame_Glom::on_box_tables_selected(): Unexpected mode" << std::endl;
      strMode = _("Unknown");
      break;
    }
  }

  m_pLabel_Mode->set_text(strMode);

  show_table_title();

  //List the reports and print layouts in the menus:
  pApp->fill_menu_reports(table_name);
  pApp->fill_menu_print_layouts(table_name);

  //show_all();
}

void Frame_Glom::show_table(const Glib::ustring& table_name, const Gnome::Gda::Value& primary_key_value_for_details)
{
  //Check that there is a table to show:
  if(table_name.empty())
  {
    alert_no_table();
  }
  else
  {
    show_table_allow_empty(table_name, primary_key_value_for_details);
  }
}

void Frame_Glom::show_no_table()
{
  show_table_allow_empty(Glib::ustring());
}

#ifndef GLOM_ENABLE_CLIENT_ONLY
void Frame_Glom::on_menu_userlevel_Developer(const Glib::RefPtr<Gtk::RadioAction>& action, const Glib::RefPtr<Gtk::RadioAction>& operator_action)
{
  if(action && action->get_active())
  {
    Document_Glom* document = dynamic_cast<Document_Glom*>(get_document());
    if(document)
    {
      //Check whether the current user has developer privileges:
      ConnectionPool* connection_pool = ConnectionPool::get_instance();
      sharedptr<SharedConnection> sharedconnection = connection_pool->connect();

      // Default to true; if we don't support users, we always have
      // priviliges to change things in developer mode.
      bool test = true;

      if(sharedconnection && sharedconnection->get_gda_connection()->supports_feature(Gnome::Gda::CONNECTION_FEATURE_USERS))
        Privs::get_user_is_in_group(connection_pool->get_user(), GLOM_STANDARD_GROUP_NAME_DEVELOPER);

      if(test)
      {
        std::cout << "DEBUG: User=" << connection_pool->get_user() << " _is_ in the developer group on the server." << std::endl;
        //Avoid double signals:
        //if(document->get_userlevel() != AppState::USERLEVEL_DEVELOPER)
        test = document->set_userlevel(AppState::USERLEVEL_DEVELOPER);
        if(!test)
          std::cout << "  DEBUG: But document->set_userlevel(AppState::USERLEVEL_DEVELOPER) failed." << std::endl;
      }
      else
      {
        std::cout << "DEBUG: User=" << connection_pool->get_user() << " is _not_ in the developer group on the server." << std::endl;
      }

      //If this was not possible then revert the menu:
      if(!test)
      {
        if(document->get_opened_from_browse())
        {
          //TODO: Obviously this could be possible but it would require a network protocol and some work:
          Gtk::MessageDialog dialog(Bakery::App_Gtk::util_bold_message(_("Developer Mode Not Available.")), true, Gtk::MESSAGE_WARNING);
          dialog.set_secondary_text(_("Developer mode is not available because the file was opened over the network from a running Glom. Only the original file may be edited."));
          dialog.set_transient_for(*get_app_window());
          dialog.run();
        }
        else
        {
          Gtk::MessageDialog dialog(Bakery::App_Gtk::util_bold_message(_("Developer Mode Not Available")), true, Gtk::MESSAGE_WARNING);
          dialog.set_secondary_text(_("Developer mode is not available. Check that you have sufficient database access rights and that the glom file is not read-only."));
          dialog.set_transient_for(*get_app_window());
          dialog.run();
        }
      }
      else if(document->get_document_format_version() < Document_Glom::get_latest_known_document_format_version())
      {
        Gtk::MessageDialog dialog(Bakery::App_Gtk::util_bold_message(_("Saving in New Document Format")), true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE);
        dialog.set_secondary_text(_("The document was created by an earlier version of the application. Making changes to the document will mean that the document cannot be opened by some earlier versions of the application."));
        dialog.set_transient_for(*get_app_window());
        dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
        dialog.add_button(_("Continue"), Gtk::RESPONSE_OK);
        const int response = dialog.run();
        test = (response == Gtk::RESPONSE_OK);
      }
      
      if(!test)
      {
        //Abort the change of user level:

        //This causes an endless loop, but it is not recursive so we can't block it.
        //TODO: Submit GTK+ bug.
        //action->set_active(false);
        operator_action->set_active();
      }
    }
  }
}

void Frame_Glom::on_menu_userlevel_Operator(const Glib::RefPtr<Gtk::RadioAction>& action)
{
  if(action &&  action->get_active())
  {
    Document_Glom* document = dynamic_cast<Document_Glom*>(get_document());
    if(document)
    {
      //Avoid double signals:
      //if(document->get_userlevel() != AppState::USERLEVEL_OPERATOR)
        document->set_userlevel(AppState::USERLEVEL_OPERATOR);
    }
  }
}

void Frame_Glom::on_menu_file_export()
{
  //Start with a sequence based on the Details view:
  //The user can changed this by clicking the button in the FileChooser:
  Document_Glom* document = get_document();
  if(!document)
    return;

  Document_Glom::type_list_layout_groups mapGroupSequence = document->get_data_layout_groups_plus_new_fields("details", m_table_name, document->get_active_layout_platform());

  Gtk::Window* pWindowApp = get_app_window();
  g_assert(pWindowApp);

  //Do not try to export the data if the user may not view it:
  Privileges table_privs = Privs::get_current_privs(m_table_name);
  if(!table_privs.m_view)
  {
    show_ok_dialog(_("Export Not Allowed."), _("You do not have permission to view the data in this table, so you may not export the data."), *pWindowApp, Gtk::MESSAGE_ERROR);
    return;
  }

  //Ask the user for the new file location, and to optionally modify the format:
  FileChooser_Export dialog;
  dialog.set_transient_for(*get_app_window());
  dialog.set_do_overwrite_confirmation();
  dialog.set_export_layout(mapGroupSequence, m_table_name, get_document());
  const int response = dialog.run();
  dialog.hide();

  if((response == Gtk::RESPONSE_CANCEL) || (response == Gtk::RESPONSE_DELETE_EVENT))
    return;

  const std::string filepath = dialog.get_filename();
  if(filepath.empty())
    return;

  dialog.get_layout_groups(mapGroupSequence);
  //std::cout << "DEBUG 0: mapGroupSequence.size()=" << mapGroupSequence.size() << std::endl;

  //const int index_primary_key = fieldsSequence.size() - 1;

  const FoundSet found_set = m_Notebook_Data.get_found_set();

  std::fstream the_stream(filepath.c_str(), std::ios_base::out | std::ios_base::trunc);
  if(!the_stream)
  {
    show_ok_dialog(_("Could Not Create File."), _("Glom could not create the specified file."), *pWindowApp, Gtk::MESSAGE_ERROR);
    return;
  }

  export_data_to_stream(the_stream, found_set, mapGroupSequence);
}

//TODO: Reduce copy/pasting in these export_data_to_*() methods:
void Frame_Glom::export_data_to_vector(Document_Glom::type_example_rows& the_vector, const FoundSet& found_set, const Document_Glom::type_list_layout_groups& sequence)
{
  type_vecLayoutFields fieldsSequence = get_table_fields_to_show_for_sequence(found_set.m_table_name, sequence);

  if(fieldsSequence.empty())
  {
    std::cerr << "Glom: Frame_Glom::export_data_to_string(): No fields in sequence." << std::endl;
    return;
  }

  const Glib::ustring query = Utils::build_sql_select_with_where_clause(found_set.m_table_name, fieldsSequence, found_set.m_where_clause, found_set.m_extra_join, found_set.m_sort_clause, found_set.m_extra_group_by);

  //TODO: Lock the database (prevent changes) during export.
  Glib::RefPtr<Gnome::Gda::DataModel> result = query_execute_select(query);

  guint rows_count = 0;
  if(result)
    rows_count = result->get_n_rows();

  if(rows_count)
  {
    const guint columns_count = result->get_n_columns();

    for(guint row_index = 0; row_index < rows_count; ++row_index)
    {
        Document_Glom::type_row_data row_data;

        for(guint col_index = 0; col_index < columns_count; ++col_index)
        {
          const Gnome::Gda::Value value = result->get_value_at(col_index, row_index);

          sharedptr<LayoutItem_Field> layout_item = fieldsSequence[col_index];
          //if(layout_item->m_field.get_glom_type() != Field::TYPE_IMAGE) //This is too much data.
          //{

            //Output data in canonical SQL format, ignoring the user's locale, and ignoring the layout formatting:
            row_data.push_back(value);  //TODO_Performance: reserve the size.

            //if(layout_item->m_field.get_glom_type() == Field::TYPE_IMAGE) //This is too much data.
            //{
             //std::cout << "  field name=" << layout_item->get_name() << ", value=" << layout_item->m_field.sql(value) << std::endl;
            //}
        }

        //std::cout << " row_string=" << row_string << std::endl;
        the_vector.push_back(row_data); //TODO_Performance: Reserve the size.
    }
  }
}

void Frame_Glom::export_data_to_string(Glib::ustring& the_string, const FoundSet& found_set, const Document_Glom::type_list_layout_groups& sequence)
{
  type_vecLayoutFields fieldsSequence = get_table_fields_to_show_for_sequence(found_set.m_table_name, sequence);

  if(fieldsSequence.empty())
  {
    std::cerr << "Glom: Frame_Glom::export_data_to_string(): No fields in sequence." << std::endl;
    return;
  }

  const Glib::ustring query = Utils::build_sql_select_with_where_clause(found_set.m_table_name, fieldsSequence, found_set.m_where_clause, found_set.m_extra_join, found_set.m_sort_clause, found_set.m_extra_group_by);

  //TODO: Lock the database (prevent changes) during export.
  Glib::RefPtr<Gnome::Gda::DataModel> result = query_execute_select(query);

  guint rows_count = 0;
  if(result)
    rows_count = result->get_n_rows();

  if(rows_count)
  {
    const guint columns_count = result->get_n_columns();

    for(guint row_index = 0; row_index < rows_count; ++row_index)
    {
        std::string row_string;

        for(guint col_index = 0; col_index < columns_count; ++col_index)
        {
          const Gnome::Gda::Value value = result->get_value_at(col_index, row_index);

          sharedptr<LayoutItem_Field> layout_item = fieldsSequence[col_index];
          //if(layout_item->m_field.get_glom_type() != Field::TYPE_IMAGE) //This is too much data.
          //{
            if(!row_string.empty())
              row_string += ",";

            //Output data in canonical SQL format, ignoring the user's locale, and ignoring the layout formatting:
            row_string += layout_item->get_full_field_details()->to_file_format(value);

            //if(layout_item->m_field.get_glom_type() == Field::TYPE_IMAGE) //This is too much data.
            //{
             //std::cout << "  field name=" << layout_item->get_name() << ", value=" << layout_item->m_field.sql(value) << std::endl;
            //}
        }

        //std::cout << " row_string=" << row_string << std::endl;
        the_string += (row_string += "\n");
    }
  }
}

void Frame_Glom::export_data_to_stream(std::ostream& the_stream, const FoundSet& found_set, const Document_Glom::type_list_layout_groups& sequence)
{
  type_vecLayoutFields fieldsSequence = get_table_fields_to_show_for_sequence(found_set.m_table_name, sequence);

  if(fieldsSequence.empty())
  {
    std::cerr << "Glom: Frame_Glom::export_data_to_stream(): No fields in sequence." << std::endl;
    return;
  }

  const Glib::ustring query = Utils::build_sql_select_with_where_clause(found_set.m_table_name, fieldsSequence, found_set.m_where_clause, found_set.m_extra_join, found_set.m_sort_clause, found_set.m_extra_group_by);
 
  //TODO: Lock the database (prevent changes) during export.
  Glib::RefPtr<Gnome::Gda::DataModel> result = query_execute_select(query);

  guint rows_count = 0;
  if(result)
    rows_count = result->get_n_rows();

  if(rows_count)
  {
    const guint columns_count = result->get_n_columns();

    for(guint row_index = 0; row_index < rows_count; ++row_index)
    {
        std::string row_string;

        for(guint col_index = 0; col_index < columns_count; ++col_index)
        {
          const Gnome::Gda::Value value = result->get_value_at(col_index, row_index);

          sharedptr<LayoutItem_Field> layout_item = fieldsSequence[col_index];
          //if(layout_item->m_field.get_glom_type() != Field::TYPE_IMAGE) //This is too much data.
          //{
            if(!row_string.empty())
              row_string += ",";

            //Output data in canonical SQL format, ignoring the user's locale, and ignoring the layout formatting:
            row_string += layout_item->get_full_field_details()->to_file_format(value);

            if(layout_item->get_glom_type() == Field::TYPE_IMAGE) //This is too much data.
            {
              if(!Conversions::value_is_empty(value))
                std::cout << "  field name=" << layout_item->get_name() << ", image value not empty=" << std::endl;
            }
            //std::cout << "  field name=" << layout_item->get_name() << ", value=" << layout_item->m_field.sql(value) << std::endl;
          //}
        }

        //std::cout << " row_string=" << row_string << std::endl;
        the_stream << row_string << std::endl;
    }
  }
}

void Frame_Glom::on_menu_file_import()
{
  if(m_table_name.empty())
  {
    Gtk::MessageDialog dialog(*get_app_window(), "There is no table to import data into", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
    dialog.run();
  }
  else
  {
    Gtk::FileChooserDialog file_chooser(*get_app_window(), _("Choose a CSV file to open"), Gtk::FILE_CHOOSER_ACTION_OPEN);
    file_chooser.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    file_chooser.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_ACCEPT);
    Gtk::FileFilter filter_csv;
    filter_csv.set_name(_("CSV files"));
    filter_csv.add_mime_type("text/csv");
    file_chooser.add_filter(filter_csv);
    Gtk::FileFilter filter_any;
    filter_any.set_name(_("All files"));
    filter_any.add_pattern("*");
    file_chooser.add_filter(filter_any);

    if(file_chooser.run() == Gtk::RESPONSE_ACCEPT)
    {
      file_chooser.hide();

      Dialog_Import_CSV* dialog = 0;
      Glib::RefPtr<Gnome::Glade::Xml> refXml = Gnome::Glade::Xml::create(Utils::get_glade_file_path("glom.glade"), "dialog_import_csv");
      refXml->get_widget_derived("dialog_import_csv", dialog);
      add_view(dialog);

      dialog->import(file_chooser.get_uri(), m_table_name);
      while(Glom::Utils::dialog_run_with_help(dialog, "dialog_import_csv") == Gtk::RESPONSE_ACCEPT)
      {
        dialog->hide();

        Dialog_Import_CSV_Progress* progress_dialog = 0;
        Glib::RefPtr<Gnome::Glade::Xml> refXml = Gnome::Glade::Xml::create(Utils::get_glade_file_path("glom.glade"), "dialog_import_csv_progress");
        refXml->get_widget_derived("dialog_import_csv_progress", progress_dialog);
        add_view(progress_dialog);

        progress_dialog->init_db_details(dialog->get_target_table_name());
        progress_dialog->import(*dialog);
        const int response = progress_dialog->run();

        remove_view(progress_dialog);
        delete progress_dialog;
        progress_dialog = 0;

        // Force update from database so the newly added entries are shown
        show_table_refresh();

        // Re-show chooser dialog when an error occured or when the user
        // cancelled.
        if(response == Gtk::RESPONSE_OK)
          break;
      }

      remove_view(dialog);
      delete dialog;
    }
  }
}
#endif // !GLOM_ENABLE_CLIENT_ONLY

void Frame_Glom::on_menu_file_print()
{
 Notebook_Glom* notebook_current = dynamic_cast<Notebook_Glom*>(m_pBox_Mode->get_child());
 if(notebook_current)
   notebook_current->do_menu_file_print();
}

#ifndef GLOM_ENABLE_CLIENT_ONLY
void Frame_Glom::on_menu_file_print_edit_layouts()
{
  on_menu_developer_print_layouts();
}

void Frame_Glom::show_layout_toolbar (bool show)
{
  m_Notebook_Data.show_layout_toolbar(show);
}

#endif // !GLOM_ENABLE_CLIENT_ONLY

void Frame_Glom::on_menu_Mode_Data()
{
  if(set_mode(MODE_Data))
    show_table(m_table_name);
}

void Frame_Glom::on_menu_Mode_Find()
{
  //This can take quite a long time, flicking between 1 or 2 intermediate screens. 
  //It shouldn't, but until we fix that, let's show the busy cursor while it's working:
  Bakery::BusyCursor busy_cursor(get_app_window());

  const bool previously_in_data_mode = (m_Mode == MODE_Data);

  const Notebook_Data::dataview list_or_details = m_Notebook_Data.get_current_view();

  //A workaround hack to make sure that the list view will be active when the results are shown.
  //Because the list doesn't refresh properly (to give the first result) when the Details view was active first.
  //murrayc.
  if(previously_in_data_mode && (list_or_details == Notebook_Data::DATA_VIEW_Details))
    m_Notebook_Data.set_current_view(Notebook_Data::DATA_VIEW_List);

  if(set_mode(MODE_Find))
  {
    show_table(m_table_name);

    if(previously_in_data_mode)
    {
      //Show the same layout in Find mode as was just being viewed in Data mode:
      m_Notebook_Find.set_current_view(list_or_details);
    }
  }
}

#ifndef GLOM_ENABLE_CLIENT_ONLY
void Frame_Glom::on_menu_Reports_EditReports()
{
  on_menu_developer_reports();
}

void Frame_Glom::on_menu_File_EditPrintLayouts()
{
  on_menu_developer_print_layouts();
}

void Frame_Glom::on_menu_Tables_EditTables()
{
  do_menu_Navigate_Table(); 
}

void Frame_Glom::on_menu_Tables_AddRelatedTable()
{
  //Delete and recreate the dialog,
  //so we start with a blank one:
  if(m_dialog_addrelatedtable)
  {
    remove_view(m_dialog_addrelatedtable);
    delete m_dialog_addrelatedtable;
    m_dialog_addrelatedtable = 0;
  }

  try
  {
    Glib::RefPtr<Gnome::Glade::Xml> refXml = Gnome::Glade::Xml::create(Utils::get_glade_file_path("glom_developer.glade"), "dialog_add_related_table");

    refXml->get_widget_derived("dialog_add_related_table", m_dialog_addrelatedtable);
  }
  catch(const Gnome::Glade::XmlError& ex)
  {
    std::cerr << ex.what() << std::endl;
  }

  if(!m_dialog_addrelatedtable)
    return;

  add_view(m_dialog_addrelatedtable); //Give it access to the document.
  m_dialog_addrelatedtable->set_fields(m_table_name);

  m_dialog_addrelatedtable->signal_request_edit_fields().connect( sigc::mem_fun(*this, &Frame_Glom::on_dialog_add_related_table_request_edit_fields) );

  m_dialog_addrelatedtable->signal_response().connect( sigc::mem_fun(*this, &Frame_Glom::on_dialog_add_related_table_response) );

  Gtk::Window* parent = get_app_window();

  if(parent)
    m_dialog_addrelatedtable->set_transient_for(*parent);

  m_dialog_addrelatedtable->set_modal(); //We don't want people to edit the main window while we are changing structure.
  m_dialog_addrelatedtable->show();
}

#endif

#ifndef GLOM_ENABLE_CLIENT_ONLY

void Frame_Glom::on_dialog_add_related_table_response(int response)
{
  if(!m_dialog_addrelatedtable)
    return;

  m_dialog_addrelatedtable->hide();

  bool stop_trying = false;
  if(response == Gtk::RESPONSE_OK)
  {
    Glib::ustring table_name, relationship_name, from_key_name;
    m_dialog_addrelatedtable->get_input(table_name, relationship_name, from_key_name);

    Gtk::Window* parent = get_app_window();

    //It would be nice to put this in the dialog's on_response() instead,
    //but I don't think we can stop the response from being returned. murrayc
    if(get_table_exists_in_database(table_name))
    {
      Frame_Glom::show_ok_dialog(_("Table Exists Already"), _("A table with this name already exists in the database. Please choose a different table name."), *parent, Gtk::MESSAGE_ERROR);
    }
    else if(get_relationship_exists(m_table_name, relationship_name))
    {
      Frame_Glom::show_ok_dialog(_("Relationship Exists Already"), _("A relationship with this name already exists for this table. Please choose a different relationship name."), *parent, Gtk::MESSAGE_ERROR);
    }
    else if(table_name.empty() || relationship_name.empty() || relationship_name.empty())
    {
      Frame_Glom::show_ok_dialog(_("More information needed"), _("You must specify a field, a table name, and a relationship name."), *parent, Gtk::MESSAGE_ERROR);
    }
    else
    {
      stop_trying = true;
    }

    if(!stop_trying)
    {
      //Offer the dialog again:
      //This signal handler should be called again when a button is clicked.
      m_dialog_addrelatedtable->show();
    }
    else
    {
      //Create the new table:
      const bool result = create_table_with_default_fields(table_name);
      if(!result)
      {
        std::cerr << "Frame_Glom::on_menu_Tables_AddRelatedTable(): create_table_with_default_fields() failed." << std::endl;
        return;
      }
   
      //Create the new relationship: 
      sharedptr<Relationship> relationship = sharedptr<Relationship>::create();

      relationship->set_name(relationship_name);
      relationship->set_title(Utils::title_from_string(relationship_name));
      relationship->set_from_table(m_table_name);
      relationship->set_from_field(from_key_name);
      relationship->set_to_table(table_name);

      sharedptr<Field> related_primary_key = get_field_primary_key_for_table(table_name); //This field was created by create_table_with_default_fields().
      if(!related_primary_key)
      {
        std::cerr << "Frame_Glom::on_menu_Tables_AddRelatedTable(): get_field_primary_key_for_table() failed." << std::endl;
        return;
      }

      relationship->set_to_field(related_primary_key->get_name());

      relationship->set_allow_edit(true);
      relationship->set_auto_create(true);

      Document_Glom* document = get_document();
      if(!document)
        return;

      document->set_relationship(m_table_name, relationship);

      on_dialog_tables_hide(); //Update the menu.

      Gtk::Window* parent = get_app_window();
      if(parent)
        show_ok_dialog(_("Related Table Created"), _("The new related table has been created."), *parent, Gtk::MESSAGE_INFO);
    }
  }
}

void Frame_Glom::on_dialog_add_related_table_request_edit_fields()
{
  if(m_dialog_addrelatedtable)
    do_menu_developer_fields(*m_dialog_addrelatedtable);
}
#endif // !GLOM_ENABLE_CLIENT_ONLY

void Frame_Glom::do_menu_Navigate_Table(bool open_default)
{
  
  if(get_document()->get_connection_database().empty())
  {
    alert_no_table();
    return;
  }

  Glib::ustring default_table_name;
  if(open_default)
    default_table_name = get_document()->get_default_table();
  
  //Create the dialog, if it has not already been created:
  if(!m_pBox_Tables)
  {
    Utils::get_glade_widget_derived_with_warning("box_navigation_tables", m_pBox_Tables);
    m_pDialog_Tables = new Dialog_Glom(m_pBox_Tables);
    m_pDialog_Tables->signal_hide().connect(sigc::mem_fun(*this, &Frame_Glom::on_dialog_tables_hide));

    Gtk::Window* pWindow = get_app_window();
    if(pWindow)
      m_pDialog_Tables->set_transient_for(*pWindow);

    m_pDialog_Tables->get_vbox()->pack_start(*m_pBox_Tables);
    m_pDialog_Tables->set_default_size(300, 400);
    m_pBox_Tables->show_all();
    add_view(m_pBox_Tables);

    //Connect signals:
    m_pBox_Tables->signal_selected.connect(sigc::mem_fun(*this, &Frame_Glom::on_box_tables_selected));
  }

  {
    Bakery::BusyCursor busy_cursor(get_app_window());
    m_pBox_Tables->init_db_details();
  }

  //Let the user choose a table:
  //m_pDialog_Tables->set_policy(false, true, false); //TODO_port
  //m_pDialog_Tables->load_from_document(); //Refresh
  if(!default_table_name.empty())
  {
    //Show the default table, and let the user navigate to another table manually if he wants:
    show_table(default_table_name);
  }
  else
  {
    m_pDialog_Tables->show();
  }
}

const Gtk::Window* Frame_Glom::get_app_window() const
{
  Frame_Glom* nonconst = const_cast<Frame_Glom*>(this);
  return nonconst->get_app_window();
}

Gtk::Window* Frame_Glom::get_app_window()
{
  Gtk::Widget* pWidget = get_parent();
  while(pWidget)
  {
    //Is this widget a Gtk::Window?:
    Gtk::Window* pWindow = dynamic_cast<Gtk::Window*>(pWidget);
    if(pWindow)
    {
      //Yes, return it.
      return pWindow;
    }
    else
    {
      //Try the parent's parent:
      pWidget = pWidget->get_parent();
    }
  }

  return 0; //not found.

}

void Frame_Glom::show_ok_dialog(const Glib::ustring& title, const Glib::ustring& message, Gtk::Window& parent, Gtk::MessageType message_type)
{
  Utils::show_ok_dialog(title, message, parent, message_type);
}

void Frame_Glom::on_button_quickfind()
{
  const Glib::ustring criteria = m_pEntry_QuickFind->get_text();
  if(criteria.empty())
  {
    Glib::ustring message(_("You have not entered any quick find criteria."));
#ifdef GLOM_ENABLE_MAEMO
    Hildon::Note note(Hildon::NOTE_TYPE_INFORMATION, *get_app_window(), message);
    note.run();
#else
    Gtk::MessageDialog dialog(Bakery::App_Gtk::util_bold_message(_("No Find Criteria")), true, Gtk::MESSAGE_WARNING );
    dialog.set_secondary_text(message);
    dialog.set_transient_for(*get_app_window());
    dialog.run();
#endif
  }
  else
  {
    const Glib::ustring where_clause = get_find_where_clause_quick(m_table_name, Gnome::Gda::Value(criteria));
    //std::cout << "Frame_Glom::on_button_quickfind(): where_clause=" << where_clause << std::endl;
    on_notebook_find_criteria(where_clause);
  }
}

void Frame_Glom::on_notebook_find_criteria(const Glib::ustring& where_clause)
{
  //std::cout << "Frame_Glom::on_notebook_find_criteria(): " << where_clause << std::endl;
  //on_menu_Mode_Data();

  App_Glom* pApp = dynamic_cast<App_Glom*>(get_app_window());
  if(pApp)
  {
    bool records_found = false;

    { //Extra scope, to control the lifetime of the busy cursor. 
      Bakery::BusyCursor busy_cursor(pApp);

      pApp->set_mode_data();

      //std::cout << "Frame_Glom::on_notebook_find_criteria: where_clause=" << where_clause << std::endl;
      FoundSet found_set;
      found_set.m_table_name = m_table_name;
      found_set.m_where_clause = where_clause;
      records_found = m_Notebook_Data.init_db_details(found_set);

      //std::cout << "Frame_Glom::on_notebook_find_criteria(): BEFORE  m_Notebook_Data.select_page_for_find_results()" << std::endl;
      m_Notebook_Data.select_page_for_find_results();
      //std::cout << "Frame_Glom::on_notebook_find_criteria(): AFTER  m_Notebook_Data.select_page_for_find_results()" << std::endl;
    }

    if(!records_found)
    {
      const bool find_again = show_warning_no_records_found(*get_app_window());

      if(find_again)
        pApp->set_mode_find();
      else
        on_button_find_all();
    }
    else
    {
      //Show how many records were found:
      update_records_count();
    }
  }
}

void Frame_Glom::on_userlevel_changed(AppState::userlevels userlevel)
{
  //show user level:
  Glib::ustring user_level_name = _("Operator");
  if(userlevel == AppState::USERLEVEL_DEVELOPER)
    user_level_name = _("Developer");

  if(m_pLabel_userlevel)
    m_pLabel_userlevel->set_text(user_level_name);
  
  show_table_title(); 
}

void Frame_Glom::show_table_title()
{
  if(get_document())
  {
    //Show the table title:
    Glib::ustring table_label = get_document()->get_table_title(m_table_name);
    if(!table_label.empty())
    {
      Document_Glom* document = dynamic_cast<Document_Glom*>(get_document());
      if(document)
      {
        if(document->get_userlevel() == AppState::USERLEVEL_DEVELOPER)
          table_label += " (" + m_table_name + ")"; //Show the table name as well, if in developer mode.
      }
    }
    else //Use the table name if there is no table title.
      table_label = m_table_name;

#ifdef GLOM_ENABLE_MAEMO
    // xx-large is too large on maemo, taking away too much (vertical)
    // screen estate
    m_pLabel_Table->set_markup("<b>" + table_label + "</b>");
#else
    m_pLabel_Table->set_markup("<b><span size=\"xx-large\">" + table_label + "</span></b>"); //Show the table title in large text, because it's very important to the user.
#endif
  }
}

#ifndef GLOM_ENABLE_CLIENT_ONLY
void Frame_Glom::update_table_in_document_from_database()
{
  //Add any new/changed information from the database to the document
  //The database should never change without the knowledge of the document anyway, so this should be unnecessary.

  //TODO_performance: There are a lot of temporary Field and Column instances here, with a lot of string copying.

  //For instance, changed field details, or new fields, or removed fields.
  typedef Box_DB_Table::type_vecFields type_vecFields;

  //Get the fields information from the database:
  Base_DB::type_vecFields fieldsDatabase = Base_DB::get_fields_for_table_from_database(m_table_name);

  Document_Glom* pDoc = dynamic_cast<const Document_Glom*>(get_document());
  if(pDoc)
  {
    bool document_must_be_updated = false;

    //Get the fields information from the document.
    //and add to, or update Document's list of fields:
    type_vecFields fieldsDocument = pDoc->get_table_fields(m_table_name);

    for(Base_DB::type_vecFields::const_iterator iter = fieldsDatabase.begin(); iter != fieldsDatabase.end(); ++iter)
    {
      sharedptr<Field> field_database = *iter;
      if(field_database)
      {
        //Is the field already in the document?
        type_vecFields::iterator iterFindDoc = std::find_if( fieldsDocument.begin(), fieldsDocument.end(), predicate_FieldHasName<Field>( field_database->get_name() ) );
        if(iterFindDoc == fieldsDocument.end()) //If it was not found:
        {
          //Add it
          fieldsDocument.push_back(field_database);
          document_must_be_updated = true;
        }
        else //if it was found.
        {
          //Compare the information:
          Glib::RefPtr<Gnome::Gda::Column> field_info_db = field_database->get_field_info();
          sharedptr<Field> field_document =  *iterFindDoc;
          if(field_document)
          {
            if(!field_document->field_info_from_database_is_equal( field_info_db )) //ignores auto_increment because libgda does not report it from the database properly.
            {
              //The database has different information. We assume that the information in the database is newer.

              //Update the field information:

              // Ignore things that libgda does not report from the database properly:
              // We do this also in Field::field_info_from_database_is_equal() and 
              // Base_DB::get_fields_for_table(), 
              // so make sure that we ignore the same things everywhere always
              // TODO: Avoid that duplication?
              field_info_db->set_auto_increment( field_document->get_auto_increment() );
              field_info_db->set_default_value( field_document->get_default_value() );

              (*iterFindDoc)->set_field_info( field_info_db );

              document_must_be_updated = true;
            }
          }
        }
      }
    }

    //Remove fields that are no longer in the database:
    //TODO_performance: This is incredibly inefficient - but it's difficut to erase() items while iterating over them.
    type_vecFields fieldsActual;
    for(type_vecFields::const_iterator iter = fieldsDocument.begin(); iter != fieldsDocument.end(); ++iter)
    {
      sharedptr<Field> field = *iter;

      //Check whether it's in the database:
      type_vecFields::iterator iterFindDatabase = std::find_if( fieldsDatabase.begin(), fieldsDatabase.end(), predicate_FieldHasName<Field>( field->get_name() ) );
      if(iterFindDatabase != fieldsDatabase.end()) //If it was found
      {
        fieldsActual.push_back(field);
      }
      else
      {
        document_must_be_updated = true; //Something changed.
      }
    }

    if(document_must_be_updated)
      pDoc->set_table_fields(m_table_name, fieldsActual);
  }
}
#endif // !GLOM_ENABLE_CLIENT_ONLY

void Frame_Glom::set_document(Document_Glom* pDocument)
{
  View_Composite_Glom::set_document(pDocument);

  Document_Glom* document = get_document();
  if(document)
  {
    //Connect to a signal that is only on the derived document class:
    document->signal_userlevel_changed().connect( sigc::mem_fun(*this, &Frame_Glom::on_userlevel_changed) );

    //Show the appropriate UI for the user level that is specified by this new document:
    on_userlevel_changed(document->get_userlevel());
  }
}

void Frame_Glom::load_from_document()
{
  Document_Glom* document = dynamic_cast<Document_Glom*>(get_document());
  if(document)
  {
    //Call base class:
    View_Composite_Glom::load_from_document();
  }
}

#ifndef GLOM_ENABLE_CLIENT_ONLY
void Frame_Glom::on_menu_developer_database_preferences()
{
  Dialog_Database_Preferences* dialog = 0;
  try
  {
    Glib::RefPtr<Gnome::Glade::Xml> refXml = Gnome::Glade::Xml::create(Utils::get_glade_file_path("glom_developer.glade"), "dialog_database_preferences");
    refXml->get_widget_derived("dialog_database_preferences", dialog);
    if(dialog)
    {
      dialog->set_transient_for(*(get_app_window()));
      add_view(dialog);
      dialog->load_from_document();

     Glom::Utils::dialog_run_with_help(dialog, "dialog_database_preferences");

      remove_view(dialog);
      delete dialog;

      //show_system_name(); //In case it has changed.
    }
  }

  catch(const Gnome::Glade::XmlError& ex)
  {
    std::cerr << ex.what() << std::endl;
  }
}

void Frame_Glom::on_menu_developer_fields()
{
  Gtk::Window* parent = get_app_window();
  if(parent)
    do_menu_developer_fields(*parent);

}

void Frame_Glom::do_menu_developer_fields(Gtk::Window& parent, const Glib::ustring table_name)
{
  if(!m_pDialog_Fields)
  {
    try
    {
      Glib::RefPtr<Gnome::Glade::Xml> refXml = Gnome::Glade::Xml::create(Utils::get_glade_file_path("glom_developer.glade"), "window_design");

      refXml->get_widget_derived("window_design", m_pDialog_Fields);
      m_pDialog_Fields->signal_hide().connect( sigc::mem_fun(*this, &Frame_Glom::on_developer_dialog_hide));
    }
    catch(const Gnome::Glade::XmlError& ex)
    {
      std::cerr << ex.what() << std::endl;
    }

    add_view(m_pDialog_Fields);
  }

  // Some database backends (SQLite) require the table to change no longer
  // being in use when changing the records, so clear the database usage
  // here. We reshow everything in on_developer_dialog_hide anyway.
  show_no_table();

  // Remember the old table name so that we re-show the previous table as
  // soon as the dialog has been closed.
  m_table_name = table_name;

  m_pDialog_Fields->set_transient_for(parent);
  m_pDialog_Fields->init_db_details(table_name);
  m_pDialog_Fields->show();
}

void Frame_Glom::do_menu_developer_fields(Gtk::Window& parent)
{
  //Check that there is a table to show:
  if(m_table_name.empty())
  {
    alert_no_table(); //TODO: Disable the menu item instead.
  }
  else
  {
    do_menu_developer_fields(parent, m_table_name);
  }
}


void Frame_Glom::on_menu_developer_relationships_overview()
{
  if(!m_dialog_relationships_overview)
  {
    Utils::get_glade_developer_widget_derived_with_warning("dialog_relationships_overview", m_dialog_relationships_overview);
    add_view(m_dialog_relationships_overview);
  }

  if(m_dialog_relationships_overview)
  {
    m_dialog_relationships_overview->set_transient_for(*(get_app_window()));
    m_dialog_relationships_overview->load_from_document();

    Glom::Utils::dialog_run_with_help(m_dialog_relationships_overview, "dialog_relationships_overview");

    remove_view(m_dialog_relationships_overview);
    delete m_dialog_relationships_overview;
    m_dialog_relationships_overview = 0;
  }
}

void Frame_Glom::do_menu_developer_relationships(Gtk::Window& parent, const Glib::ustring table_name)
{
  //Create the widget if necessary:
  if(!m_pDialog_Relationships)
  {
    Utils::get_glade_developer_widget_derived_with_warning("window_design", m_pDialog_Relationships);
    m_pDialog_Relationships->set_title("Relationships");
    m_pDialog_Relationships->signal_hide().connect( sigc::mem_fun(*this, &Frame_Glom::on_developer_dialog_hide));
    add_view(m_pDialog_Relationships); //Also a composite view.
  }

  m_pDialog_Relationships->set_transient_for(parent);
  m_pDialog_Relationships->init_db_details(table_name);
  m_pDialog_Relationships->show();
}

void Frame_Glom::on_menu_developer_relationships()
{
  //Check that there is a table to show:
  if(m_table_name.empty())
  {
    alert_no_table(); //TODO: Disable the menu item instead.
  }
  else
  {
    do_menu_developer_relationships(*get_app_window(), m_table_name);
  }
}

void Frame_Glom::on_menu_developer_users()
{
  Dialog_GroupsList* dialog = 0;
  try
  {
    Glib::RefPtr<Gnome::Glade::Xml> refXml = Gnome::Glade::Xml::create(Utils::get_glade_file_path("glom_developer.glade"), "window_groups");

    refXml->get_widget_derived("window_groups", dialog);
  }
  catch(const Gnome::Glade::XmlError& ex)
  {
    std::cerr << ex.what() << std::endl;
  }

  dialog->set_transient_for(*get_app_window());

  add_view(dialog); //Give it access to the document.
  dialog->load_from_document(); //Update the UI now that it has the document.

  Glom::Utils::dialog_run_with_help(dialog, "window_groups");
  remove_view(dialog);
  delete dialog;

  //Update the Details and List layouts, in case the permissions have changed:
  //TODO: Also update them somehow if another user has changed them,
  //or respond to the failed SQL nicely.
  show_table(m_table_name);
}

void Frame_Glom::on_menu_developer_layout()
{
  //Check that there is a table to show:
  if(m_table_name.empty())
  {
    alert_no_table(); //TODO: Disable the menu item instead.
  }
  else
  {
    Notebook_Glom* notebook_current = dynamic_cast<Notebook_Glom*>(m_pBox_Mode->get_child());
    if(notebook_current)
      notebook_current->do_menu_developer_layout();
  }
}

void Frame_Glom::on_menu_developer_reports()
{
  //Check that there is a table to show:
  if(m_table_name.empty())
  {
    alert_no_table(); //TODO: Disable the menu item instead.
    return;
  }
  
  //Create the widget if necessary:
  if(!m_pBox_Reports)
  {
    Utils::get_glade_developer_widget_derived_with_warning("box_reports", m_pBox_Reports);
    m_pDialog_Reports = new Dialog_Glom(m_pBox_Reports);
    m_pDialog_Reports->set_transient_for(*(get_app_window()));

    Utils::get_glade_developer_widget_derived_with_warning("window_report_layout", m_pDialogLayoutReport);
    add_view(m_pDialogLayoutReport);
    m_pDialogLayoutReport->set_transient_for(*(get_app_window()));
    m_pDialogLayoutReport->signal_hide().connect( sigc::mem_fun(*this, &Frame_Glom::on_dialog_layout_report_hide) );

    m_pDialog_Reports->get_vbox()->pack_start(*m_pBox_Reports);
    m_pDialog_Reports->set_default_size(300, 400);
    m_pBox_Reports->show_all();

    m_pBox_Reports->signal_selected.connect(sigc::mem_fun(*this, &Frame_Glom::on_box_reports_selected));
    add_view(m_pBox_Reports);
  }

  m_pBox_Reports->init_db_details(m_table_name);
  m_pDialog_Reports->show();
}

void Frame_Glom::on_menu_developer_print_layouts()
{
  //Check that there is a table to show:
  if(m_table_name.empty())
  {
    alert_no_table(); //TODO: Disable the menu item instead.
    return;
  }

  //Create the widget if necessary:
  if(!m_pBox_PrintLayouts)
  {
    Utils::get_glade_developer_widget_derived_with_warning("box_print_layouts", m_pBox_PrintLayouts);
    m_pDialog_PrintLayouts = new Dialog_Glom(m_pBox_PrintLayouts);

    m_pDialog_PrintLayouts->get_vbox()->pack_start(*m_pBox_PrintLayouts);
    m_pDialog_PrintLayouts->set_default_size(300, 400);
    m_pBox_PrintLayouts->show_all();
    add_view(m_pBox_PrintLayouts);

    m_pBox_PrintLayouts->signal_selected.connect(sigc::mem_fun(*this, &Frame_Glom::on_box_print_layouts_selected));
  }
  
  m_pBox_PrintLayouts->init_db_details(m_table_name);
  m_pDialog_PrintLayouts->show();
}

void Frame_Glom::on_menu_developer_script_library()
{
  Dialog_ScriptLibrary* dialog = 0;
  Glib::RefPtr<Gnome::Glade::Xml> refXml = Gnome::Glade::Xml::create(Utils::get_glade_file_path("glom_developer.glade"), "dialog_script_library");
  refXml->get_widget_derived("dialog_script_library", dialog);
  dialog->set_transient_for(*(get_app_window()));
  add_view(dialog); //Give it access to the document.
  dialog->load_from_document();
  Glom::Utils::dialog_run_with_help(dialog, "dialog_script_library"); //TODO: Create the help section.
  dialog->save_to_document();
  remove_view(dialog);
  delete dialog;
}

void Frame_Glom::on_box_reports_selected(const Glib::ustring& report_name)
{
  m_pDialog_Reports->hide();

  sharedptr<Report> report = get_document()->get_report(m_table_name, report_name);
  if(report)
  {
    m_pDialogLayoutReport->set_transient_for(*get_app_window());
    m_pDialogLayoutReport->set_report(m_table_name, report);
    m_pDialogLayoutReport->show();
  }
}



void Frame_Glom::on_box_print_layouts_selected(const Glib::ustring& print_layout_name)
{
  //Create the dialog if necessary:
  if(!m_pDialogLayoutPrint)
  {
    Utils::get_glade_developer_widget_derived_with_warning("window_print_layout_edit", m_pDialogLayoutPrint);
    add_view(m_pDialogLayoutPrint);
    m_pDialogLayoutPrint->signal_hide().connect( sigc::mem_fun(*this, &Frame_Glom::on_dialog_layout_print_hide) );
  }

  m_pDialog_PrintLayouts->hide();

  sharedptr<PrintLayout> print_layout = get_document()->get_print_layout(m_table_name, print_layout_name);
  if(print_layout)
  {
    m_pDialogLayoutPrint->set_transient_for(*get_app_window());
    m_pDialogLayoutPrint->set_print_layout(m_table_name, print_layout);
    m_pDialogLayoutPrint->show();
  }
}

void Frame_Glom::on_developer_dialog_hide()
{
  //The dababase structure might have changed, so refresh the data view:
  show_table(m_table_name);

  //TODO: This is a bit of a hack. It's not always useful to do this:
  if(m_dialog_addrelatedtable)
    m_dialog_addrelatedtable->set_fields(m_table_name);

  //Update the display. TODO: Shouldn't this happen automatically via the view?
  if(m_dialog_relationships_overview)
    m_dialog_relationships_overview->load_from_document();
}
#endif // !GLOM_ENABLE_CLIENT_ONLY

namespace 
{
  void setup_connection_pool_from_document(Document_Glom* document)
  {
    ConnectionPool* connection_pool = ConnectionPool::get_instance();
    switch(document->get_hosting_mode())
    {
#ifdef GLOM_ENABLE_POSTGRESQL

#ifndef GLOM_ENABLE_CLIENT_ONLY
    case Document_Glom::HOSTING_MODE_POSTGRES_SELF:
      {
        ConnectionPoolBackends::PostgresSelfHosted* backend = new ConnectionPoolBackends::PostgresSelfHosted;
        backend->set_self_hosting_data_uri(document->get_connection_self_hosted_directory_uri());
        connection_pool->set_backend(std::auto_ptr<ConnectionPool::Backend>(backend));
      }
      break;
#endif //GLOM_ENABLE_CLIENT_ONLY

    case Document_Glom::HOSTING_MODE_POSTGRES_CENTRAL:
      {
        ConnectionPoolBackends::PostgresCentralHosted* backend = new ConnectionPoolBackends::PostgresCentralHosted;
        backend->set_host(document->get_connection_server());
        backend->set_port(document->get_connection_port());
        backend->set_try_other_ports(document->get_connection_try_other_ports());
        connection_pool->set_backend(std::auto_ptr<ConnectionPool::Backend>(backend));
      }
      break;
#endif //GLOM_ENABLE_POSTGRESQL

#ifdef GLOM_ENABLE_SQLITE
    case Document_Glom::HOSTING_MODE_SQLITE:
      {
        ConnectionPoolBackends::Sqlite* backend = new ConnectionPoolBackends::Sqlite;
        backend->set_database_directory_uri(document->get_connection_self_hosted_directory_uri());
        connection_pool->set_backend(std::auto_ptr<ConnectionPool::Backend>(backend));
      }
      break;
#endif // GLOM_ENABLE_SQLITE

    default:
      g_assert_not_reached();
      break;
    }

    // Might be overwritten later when actually attempting a connection:
    connection_pool->set_user(document->get_connection_user());
    connection_pool->set_database(document->get_connection_database());

    connection_pool->set_ready_to_connect();
  }
}

bool Frame_Glom::connection_request_password_and_choose_new_database_name()
{
  Document_Glom* document = dynamic_cast<Document_Glom*>(get_document());
  if(!document)
    return false;

  ConnectionPool* connection_pool = ConnectionPool::get_instance();
  setup_connection_pool_from_document(document);

  if(!m_pDialogConnection)
  {
    Utils::get_glade_widget_derived_with_warning("dialog_connection", m_pDialogConnection);
    add_view(m_pDialogConnection); //Also a composite view.
  }

  //Ask either for the existing username and password to use an existing database server,
  //or ask for a new username and password to specify when creating a new self-hosted database.
  switch(document->get_hosting_mode())
  {
#ifdef GLOM_ENABLE_POSTGRESQL
  case Document_Glom::HOSTING_MODE_POSTGRES_SELF:
    {
#ifndef GLOM_ENABLE_CLIENT_ONLY
      Dialog_NewSelfHostedConnection* dialog = 0;
      Glib::RefPtr<Gnome::Glade::Xml> refXml;

#ifdef GLIBMM_EXCEPTIONS_ENABLED
      try
      {
        refXml = Gnome::Glade::Xml::create(Utils::get_glade_file_path("glom_developer.glade"), "dialog_new_self_hosted_connection");
      }
      catch(const Gnome::Glade::XmlError& ex)
      {
        std::cerr << ex.what() << std::endl;
        return false;
      }
#else
      std::auto_ptr<Gnome::Glade::XmlError> error;
      refXml = Gnome::Glade::Xml::create(Utils::get_glade_file_path("glom_developer.glade"), "dialog_new_self_hosted_connection", error);
      if(error.get())
      {
        std::cerr << error->what() << std::endl;
        return false;
      }
#endif //GLIBMM_EXCEPTIONS_ENABLED

      refXml->get_widget_derived("dialog_new_self_hosted_connection", dialog);
      if(!dialog) return false;

      add_view(dialog);


      int response = Gtk::RESPONSE_OK;
      bool keep_trying = true;
      while(keep_trying)
      {
        response = Glom::Utils::dialog_run_with_help(dialog, "dialog_new_self_hosted_connection");

        //Check the password is acceptable:
        if(response == Gtk::RESPONSE_OK)
        {
          const bool password_ok = dialog->check_password();
          if(password_ok)
          {
            keep_trying = false; //Everything is OK.
          }
        }
        else
          keep_trying = false; //The user cancelled.

      }

      dialog->hide();
 
      // Create the requested self-hosting database:
      bool created = false;
      if(response == Gtk::RESPONSE_OK)
      {
        created = dialog->create_self_hosted();
        if(!created)
          return false;

      //Put the details into m_pDialogConnection too, because that's what we use to connect.
      //This is a bit of a hack:
        m_pDialogConnection->set_self_hosted_user_and_password(connection_pool->get_user(), connection_pool->get_password());
      }

      remove_view(dialog);

    //std::cout << "DEBUG: after dialog->create_self_hosted(). The database cluster should now exist." << std::endl;

      if(!created)
        return false; // The user cancelled
#else
      // Self-hosted postgres not supported in client only mode
      g_assert_not_reached();
#endif // !GLOM_ENABLE_CLIENT_ONLY
    }

    break;
  case Document_Glom::HOSTING_MODE_POSTGRES_CENTRAL:
    {
      //Ask for connection details:
      m_pDialogConnection->load_from_document(); //Get good defaults.
      m_pDialogConnection->set_transient_for(*get_app_window());
      int response = Glom::Utils::dialog_run_with_help(m_pDialogConnection, "dialog_connection");
      m_pDialogConnection->hide();

      if(response == Gtk::RESPONSE_OK)
      {
        // We are not self-hosting, but we also call initialize() for
        // consistency (the backend will ignore it anyway).
        if(!connection_pool->initialize(get_app_window()))
          return false;
      }
      else
      {
        // The user cancelled
        return false;
      }
    }

    break;
#endif //GLOM_ENABLE_POSTGRESQL
#ifdef GLOM_ENABLE_SQLITE
  case Document_Glom::HOSTING_MODE_SQLITE:
    {
      // sqlite
      if(!connection_pool->initialize(get_app_window()))
        return false;

      m_pDialogConnection->load_from_document(); //Get good defaults.
      // No authentication required
    }

    break;
#endif //GLOM_ENABLE_SQLITE
  default:
    g_assert_not_reached();
    break;
  }

  // Do startup, such as starting the self-hosting database server
  if(!connection_pool->startup(get_app_window()))
    return false;

  const Glib::ustring database_name = document->get_connection_database();

  //std::cout << "debug: database_name to create=" << database_name << std::endl;


  bool keep_trying = true;
  size_t extra_num = 0;
  while(keep_trying)
  {
    Glib::ustring database_name_possible;
    if(extra_num == 0)
      database_name_possible = database_name; //Try the original name first.
    else
    {
      //Create a new database name by appending a number to the original name:
      Glib::ustring pchExtraNum = Glib::ustring::compose("%1", extra_num);
      database_name_possible = (database_name + pchExtraNum);
    }
    ++extra_num;

    m_pDialogConnection->set_database_name(database_name_possible);
    //std::cout << "debug: possible name=" << database_name_possible << std::endl;

#ifdef GLIBMM_EXCEPTIONS_ENABLED
    try
    {
      sharedptr<SharedConnection> sharedconnection = m_pDialogConnection->connect_to_server_with_connection_settings();
      //If no exception was thrown then the database exists.
      //But we are looking for an unused database name, so we will try again.
    }
    catch(const ExceptionConnection& ex)
    {
#else
    std::auto_ptr<ExceptionConnection> error;
    sharedptr<SharedConnection> sharedconnection = m_pDialogConnection->connect_to_server_with_connection_settings(error);
    if(error.get())
    {
      const ExceptionConnection& ex = *error;
#endif
      //g_warning("Frame_Glom::connection_request_password_and_choose_new_database_name(): caught exception.");

      if(ex.get_failure_type() == ExceptionConnection::FAILURE_NO_SERVER)
      {
        //Warn the user, and let him try again:
        Utils::show_ok_dialog(_("Connection Failed"), _("Glom could not connect to the database server. Maybe you entered an incorrect user name or password, or maybe the postgres database server is not running."), *(get_app_window()), Gtk::MESSAGE_ERROR); //TODO: Add help button.
        keep_trying = false;
      }
      else
      {
        std::cout << "Frame_Glom::connection_request_password_and_choose_new_database_name(): unused database name successfully found: " << database_name_possible << std::endl; 
        //The connection to the server is OK, but the specified database does not exist.
        //That's good - we were looking for an unused database name.

        std::cout << "debug: unused database name found: " << database_name_possible << std::endl;
        document->set_connection_database(database_name_possible);

        // Remember host if the document is not self hosted
        #ifdef GLOM_ENABLE_POSTGRESQL
        if(document->get_hosting_mode() == Document_Glom::HOSTING_MODE_POSTGRES_CENTRAL)
        {
          ConnectionPool::Backend* backend = connection_pool->get_backend();
          ConnectionPoolBackends::PostgresCentralHosted* central = dynamic_cast<ConnectionPoolBackends::PostgresCentralHosted*>(backend);
          g_assert(central != NULL);

          document->set_connection_server(central->get_host());
        }

        #ifndef GLOM_ENABLE_CLIENT_ONLY
        // Remember port if the document is self-hosted, so that remote
        // connections to the database (usinc browse network) know what port to use.
        // TODO: There is already similar code in
        // connect_to_server_with_connection_settings, which is just not
        // executed because it failed with no database present. We should
        // somehow avoid this code duplication.
        else if(document->get_hosting_mode() == Document_Glom::HOSTING_MODE_POSTGRES_SELF)
        {
          ConnectionPool::Backend* backend = connection_pool->get_backend();
          ConnectionPoolBackends::PostgresSelfHosted* self = dynamic_cast<ConnectionPoolBackends::PostgresSelfHosted*>(backend);
          g_assert(self != NULL);

          document->set_connection_port(self->get_port());
        }
        #endif //GLOM_ENABLE_CLIENT_ONLY

        #endif //GLOM_ENABLE_POSTGRESQL

        return true;
      }
    }
  }

  connection_pool->cleanup(get_app_window());
  return false;
}

#ifdef GLIBMM_EXCEPTIONS_ENABLED
bool Frame_Glom::connection_request_password_and_attempt(const Glib::ustring known_username, const Glib::ustring& known_password)
#else
bool Frame_Glom::connection_request_password_and_attempt(const Glib::ustring known_username, const Glib::ustring& known_password, std::auto_ptr<ExceptionConnection>& error)
#endif
{
  Document_Glom* document = dynamic_cast<Document_Glom*>(get_document());
  if(!document)
    return false;

  if(!m_pDialogConnection)
  {
    Utils::get_glade_widget_derived_with_warning("dialog_connection", m_pDialogConnection);
    add_view(m_pDialogConnection); //Also a composite view.
  }

  ConnectionPool* connection_pool = ConnectionPool::get_instance();
  setup_connection_pool_from_document(document);
  if(!connection_pool->startup(get_app_window()))
    return false;

  while(true) //Loop until a return
  {
    //Ask for connection details:
    m_pDialogConnection->load_from_document(); //Get good defaults.
    m_pDialogConnection->set_transient_for(*get_app_window());

    if(!known_username.empty())
      m_pDialogConnection->set_username(known_username);

    if(!known_password.empty())
      m_pDialogConnection->set_password(known_password);

    //Only show the dialog if we don't know the correct username/password yet:
    int response = Gtk::RESPONSE_OK;

    // Don't ask for user/password for sqlite databases, since sqlite does
    // not support authentication. I'd prefer to get that information from
    // libgda, but gda_connection_supports_feature() requires a GdaConnection
    // which we don't have at this point.
#ifdef GLOM_ENABLE_SQLITE
    if(document->get_hosting_mode() != Document_Glom::HOSTING_MODE_SQLITE)
#endif
    {
      if(known_username.empty() && known_password.empty())
      {
        response = Glom::Utils::dialog_run_with_help(m_pDialogConnection, "dialog_connection");
        m_pDialogConnection->hide();
      }
    }

    if(response == Gtk::RESPONSE_OK)
    {
#ifdef GLIBMM_EXCEPTIONS_ENABLED
      try
      {
        //TODO: Remove any previous database setting?
        sharedptr<SharedConnection> sharedconnection = m_pDialogConnection->connect_to_server_with_connection_settings();
        return true; //Succeeded, because no exception was thrown.
      }
      catch(const ExceptionConnection& ex)
      {
#else
      std::auto_ptr<ExceptionConnection> local_error;
      sharedptr<SharedConnection> sharedconnection = m_pDialogConnection->connect_to_server_with_connection_settings(local_error);
      if(!local_error.get())
        return true;
      else
      {
        const ExceptionConnection& ex = *local_error;
#endif
        g_warning("Frame_Glom::connection_request_password_and_attempt(): caught exception.");

        if(ex.get_failure_type() == ExceptionConnection::FAILURE_NO_SERVER)
        {
          //Warn the user, and let him try again:
          Utils::show_ok_dialog(_("Connection Failed"), _("Glom could not connect to the database server. Maybe you entered an incorrect user name or password, or maybe the postgres database server is not running."), *(get_app_window()), Gtk::MESSAGE_ERROR); //TODO: Add help button.

          response = Glom::Utils::dialog_run_with_help(m_pDialogConnection, "dialog_connection");
          m_pDialogConnection->hide();
          if(response != Gtk::RESPONSE_OK)
          {
            connection_pool->cleanup(get_app_window());
            return false; //The user cancelled.
          }
        }
        else
        {
          g_warning("Frame_Glom::connection_request_password_and_attempt(): rethrowing exception.");

          connection_pool->cleanup(get_app_window());

          //The connection to the server is OK, but the specified database does not exist:
#ifdef GLIBMM_EXCEPTIONS_ENABLED
          throw ex; //Pass it on for the caller to handle.
#else
          error = local_error; //Pass it on for the caller to handle.
#endif
          return false;
        }
      }

      //Try again.
    }
    else
    {
      connection_pool->cleanup(get_app_window());
      return false; //The user cancelled.
    }
  }
}

#ifndef GLOM_ENABLE_CLIENT_ONLY
bool Frame_Glom::create_database(const Glib::ustring& database_name, const Glib::ustring& title)
{
#if 1
  // This seems to increase the change that the database creation does not
  // fail due to the "source database is still in use" error. armin.
  //std::cout << "Going to sleep" << std::endl;
  Glib::usleep(500 * 1000);
  //std::cout << "Awake" << std::endl;
#endif

  Gtk::Window* pWindowApp = get_app_window();
  g_assert(pWindowApp);

  Bakery::BusyCursor busycursor(*pWindowApp);

  try
  {
    ConnectionPool::get_instance()->create_database(database_name);
  }
  catch(const Glib::Exception& ex) // libgda does not set error domain
  {
    //I think a failure here might be caused by installing unstable libgda, which seems to affect stable libgda-1.2.
    //Doing a "make install" in libgda-1.2 seems to fix this:
    //TODO: Is this still relevant in libgda-3.0?
    std::cerr << "Frame_Glom::create_database():  Gnome::Gda::Connection::create_database(" << database_name << ") failed: " << ex.what() << std::endl;

    //Tell the user:
    Gtk::Dialog* dialog = 0;
    try
    {
       // TODO: Tell the user what has gone wrong (ex.what())
      Glib::RefPtr<Gnome::Glade::Xml> refXml = Gnome::Glade::Xml::create(Utils::get_glade_file_path("glom_developer.glade"), "dialog_error_create_database");
      refXml->get_widget("dialog_error_create_database", dialog);
      dialog->set_transient_for(*pWindowApp);
      Glom::Utils::dialog_run_with_help(dialog, "dialog_error_create_database");
      delete dialog;
    }
    catch(const Gnome::Glade::XmlError& ex)
    {
      std::cerr << ex.what() << std::endl;
    }

     return false;
  }

  //Connect to the actual database:
  ConnectionPool* connection_pool = ConnectionPool::get_instance();
  connection_pool->set_database(database_name);

  sharedptr<SharedConnection> sharedconnection;
  try
  {
    sharedconnection = connection_pool->connect();
  }
  catch(const Glib::Exception& ex)
  {
    std::cerr << "Frame_Glom::create_database(): Could not connect to just-created database. exception caught:" << ex.what() << std::endl;
    return false;
  }
  catch(const std::exception& ex)
  {
    std::cerr << "Frame_Glom::create_database(): Could not connect to just-created database. exception caught:" << ex.what() << std::endl;
    return false;
  }

  if(sharedconnection)
  {
    bool test = add_standard_tables(); //Add internal, hidden, tables.
    if(!test)
      return false;

    //Create the developer group, and make this user a member of it:
    //If we got this far then the user must really have developer privileges already:
    test = add_standard_groups();
    if(!test)
      return false;

    //std::cout << "Frame_Glom::create_database(): Creation of standard tables and groups finished." << std::endl;

    //Set the title based on the title in the example document, or the user-supplied title when creating new documents:
    SystemPrefs prefs = get_database_preferences();
    if(prefs.m_name.empty())
    {
      //std::cout << "Frame_Glom::create_database(): Setting title in the database." << std::endl;
      prefs.m_name = title;
      set_database_preferences(prefs);
    }
    else
    {
      //std::cout << "Frame_Glom::create_database(): database has title: " << prefs.m_name << std::endl;
    }

    return true;
  }
  else
  {
    std::cerr << "Frame_Glom::create_database(): Could not connect to just-created database." << std::endl;
    return false;
  }
}
#endif // !GLOM_ENABLE_CLIENT_ONLY

void Frame_Glom::on_menu_report_selected(const Glib::ustring& report_name)
{
  const Privileges table_privs = Privs::get_current_privs(m_table_name);

  //Don't try to print tables that the user can't view.
  if(!table_privs.m_view)
  {
    //TODO: Warn the user.
    return;
  }

  Document_Glom* document = get_document();
  sharedptr<Report> report = document->get_report(m_table_name, report_name);
  if(!report)
    return;

  FoundSet found_set = m_Notebook_Data.get_found_set();

  ReportBuilder report_builder;
  report_builder.set_document(document);
  report_builder.report_build(found_set, report, get_app_window()); //TODO: Use found set's where_clause.
}

#ifndef GLOM_ENABLE_CLIENT_ONLY
void Frame_Glom::on_menu_print_layout_selected(const Glib::ustring& print_layout_name)
{
  const Privileges table_privs = Privs::get_current_privs(m_table_name);

  //Don't try to print tables that the user can't view.
  if(!table_privs.m_view)
  {
    //TODO: Warn the user.
    return;
  }

  Document_Glom* document = get_document();
  sharedptr<PrintLayout> print_layout = document->get_print_layout(m_table_name, print_layout_name);
  if(!print_layout)
    return;

  Canvas_PrintLayout canvas;
  add_view(&canvas); //So it has access to the document.
  canvas.set_print_layout(m_table_name, print_layout);

  //Create a new PrintOperation with our PageSetup and PrintSettings:
  //(We use our derived PrintOperation class)
  Glib::RefPtr<PrintOperationPrintLayout> print = PrintOperationPrintLayout::create();
  print->set_canvas(&canvas);

  print->set_track_print_status();
  print->set_default_page_setup(print_layout->get_page_setup());
  //print->set_print_settings(m_refSettings);

  //print->signal_done().connect(sigc::bind(sigc::mem_fun(*this,
  //                &ExampleWindow::on_printoperation_done), print));
  
  const FoundSet found_set = m_Notebook_Data.get_found_set_details();
  canvas.fill_with_data(found_set);

  try
  {
    App_Glom* pApp = dynamic_cast<App_Glom*>(get_app_window());
    if(pApp)
      print->run(Gtk::PRINT_OPERATION_ACTION_PRINT_DIALOG, *pApp);
  }
  catch (const Gtk::PrintError& ex)
  {
    //See documentation for exact Gtk::PrintError error codes.
    std::cerr << "An error occurred while trying to run a print operation:"
        << ex.what() << std::endl;
  }

  remove_view(&canvas);
}
#endif // !GLOM_ENABLE_CLIENT_ONLY

#ifndef GLOM_ENABLE_CLIENT_ONLY
void Frame_Glom::on_dialog_layout_report_hide()
{
  Document_Glom* document = get_document();

  if(document && true) //m_pDialogLayoutReport->get_modified())
  {
    const Glib::ustring original_name = m_pDialogLayoutReport->get_original_report_name();
    sharedptr<Report> report = m_pDialogLayoutReport->get_report();
    if(report && (original_name != report->get_name()))
      document->remove_report(m_table_name, original_name);

    document->set_report(m_table_name, report);
  }

  //Update the reports menu:
  App_Glom* pApp = dynamic_cast<App_Glom*>(get_app_window());
  if(pApp)
    pApp->fill_menu_reports(m_table_name);
}

void Frame_Glom::on_dialog_layout_print_hide()
{
  Document_Glom* document = get_document();

  if(document && true) //m_pDialogLayoutReport->get_modified())
  {
    const Glib::ustring original_name = m_pDialogLayoutPrint->get_original_name();
    sharedptr<PrintLayout> print_layout = m_pDialogLayoutPrint->get_print_layout();
    if(print_layout && (original_name != print_layout->get_name()))
      document->remove_report(m_table_name, original_name);

    document->set_print_layout(m_table_name, print_layout);
  }

  //Update the reports menu:
  App_Glom* pApp = dynamic_cast<App_Glom*>(get_app_window());
  if(pApp)
    pApp->fill_menu_print_layouts(m_table_name);
}

void Frame_Glom::on_dialog_reports_hide()
{
  //Update the reports menu:
  App_Glom* pApp = dynamic_cast<App_Glom*>(get_app_window());
  if(pApp)
    pApp->fill_menu_reports(m_table_name);
}

void Frame_Glom::on_dialog_print_layouts_hide()
{
  //Update the reports menu:
  App_Glom* pApp = dynamic_cast<App_Glom*>(get_app_window());
  if(pApp)
    pApp->fill_menu_print_layouts(m_table_name);
}
#endif // !GLOM_ENABLE_CLIENT_ONLY

void Frame_Glom::on_dialog_tables_hide()
{
  //If tables could have been added or removed, update the tables menu:
  Document_Glom* document = dynamic_cast<Document_Glom*>(get_document());
  if(document)
  {
    // This is never true in client only mode, so we can as well save the
    // code size.
#ifndef GLOM_ENABLE_CLIENT_ONLY
    if(document->get_userlevel() == AppState::USERLEVEL_DEVELOPER)
    {
      App_Glom* pApp = dynamic_cast<App_Glom*>(get_app_window());
      if(pApp)
        pApp->fill_menu_tables();

      //Select a different table if the current one no longer exists:
      if(!document->get_table_is_known(m_table_name))
      {
        //Open the default table, or the first table if there is no default: 
        Glib::ustring table_name = document->get_default_table();
        if(table_name.empty())
          table_name = document->get_first_table();

        show_table(table_name);
      }
    }
#endif // !GLOM_ENABLE_CLIENT_ONLY
  }
}

void Frame_Glom::on_notebook_data_switch_page(GtkNotebookPage* /* page */, guint /* page_num */)
{
  //Refill this menu, because it depends on whether list or details are visible:
  App_Glom* pApp = dynamic_cast<App_Glom*>(get_app_window());
  if(pApp)
    pApp->fill_menu_print_layouts(m_table_name);
}

void Frame_Glom::on_notebook_data_record_details_requested(const Glib::ustring& table_name, Gnome::Gda::Value primary_key_value)
{
  //Specifying a primary key value causes the details tab to be shown:
  show_table(table_name, primary_key_value);
}

void Frame_Glom::update_records_count()
{
  //Get the number of records available and the number found,
  //and all the user to find all if necessary.

  gulong count_all = 0;
  gulong count_found = 0;
  m_Notebook_Data.get_record_counts(count_all, count_found);

  std::string str_count_all, str_count_found;

  std::stringstream the_stream;
  //the_stream.imbue( current_locale );
  the_stream << count_all;
  the_stream >> str_count_all;

  if(count_found == count_all)
  {
    if(count_found != 0) //Show 0 instead of "all" when all of no records are found, to avoid confusion.
      str_count_found = _("All");
    else
      str_count_found = str_count_all;

    m_pButton_FindAll->hide();
  }
  else
  {
    std::stringstream the_stream; //Reusing the existing stream seems to produce an empty string.
    the_stream << count_found;
    the_stream >> str_count_found;

    m_pButton_FindAll->show();
  }

  m_pLabel_RecordsCount->set_text(str_count_all);
  m_pLabel_FoundCount->set_text(str_count_found);

}

void Frame_Glom::on_button_find_all()
{
  //Change the found set to all records:
  show_table(m_table_name);
}

bool Frame_Glom::get_viewing_details() const
{
  return (m_Notebook_Data.get_current_view() == Notebook_Data::DATA_VIEW_Details);
}

} //namespace Glom

