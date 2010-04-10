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

#include "dialog_groups_list.h"
#include "dialog_user.h"
#include "dialog_users_list.h"
#include "dialog_new_group.h"
#include <libglom/standard_table_prefs_fields.h>
#include <glom/glade_utils.h>
#include <glom/glom_privs.h>
//#include <libgnome/gnome-i18n.h>
#include <glom/utils_ui.h> //For bold_message()).
#include <glibmm/i18n.h>

namespace Glom
{

const char* Dialog_GroupsList::glade_id("window_groups");
const bool Dialog_GroupsList::glade_developer(true);

Dialog_GroupsList::Dialog_GroupsList(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
: Gtk::Dialog(cobject),
  m_treeview_groups(0),
  m_button_group_new(0),
  m_button_group_delete(0),
  m_button_group_users(0)
{
  //set_default_size(600, -1);

  builder->get_widget("treeview_groups", m_treeview_groups);
  builder->get_widget("treeview_tables", m_treeview_tables);

  m_model_groups = Gtk::ListStore::create(m_model_columns_groups);
  m_model_tables = Gtk::ListStore::create(m_model_columns_tables);

  fill_group_list();
  //fill_table_list();

  m_treeview_groups->set_model(m_model_groups);
  m_treeview_tables->set_model(m_model_tables);

  // Append the View columns:

  //Groups:
  Gtk::CellRendererText* pCell = Gtk::manage(new Gtk::CellRendererText);
  Gtk::TreeView::Column* pViewColumn = Gtk::manage(new Gtk::TreeView::Column(_("Name"), *pCell) );
  pViewColumn->set_cell_data_func(*pCell, sigc::mem_fun(*this, &Dialog_GroupsList::on_cell_data_group_name));
  m_treeview_groups->append_column(*pViewColumn);

  m_treeview_groups->append_column(_("Description"), m_model_columns_groups.m_col_description);

  //Tables:
  m_treeview_tables->append_column(_("Table"), m_model_columns_tables.m_col_title);


  treeview_append_bool_column(*m_treeview_tables, _("View"), m_model_columns_tables.m_col_view,
    sigc::mem_fun( *this, &Dialog_GroupsList::on_treeview_tables_toggled_view) );

  treeview_append_bool_column(*m_treeview_tables, _("Edit"), m_model_columns_tables.m_col_edit,
    sigc::mem_fun( *this, &Dialog_GroupsList::on_treeview_tables_toggled_edit) );

  treeview_append_bool_column(*m_treeview_tables, _("Create"), m_model_columns_tables.m_col_create,
    sigc::mem_fun( *this, &Dialog_GroupsList::on_treeview_tables_toggled_create) );

  treeview_append_bool_column(*m_treeview_tables, _("Delete"), m_model_columns_tables.m_col_delete,
    sigc::mem_fun( *this, &Dialog_GroupsList::on_treeview_tables_toggled_delete) );


  Glib::RefPtr<Gtk::TreeView::Selection> refSelection = m_treeview_groups->get_selection();
  if(refSelection)
  {
    refSelection->signal_changed().connect( sigc::mem_fun(*this, &Dialog_GroupsList::on_treeview_groups_selection_changed) );
  }

  refSelection = m_treeview_tables->get_selection();
  if(refSelection)
  {
    refSelection->signal_changed().connect( sigc::mem_fun(*this, &Dialog_GroupsList::on_treeview_tables_selection_changed) );
  }

  builder->get_widget("button_group_new", m_button_group_new);
  m_button_group_new->signal_clicked().connect( sigc::mem_fun(*this, &Dialog_GroupsList::on_button_group_new) );

  builder->get_widget("button_group_delete", m_button_group_delete);
  m_button_group_delete->signal_clicked().connect( sigc::mem_fun(*this, &Dialog_GroupsList::on_button_group_delete) );

  builder->get_widget("button_group_users", m_button_group_users);
  m_button_group_users->signal_clicked().connect( sigc::mem_fun(*this, &Dialog_GroupsList::on_button_group_users) );

  enable_buttons();

  show_all_children();
}

Dialog_GroupsList::~Dialog_GroupsList()
{
}

/*
void Dialog_GroupsList::set_document(const Glib::ustring& layout, Document* document, const Glib::ustring& table_name, const type_vecLayoutFields& table_fields)
{
  m_modified = false;

  Dialog_Layout::set_document(layout, document, table_name, table_fields);


  //Update the tree models from the document
  if(document)
  {
    //Set the table name and title:
    m_label_table_name->set_text(table_name);
    m_entry_table_title->set_text( document->get_table_title(table_name) );

    Document::type_list_layout_groups mapGroups = document->get_data_layout_groups_plus_new_fields(layout, m_table_name, m_layout_platform);

    //If no information is stored in the document, then start with something:

    if(mapGroups.empty())
    {
      LayoutGroup group;
      group.set_name("main");
      group.m_columns_count = 2;

      guint field_sequence = 1; //0 means no sequence
      for(type_vecLayoutFields::const_iterator iter = table_fields.begin(); iter != table_fields.end(); ++iter)
      {
        LayoutItem_Field item = *iter;

        group.add_item(item);

        ++field_sequence;
      }

      mapGroups[1] = group;
    }

    //Show the field layout
    typedef std::list< Glib::ustring > type_listStrings;

    m_model_groups->clear();

    for(Document::type_list_layout_groups::const_iterator iter = mapGroups.begin(); iter != mapGroups.end(); ++iter)
    {
      sharedptr<const LayoutGroup> group = iter->second;

      add_group(Gtk::TreeModel::iterator(), group);
    }

    //treeview_fill_sequences(m_model_groups, m_model_groups->m_columns.m_col_sequence); //The document should have checked this already, but it does not hurt to check again.
  }

  //Open all the groups:
  m_treeview_groups->expand_all();

  m_modified = false;

}
*/

void Dialog_GroupsList::enable_buttons()
{
  if(!m_button_group_users)
    return;

  Glib::RefPtr<Gtk::TreeView::Selection> refSelection = m_treeview_groups->get_selection();
  if(refSelection)
  {
    Gtk::TreeModel::iterator iter = refSelection->get_selected();
    if(iter)
    {
      m_button_group_users->set_sensitive(true);
      m_button_group_delete->set_sensitive(true);
    }
    else
    {
      //Disable all buttons that act on a selection:
      m_button_group_users->set_sensitive(false);
      m_button_group_delete->set_sensitive(false);
    }
  }

}

void Dialog_GroupsList::on_button_group_delete()
{
  Glib::RefPtr<Gtk::TreeSelection> refTreeSelection = m_treeview_groups->get_selection();
  if(refTreeSelection)
  {
    Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
    if(iter)
    {
      Gtk::TreeModel::Row row = *iter;

      const Glib::ustring group = row[m_model_columns_groups.m_col_name];
      if(!group.empty())
      {
        //TODO: Prevent deletion of standard groups
        Gtk::MessageDialog dialog(Utils::bold_message(_("Delete Group")), true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_OK_CANCEL);
        dialog.set_secondary_text(_("Are your sure that you wish to delete this group?"));
        dialog.set_icon_name("glom");
        dialog.set_transient_for(*this);

        int response = dialog.run();
        dialog.hide();

        if(response == Gtk::RESPONSE_OK)
        {
          Glib::ustring strQuery = "DROP GROUP \"" + group + "\"";
          const bool test = query_execute(strQuery);
          if(!test)
            std::cerr << "Box_DB_Table_Definition::on_adddel_delete(): DROP GROUP failed." << std::endl;

          fill_group_list();
        }
      }

      //m_modified = true;
    }
  }
}

void Dialog_GroupsList::on_button_group_new()
{
  Dialog_NewGroup* dialog = 0;
  Utils::get_glade_widget_derived_with_warning(dialog);

  dialog->set_icon_name("glom");
  dialog->set_transient_for(*this);
  const int response = Glom::Utils::dialog_run_with_help(dialog, "dialog_new_group");

  const Glib::ustring group_name = dialog->m_entry_name->get_text();

  delete dialog;

  if(response != Gtk::RESPONSE_OK)
    return;

  if(!group_name.empty())
  {
    const Glib::ustring strQuery = "CREATE GROUP \"" + group_name + "\"";
    const bool test = query_execute(strQuery);
    if(!test)
      std::cout << "Dialog_GroupsList::on_button_group_new(): CREATE GROUP failed." << std::endl;

    //Give the new group some sensible default privileges:
    Privileges priv;
    priv.m_view = true;
    priv.m_edit = true;

    Document::type_listTableInfo table_list = get_document()->get_tables(true /* plus system prefs */);

    for(Document::type_listTableInfo::const_iterator iter = table_list.begin(); iter != table_list.end(); ++iter)
    {
      Privs::set_table_privileges(group_name, (*iter)->get_name(), priv);
    }

    //Let them edit the autoincrements too:
    Privs::set_table_privileges(group_name, GLOM_STANDARD_TABLE_AUTOINCREMENTS_TABLE_NAME, priv);

    fill_group_list();
  }
}

void Dialog_GroupsList::on_button_group_users()
{

  //Get the selected item:
  Glib::RefPtr<Gtk::TreeSelection> refTreeSelection = m_treeview_groups->get_selection();
  if(refTreeSelection)
  {
    Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
    if(iter)
    {
      Gtk::TreeModel::Row row = *iter;
      const Glib::ustring group_name = row[m_model_columns_groups.m_col_name];

      Dialog_UsersList* dialog = 0;
      Utils::get_glade_widget_derived_with_warning(dialog);

      dialog->set_icon_name("glom");
      dialog->set_transient_for(*this);
      add_view(dialog); //Give it access to the document.

      dialog->set_group(group_name);

      Glom::Utils::dialog_run_with_help(dialog, "window_groups");

      remove_view(dialog);
      delete dialog;

      fill_group_list();
    }
  }
}

void Dialog_GroupsList::save_to_document()
{


  //if(m_modified)
  //{

  //}
}

void Dialog_GroupsList::on_treeview_tables_selection_changed()
{
  enable_buttons();
}

Glib::ustring Dialog_GroupsList::get_selected_group() const
{
  Glib::RefPtr<Gtk::TreeView::Selection> refSelection = m_treeview_groups->get_selection();
  if(refSelection)
  {
    Gtk::TreeModel::iterator iter = refSelection->get_selected();
    if(iter)
    {
      Gtk::TreeModel::Row row = *iter;
      return row[m_model_columns_groups.m_col_name];
    }
  }

  return Glib::ustring();
}

void Dialog_GroupsList::on_treeview_groups_selection_changed()
{
  //Update the tables list for the currently-selected group:
  fill_table_list( get_selected_group() );

  enable_buttons();
}

void Dialog_GroupsList::fill_group_list()
{
  //Fill the model rows:
  m_model_groups->clear();

  type_vec_strings group_list = Privs::get_database_groups();
  for(type_vec_strings::const_iterator iter = group_list.begin(); iter != group_list.end(); ++iter)
  {
    Gtk::TreeModel::iterator iterTree = m_model_groups->append();
    Gtk::TreeModel::Row row = *iterTree;

    row[m_model_columns_groups.m_col_name] = *iter;

    if(*iter == GLOM_STANDARD_GROUP_NAME_DEVELOPER)
      row[m_model_columns_groups.m_col_description] = _("Full access.");
  }

  if(m_treeview_groups && m_treeview_groups->get_model()) //Avoid a warning.
  {
    //Select the first item, so that there is always something in the tables TreeView:
    Glib::RefPtr<Gtk::TreeView::Selection> refSelection = m_treeview_groups->get_selection();
    if(refSelection)
    {
      Gtk::TreeModel::iterator iterFirst = m_model_groups->children().begin();
      if(iterFirst)
      {
        refSelection->select(iterFirst);
      }
    }
  }
}

void Dialog_GroupsList::fill_table_list(const Glib::ustring& group_name)
{
  //Fill the model rows:
  m_model_tables->clear();

  Document* pDocument = get_document();
  if(pDocument)
  {
    // Make sure that these are in the document,
    // so that the correct groups will be created if we recreate the database from the document:
    GroupInfo group_info;
    group_info.set_name(group_name);

    Document::type_listTableInfo table_list = pDocument->get_tables(true /* plus system prefs */);

    for(Document::type_listTableInfo::const_iterator iter = table_list.begin(); iter != table_list.end(); ++iter)
    {
      Gtk::TreeModel::iterator iterTree = m_model_tables->append();
      Gtk::TreeModel::Row row = *iterTree;

      const Glib::ustring table_name = (*iter)->get_name();

      row[m_model_columns_tables.m_col_name] = table_name;
      row[m_model_columns_tables.m_col_title] = (*iter)->get_title_or_name();

      const Privileges privs = Privs::get_table_privileges(group_name, table_name);
      row[m_model_columns_tables.m_col_view] = privs.m_view;
      row[m_model_columns_tables.m_col_edit] = privs.m_edit;
      row[m_model_columns_tables.m_col_create] = privs.m_create;
      row[m_model_columns_tables.m_col_delete] = privs.m_delete;


      group_info.m_map_privileges[table_name] = privs;
    }

     pDocument->set_group(group_info);
  }
}

void Dialog_GroupsList::load_from_document()
{
  //Ensure that the glom_developer group exists.
  add_standard_groups();

  fill_group_list();
  //fill_table_list();
}

void Dialog_GroupsList::treeview_append_bool_column(Gtk::TreeView& treeview, const Glib::ustring& title, Gtk::TreeModelColumn<bool>& model_column, const sigc::slot<void,  const Glib::ustring&>& slot_toggled)
{
  Gtk::CellRendererToggle* pCellRenderer = Gtk::manage( new Gtk::CellRendererToggle() );

  //GTK+'s "activatable" really means "editable":
  pCellRenderer->property_activatable() = true;

  Gtk::TreeView::Column* pViewColumn = Gtk::manage( new Gtk::TreeView::Column(title, *pCellRenderer) );
  pViewColumn->set_renderer(*pCellRenderer, model_column); //render it via the default "text" property.

  treeview.append_column(*pViewColumn);

  pCellRenderer->signal_toggled().connect( slot_toggled );
}

bool Dialog_GroupsList::set_table_privilege(const Glib::ustring& table_name, const Glib::ustring& group_name, bool grant, enumPriv priv)
{
  if(group_name.empty() || table_name.empty())
    return false;

  if(group_name == GLOM_STANDARD_GROUP_NAME_DEVELOPER)
    return false;

  //Change the permission in the database:

  //Build the SQL statement:

  //Grant or revoke:
  Glib::ustring strQuery;
  if(grant)
    strQuery = "GRANT";
  else
    strQuery = "REVOKE";

  //What to grant or revoke:
  Glib::ustring strPrivilege;
  if(priv == PRIV_VIEW)
    strPrivilege = "SELECT";
  else if(priv == PRIV_EDIT)
    strPrivilege = "UPDATE";
  else if(priv == PRIV_CREATE)
    strPrivilege = "INSERT";
  else if(priv == PRIV_DELETE)
    strPrivilege = "DELETE";

  strQuery += " " + strPrivilege + " ON \"" + table_name + "\" ";

  //This must match the Grant or Revoke:
  if(grant)
    strQuery += "TO";
  else
    strQuery += "FROM";

  strQuery += " GROUP \"" + group_name + "\"";

  const bool test = query_execute(strQuery); //TODO: Handle errors.
  if(!test)
    std::cerr << "Dialog_GroupsList::set_table_privilege(): GRANT/REVOKE failed." << std::endl;

  return test;
}

void Dialog_GroupsList::on_treeview_tables_toggled_view(const Glib::ustring& path_string)
{
  Gtk::TreeModel::Path path(path_string);

  //Get the row from the path:
  Gtk::TreeModel::iterator iter = m_model_tables->get_iter(path);
  if(iter)
  {
    Gtk::TreeRow row = *iter;

    //Toggle the value in the model:
    bool bActive = row[m_model_columns_tables.m_col_view];
    bActive = !bActive;

    const Glib::ustring group_name = get_selected_group();
    const Glib::ustring table_name = row[m_model_columns_tables.m_col_name];

    bool test = set_table_privilege(table_name, group_name, bActive, PRIV_VIEW);
    if(test)
      row[m_model_columns_tables.m_col_view] = bActive;

    //If the group cannot view, then it should not do anything else either:
    if(!bActive)
    {
      bool test = set_table_privilege(table_name, group_name, bActive, PRIV_EDIT);
      if(test)
        row[m_model_columns_tables.m_col_edit] = false;

      test = set_table_privilege(table_name, group_name, bActive, PRIV_CREATE);
      if(test)
        row[m_model_columns_tables.m_col_create] = false;

      test = set_table_privilege(table_name, group_name, bActive, PRIV_DELETE);
      if(test)
        row[m_model_columns_tables.m_col_delete] = false;
    }
  }
}

void Dialog_GroupsList::on_treeview_tables_toggled_edit(const Glib::ustring& path_string)
{
  Gtk::TreeModel::Path path(path_string);

  //Get the row from the path:
  Gtk::TreeModel::iterator iter = m_model_tables->get_iter(path);
  if(iter)
  {
    Gtk::TreeRow row = *iter;

    //Toggle the value in the model:
    bool bActive = row[m_model_columns_tables.m_col_edit];
    bActive = !bActive;

    const Glib::ustring group_name = get_selected_group();
    const Glib::ustring table_name = row[m_model_columns_tables.m_col_name];

    bool test = set_table_privilege(table_name, group_name, bActive, PRIV_EDIT);

    if(test)
      row[m_model_columns_tables.m_col_edit] = bActive;
  }
}

void Dialog_GroupsList::on_treeview_tables_toggled_create(const Glib::ustring& path_string)
{
  Gtk::TreeModel::Path path(path_string);

  //Get the row from the path:
  Gtk::TreeModel::iterator iter = m_model_tables->get_iter(path);
  if(iter)
  {
    Gtk::TreeRow row = *iter;

    //Toggle the value in the model:
    bool bActive = row[m_model_columns_tables.m_col_create];
    bActive = !bActive;

    const Glib::ustring group_name = get_selected_group();
    const Glib::ustring table_name = row[m_model_columns_tables.m_col_name];

    const bool test = set_table_privilege(table_name, group_name, bActive, PRIV_CREATE);

    if(test)
      row[m_model_columns_tables.m_col_create] = bActive;
  }
}

void Dialog_GroupsList::on_treeview_tables_toggled_delete(const Glib::ustring& path_string)
{
  Gtk::TreeModel::Path path(path_string);

  //Get the row from the path:
  Gtk::TreeModel::iterator iter = m_model_tables->get_iter(path);
  if(iter)
  {
    Gtk::TreeRow row = *iter;

    //Toggle the value in the model:
    bool bActive = row[m_model_columns_tables.m_col_delete];
    bActive = !bActive;

    const Glib::ustring group_name = get_selected_group();
    const Glib::ustring table_name = row[m_model_columns_tables.m_col_name];

    const bool test = set_table_privilege(table_name, group_name, bActive, PRIV_DELETE);

    if(test)
      row[m_model_columns_tables.m_col_delete] = bActive;
  }
}

void Dialog_GroupsList::on_cell_data_group_name(Gtk::CellRenderer* renderer, const Gtk::TreeModel::iterator& iter)
{
 //Set the view's cell properties depending on the model's data:
  Gtk::CellRendererText* renderer_text = dynamic_cast<Gtk::CellRendererText*>(renderer);
  if(renderer_text)
  {
    if(iter)
    {
      Gtk::TreeModel::Row row = *iter;

      Glib::ustring name = row[m_model_columns_groups.m_col_name];

      renderer_text->property_text() = Privs::get_user_visible_group_name(name);
    }
  }
}

} //namespace Glom


