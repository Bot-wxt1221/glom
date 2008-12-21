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

#include "dialog_layout_report.h"
#include <glom/libglom/data_structure/layout/report_parts/layoutitem_groupby.h>
#include <glom/libglom/data_structure/layout/report_parts/layoutitem_summary.h>
#include <glom/libglom/data_structure/layout/report_parts/layoutitem_fieldsummary.h>
#include <glom/libglom/data_structure/layout/report_parts/layoutitem_verticalgroup.h>
#include <glom/libglom/data_structure/layout/report_parts/layoutitem_header.h>
#include <glom/libglom/data_structure/layout/report_parts/layoutitem_footer.h>
#include <glom/libglom/data_structure/layout/layoutitem_field.h>
#include <glom/libglom/data_structure/layout/layoutitem_text.h>
#include <glom/libglom/data_structure/layout/layoutitem_image.h>
#include <glom/libglom/glade_utils.h>
#include "../mode_data/dialog_choose_field.h"
#include "../layout_item_dialogs/dialog_field_layout.h"
#include "../layout_item_dialogs/dialog_group_by.h"
#include "../layout_item_dialogs/dialog_field_summary.h"
#include "../mode_data/dialog_choose_relationship.h"
//#include <libgnome/gnome-i18n.h>
#include <bakery/App/App_Gtk.h> //For util_bold_message().
#include <glibmm/i18n.h>
#include <sstream> //For stringstream

namespace Glom
{

Dialog_Layout_Report::Dialog_Layout_Report(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& refGlade)
: Dialog_Layout(cobject, refGlade, false /* No table title */),
  m_notebook_parts(0),
  m_treeview_parts_header(0),
  m_treeview_parts_footer(0),
  m_treeview_parts_main(0),
  m_treeview_available_parts(0),
  m_button_up(0),
  m_button_down(0),
  m_button_add(0),
  m_button_delete(0),
  m_button_edit(0),
  m_button_formatting(0),
  m_label_table_name(0),
  m_entry_name(0),
  m_entry_title(0),
  m_checkbutton_table_title(0)
{
  refGlade->get_widget("label_table_name", m_label_table_name);
  refGlade->get_widget("entry_name", m_entry_name);
  refGlade->get_widget("entry_title", m_entry_title);
  refGlade->get_widget("checkbutton_table_title", m_checkbutton_table_title);

  refGlade->get_widget("button_up", m_button_up);
  m_button_up->signal_clicked().connect( sigc::mem_fun(*this, &Dialog_Layout_Report::on_button_up) );

  refGlade->get_widget("button_down", m_button_down);
  m_button_down->signal_clicked().connect( sigc::mem_fun(*this, &Dialog_Layout_Report::on_button_down) );

  refGlade->get_widget("button_delete", m_button_delete);
  m_button_delete->signal_clicked().connect( sigc::mem_fun(*this, &Dialog_Layout_Report::on_button_delete) );

  refGlade->get_widget("button_add", m_button_add);
  m_button_add->signal_clicked().connect( sigc::mem_fun(*this, &Dialog_Layout_Report::on_button_add) );

  refGlade->get_widget("button_edit", m_button_edit);
  m_button_edit->signal_clicked().connect( sigc::mem_fun(*this, &Dialog_Layout_Report::on_button_edit) );

  refGlade->get_widget("button_formatting", m_button_formatting);
  m_button_formatting->signal_clicked().connect( sigc::mem_fun(*this, &Dialog_Layout_Report::on_button_formatting) );

  refGlade->get_widget("notebook_parts", m_notebook_parts);
  m_notebook_parts->set_current_page(1); //The main part, because it is used most often.

  //Available parts:
  refGlade->get_widget("treeview_available_parts", m_treeview_available_parts);
  if(m_treeview_available_parts)
  {
    //Add list of available parts:
    //These are deleted in the destructor:

    //Main parts:
    {
      m_model_available_parts_main = type_model::create();

  //     Gtk::TreeModel::iterator iterHeader = m_model_available_parts_main->append();
  //     (*iterHeader)[m_model_available_parts_main->m_columns.m_col_item] = sharedptr<LayoutItem>(static_cast<LayoutItem*>(new LayoutItem_Header()));

      Gtk::TreeModel::iterator iter = m_model_available_parts_main->append();
      (*iter)[m_model_available_parts_main->m_columns.m_col_item] = sharedptr<LayoutItem>(static_cast<LayoutItem*>(new LayoutItem_GroupBy()));

      Gtk::TreeModel::iterator iterField = m_model_available_parts_main->append(iter->children()); //Place Field under GroupBy to indicate that that's where it belongs in the actual layout.
      (*iterField)[m_model_available_parts_main->m_columns.m_col_item] = sharedptr<LayoutItem>(static_cast<LayoutItem*>(new LayoutItem_Field()));

      Gtk::TreeModel::iterator iterText = m_model_available_parts_main->append(iter->children());
      (*iterText)[m_model_available_parts_main->m_columns.m_col_item] = sharedptr<LayoutItem>(static_cast<LayoutItem*>(new LayoutItem_Text()));

      Gtk::TreeModel::iterator iterImage = m_model_available_parts_main->append(iter->children());
      (*iterImage)[m_model_available_parts_main->m_columns.m_col_item] = sharedptr<LayoutItem>(static_cast<LayoutItem*>(new LayoutItem_Image()));

      Gtk::TreeModel::iterator iterVerticalGroup = m_model_available_parts_main->append(iter->children());
      (*iterVerticalGroup)[m_model_available_parts_main->m_columns.m_col_item] = sharedptr<LayoutItem>(static_cast<LayoutItem*>(new LayoutItem_VerticalGroup()));

      iter = m_model_available_parts_main->append();
      (*iter)[m_model_available_parts_main->m_columns.m_col_item] = sharedptr<LayoutItem>(static_cast<LayoutItem*>(new LayoutItem_Summary()));
      iter = m_model_available_parts_main->append(iter->children());
      (*iter)[m_model_available_parts_main->m_columns.m_col_item] = sharedptr<LayoutItem>(static_cast<LayoutItem*>(new LayoutItem_FieldSummary()));

//     Gtk::TreeModel::iterator iterFooter = m_model_available_parts_main->append();
//     (*iterFooter)[m_model_available_parts_main->m_columns.m_col_item] = sharedptr<LayoutItem>(static_cast<LayoutItem*>(new LayoutItem_Footer()));
    }

    //Header/Footer parts:
    {
      m_model_available_parts_headerfooter = type_model::create();

      Gtk::TreeModel::iterator iterVerticalGroup = m_model_available_parts_headerfooter->append();
      (*iterVerticalGroup)[m_model_available_parts_headerfooter->m_columns.m_col_item] = sharedptr<LayoutItem>(static_cast<LayoutItem*>(new LayoutItem_VerticalGroup()));

      Gtk::TreeModel::iterator iterField = m_model_available_parts_headerfooter->append(iterVerticalGroup->children());
      (*iterField)[m_model_available_parts_headerfooter->m_columns.m_col_item] = sharedptr<LayoutItem>(static_cast<LayoutItem*>(new LayoutItem_Field()));

      Gtk::TreeModel::iterator iterText = m_model_available_parts_headerfooter->append(iterVerticalGroup->children());
      (*iterText)[m_model_available_parts_headerfooter->m_columns.m_col_item] = sharedptr<LayoutItem>(static_cast<LayoutItem*>(new LayoutItem_Text()));

      Gtk::TreeModel::iterator iterImage = m_model_available_parts_headerfooter->append(iterVerticalGroup->children());
      (*iterImage)[m_model_available_parts_headerfooter->m_columns.m_col_item] = sharedptr<LayoutItem>(static_cast<LayoutItem*>(new LayoutItem_Image()));
    }

    m_treeview_available_parts->set_model(m_model_available_parts_main);
    m_treeview_available_parts->expand_all();

    // Append the View columns:
    // Use set_cell_data_func() to give more control over the cell attributes depending on the row:

    //Name column:
    Gtk::TreeView::Column* column_part = Gtk::manage( new Gtk::TreeView::Column(_("Part")) );
    m_treeview_available_parts->append_column(*column_part);

    Gtk::CellRendererText* renderer_part = Gtk::manage(new Gtk::CellRendererText());
    column_part->pack_start(*renderer_part);
    column_part->set_cell_data_func(*renderer_part, sigc::mem_fun(*this, &Dialog_Layout_Report::on_cell_data_available_part));

    m_treeview_available_parts->set_headers_visible(false); //There's only one column, so this is not useful.

    //Respond to changes of selection:
    Glib::RefPtr<Gtk::TreeView::Selection> refSelection = m_treeview_available_parts->get_selection();
    if(refSelection)
    {
      refSelection->signal_changed().connect( sigc::mem_fun(*this, &Dialog_Layout_Report::on_treeview_available_parts_selection_changed) );
    }
  }

  refGlade->get_widget("treeview_parts_header", m_treeview_parts_header);
  setup_model(*m_treeview_parts_header, m_model_parts_header);
  refGlade->get_widget("treeview_parts_footer", m_treeview_parts_footer);
  setup_model(*m_treeview_parts_footer, m_model_parts_footer);
  refGlade->get_widget("treeview_parts_main", m_treeview_parts_main);
  setup_model(*m_treeview_parts_main, m_model_parts_main);


  show_all_children();

  m_notebook_parts->signal_switch_page().connect(sigc::mem_fun(*this, &Dialog_Layout_Report::on_notebook_switch_page));
}

Dialog_Layout_Report::~Dialog_Layout_Report()
{
}

void Dialog_Layout_Report::setup_model(Gtk::TreeView& treeview, Glib::RefPtr<type_model>& model)
{
  //Allow drag-and-drop:
  treeview.enable_model_drag_source();
  treeview.enable_model_drag_dest();

  model = type_model::create();
  treeview.set_model(model);

  // Append the View columns:
  // Use set_cell_data_func() to give more control over the cell attributes depending on the row:

  //Name column:
  Gtk::TreeView::Column* column_part = Gtk::manage( new Gtk::TreeView::Column(_("Part")) );
  treeview.append_column(*column_part);

  Gtk::CellRendererText* renderer_part = Gtk::manage(new Gtk::CellRendererText);
  column_part->pack_start(*renderer_part);
  column_part->set_cell_data_func(*renderer_part, 
    sigc::bind( sigc::mem_fun(*this, &Dialog_Layout_Report::on_cell_data_part), model));

  //Details column:
  Gtk::TreeView::Column* column_details = Gtk::manage( new Gtk::TreeView::Column(_("Details")) );
  treeview.append_column(*column_details);

  Gtk::CellRendererText* renderer_details = Gtk::manage(new Gtk::CellRendererText);
  column_details->pack_start(*renderer_details);
  column_details->set_cell_data_func(*renderer_details, 
    sigc::bind( sigc::mem_fun(*this, &Dialog_Layout_Report::on_cell_data_details), model));


  //Connect to its signal:
  //renderer_count->signal_edited().connect(
  //  sigc::bind( sigc::mem_fun(*this, &Dialog_Layout_Report::on_treeview_cell_edited_numeric), m_model_parts_main->m_columns.m_col_columns_count) );

  //Respond to changes of selection:
  Glib::RefPtr<Gtk::TreeView::Selection> refSelection = treeview.get_selection();
  if(refSelection)
  {
    refSelection->signal_changed().connect( sigc::mem_fun(*this, &Dialog_Layout_Report::on_treeview_parts_selection_changed) );
  }

  //m_model_parts_main->signal_row_changed().connect( sigc::mem_fun(*this, &Dialog_Layout_Report::on_treemodel_row_changed) );
}

sharedptr<LayoutGroup> Dialog_Layout_Report::fill_group(const Gtk::TreeModel::iterator& iter, const Glib::RefPtr<const type_model> model)
{
  if(iter)
  {
    Gtk::TreeModel::Row row = *iter;

    sharedptr<LayoutItem> pItem = row[model->m_columns.m_col_item];
    sharedptr<LayoutGroup> pGroup = sharedptr<LayoutGroup>::cast_dynamic(pItem);
    if(pGroup)
    {
      //Make sure that it contains the child items:
      fill_group_children(pGroup, iter, model);
      return glom_sharedptr_clone(pGroup);
    }

  }

  return  sharedptr<LayoutGroup>();
}

void Dialog_Layout_Report::fill_group_children(const sharedptr<LayoutGroup>& group, const Gtk::TreeModel::iterator& iter, const Glib::RefPtr<const type_model> model)
{
  if(iter)
  {
    Gtk::TreeModel::Row row = *iter;

    group->remove_all_items();
    for(Gtk::TreeModel::iterator iterChild = row.children().begin(); iterChild != row.children().end(); ++iterChild)
    {
      Gtk::TreeModel::Row row = *iterChild;
      sharedptr<LayoutItem> item = row[model->m_columns.m_col_item];

      //Recurse:
      sharedptr<LayoutGroup> child_group = sharedptr<LayoutGroup>::cast_dynamic(item);
      if(child_group)
        fill_group_children(child_group, iterChild, model);

      //std::cout << "fill_report_parts(): Adding group child: parent part type=" << group->get_part_type_name() << ", child part type=" << item->get_part_type_name() << std::endl;
      group->add_item(item);
    }

  }
}

void Dialog_Layout_Report::add_group_children(const Glib::RefPtr<type_model>& model_parts, const Gtk::TreeModel::iterator& parent, const sharedptr<const LayoutGroup>& group)
{
  for(LayoutGroup::type_list_items::const_iterator iter = group->m_list_items.begin(); iter != group->m_list_items.end(); ++iter)
  {
    sharedptr<const LayoutItem> item = *iter;
    sharedptr<const LayoutGroup> group = sharedptr<const LayoutGroup>::cast_dynamic(item);
    if(group)
    {
      sharedptr<const LayoutItem_Header> header = sharedptr<const LayoutItem_Header>::cast_dynamic(item);
      sharedptr<const LayoutItem_Footer> footer = sharedptr<const LayoutItem_Footer>::cast_dynamic(item);

      //Special-case the header and footer so that their items go into the separate treeviews:
      if(header)
        add_group_children(m_model_parts_header, parent, group); //Without the Header group being explicitly shown.
      else if(footer)
        add_group_children(m_model_parts_footer, parent, group);  //Without the Footer group being explicitly shown.
      else
      {
        add_group(model_parts, parent, group);
      }
    }
    else
    {
      Gtk::TreeModel::iterator iter = model_parts->append(parent->children());
      Gtk::TreeModel::Row row = *iter;
      row[model_parts->m_columns.m_col_item] = glom_sharedptr_clone(item);
    }
  }

  m_modified = true;
}

void Dialog_Layout_Report::add_group(const Glib::RefPtr<type_model>& model_parts, const Gtk::TreeModel::iterator& parent, const sharedptr<const LayoutGroup>& group)
{
  Gtk::TreeModel::iterator iterNewItem;
  if(!parent)
  {
    //Add it at the top-level, because nothing was selected:
    iterNewItem = model_parts->append();
  }
  else
  {
    iterNewItem = model_parts->append(parent->children());
  }

  if(iterNewItem)
  {
    Gtk::TreeModel::Row row = *iterNewItem;

    row[model_parts->m_columns.m_col_item] = sharedptr<LayoutItem>(static_cast<LayoutItem*>(group->clone()));

    add_group_children(model_parts, iterNewItem /* parent */, group);

    m_modified = true;
  }
}

//void Dialog_Layout_Report::set_document(const Glib::ustring& layout, Document_Glom* document, const Glib::ustring& table_name, const type_vecLayoutFields& table_fields)
void Dialog_Layout_Report::set_report(const Glib::ustring& table_name, const sharedptr<const Report>& report)
{
  m_modified = false;

  m_name_original = report->get_name();
  m_report = sharedptr<Report>(new Report(*report)); //Copy it, so we only use the changes when we want to.
  m_table_name = table_name;

  //Dialog_Layout::set_document(layout, document, table_name, table_fields);

  //Set the table name and title:
  m_label_table_name->set_text(table_name);

  m_entry_name->set_text(report->get_name()); 
  m_entry_title->set_text(report->get_title());
  m_checkbutton_table_title->set_active(report->get_show_table_title());

  //Update the tree models from the document

  if(true) //document)
  {


    //m_entry_table_title->set_text( document->get_table_title(table_name) );

    //document->fill_layout_field_details(m_table_name, mapGroups); //Update with full field information.

    //Show the report items:
    m_model_parts_header->clear();
    m_model_parts_main->clear();
    m_model_parts_footer->clear();

    //Add most parts to main, adding any found header or footer chidlren to those other models:
    add_group_children(m_model_parts_main, Gtk::TreeModel::iterator() /* null == top-level */, report->m_layout_group);

    //treeview_fill_sequences(m_model_parts_main, m_model_parts_main->m_columns.m_col_sequence); //The document should have checked this already, but it does not hurt to check again.
  }

  //Open all the groups:
  m_treeview_parts_header->expand_all();
  m_treeview_parts_main->expand_all();
  m_treeview_parts_footer->expand_all();

  m_notebook_parts->set_current_page(1); //The main part, because it is used most often.

  m_modified = false;
}



void Dialog_Layout_Report::enable_buttons()
{
  sharedptr<LayoutItem> layout_item_available;
  bool enable_add = false;

  //Available Parts:
  Glib::RefPtr<Gtk::TreeView::Selection> refSelectionAvailable = m_treeview_available_parts->get_selection();
  if(refSelectionAvailable)
  {
    Gtk::TreeModel::iterator iter = refSelectionAvailable->get_selected();
    if(iter)
    {
      layout_item_available = (*iter)[m_model_available_parts_main->m_columns.m_col_item];

      enable_add = true;
    }
    else
    {
      enable_add = false;
    }
  }

  sharedptr<LayoutItem> layout_item_parent;

  Gtk::TreeView* treeview = get_selected_treeview();
  if(!treeview)
    return;

  Glib::RefPtr<type_model> model = get_selected_model();

  //Parts:
  Glib::RefPtr<Gtk::TreeView::Selection> refSelection = treeview->get_selection();
  if(refSelection)
  {
    Gtk::TreeModel::iterator iter = refSelection->get_selected();
    if(iter)
    {
      layout_item_parent = (*iter)[model->m_columns.m_col_item];

      //Disable Up if It can't go any higher.
      bool enable_up = true;
      if(iter == model->children().begin())
        enable_up = false;  //It can't go any higher.

      m_button_up->set_sensitive(enable_up);


      //Disable Down if It can't go any lower.
      Gtk::TreeModel::iterator iterNext = iter;
      iterNext++;

      bool enable_down = true;
      if(iterNext == model->children().end())
        enable_down = false;

      m_button_down->set_sensitive(enable_down);

      m_button_delete->set_sensitive(true);

      //The [Formatting] button:
      bool enable_formatting = false;
      sharedptr<LayoutItem_Field> item_field = sharedptr<LayoutItem_Field>::cast_dynamic(layout_item_parent);
      if(item_field)
        enable_formatting = true; 

      m_button_formatting->set_sensitive(enable_formatting);

      //m_button_formatting->set_sensitive( (*iter)[m_columns_parts->m_columns.m_col_type] == TreeStore_Layout::TYPE_FIELD);
    }
    else
    {
      //Disable all buttons that act on a selection:
      m_button_down->set_sensitive(false);
      m_button_up->set_sensitive(false);
      m_button_delete->set_sensitive(false);
      m_button_formatting->set_sensitive(false);
    }

     //Not all parts may be children of all other parts.
    if(layout_item_available && layout_item_parent)
    {
      const bool may_be_child_of_parent = TreeStore_ReportLayout::may_be_child_of(layout_item_parent, layout_item_available);
      enable_add = may_be_child_of_parent;

      if(!may_be_child_of_parent)
      {
        //Maybe it can be a sibling of the parent instead (and that's what would happen if Add was clicked).
        sharedptr<LayoutItem> layout_item_parent_of_parent;

        Gtk::TreeModel::iterator iterParent = iter->parent();
        if(iterParent)
          layout_item_parent_of_parent = (*iterParent)[model->m_columns.m_col_item];

        enable_add = TreeStore_ReportLayout::may_be_child_of(layout_item_parent_of_parent, layout_item_available);
      }
    }
  }

  m_button_add->set_sensitive(enable_add);
}

Glib::RefPtr<Dialog_Layout_Report::type_model> Dialog_Layout_Report::get_selected_model()
{
  Glib::RefPtr <type_model> model;

  Gtk::TreeView* treeview = get_selected_treeview();
  if(treeview)
    model = Glib::RefPtr<type_model>::cast_dynamic(treeview->get_model());

  return model;
}

Glib::RefPtr<const Dialog_Layout_Report::type_model> Dialog_Layout_Report::get_selected_model() const
{
  Dialog_Layout_Report* this_nonconst = const_cast<Dialog_Layout_Report*>(this);
  return this_nonconst->get_selected_model();
}

Gtk::TreeView* Dialog_Layout_Report::get_selected_treeview()
{
  switch(m_notebook_parts->get_current_page())
  {
    //These numbers depende on the page position as defined by our .glade file.
    //We could just get the page widget, but I'd like to allow other widgets in the page if we decide to do that. It's not ideal. murrayc.
    case(0): //Header
      return m_treeview_parts_header;
    case(1): //Main
      return m_treeview_parts_main;
    case(2): //Footer
      return m_treeview_parts_footer;
    default:
    {
      std::cerr << "Dialog_Layout_Report::get_selected_treeview(): Unrecognised current notebook page:"  << m_notebook_parts->get_current_page() << std::endl;
      return 0;
    }
  }
}

const Gtk::TreeView* Dialog_Layout_Report::get_selected_treeview() const
{
  Dialog_Layout_Report* this_nonconst = const_cast<Dialog_Layout_Report*>(this);
  return this_nonconst->get_selected_treeview();
}



void Dialog_Layout_Report::on_button_delete()
{
  Gtk::TreeView* treeview = get_selected_treeview();
  if(!treeview)
    return;

  Glib::RefPtr<type_model> model = get_selected_model();

  Glib::RefPtr<Gtk::TreeSelection> refTreeSelection = treeview->get_selection();
  if(refTreeSelection)
  {
    Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
    if(iter)
    {
      model->erase(iter);

      m_modified = true;
    }
  }
}

void Dialog_Layout_Report::on_button_up()
{
  Gtk::TreeView* treeview = get_selected_treeview();
  if(!treeview)
    return;

  Glib::RefPtr<type_model> model = get_selected_model();

  Glib::RefPtr<Gtk::TreeView::Selection> refSelection = treeview->get_selection();
  if(refSelection)
  {
    Gtk::TreeModel::iterator iter = refSelection->get_selected();
    if(iter)
    {
      Gtk::TreeModel::iterator parent = iter->parent();
      bool is_first = false;
      if(parent)
        is_first = (iter == parent->children().begin());
      else
        is_first = (iter == model->children().begin());

      if(!is_first)
      {
        Gtk::TreeModel::iterator iterBefore = iter;
        --iterBefore;

        model->iter_swap(iter, iterBefore);

        m_modified = true;
      }
    }

  }

  enable_buttons();
}

void Dialog_Layout_Report::on_button_down()
{
  Gtk::TreeView* treeview = get_selected_treeview();
  if(!treeview)
    return;

  Glib::RefPtr<type_model> model = get_selected_model();

  Glib::RefPtr<Gtk::TreeView::Selection> refSelection = treeview->get_selection();
  if(refSelection)
  {
    Gtk::TreeModel::iterator iter = refSelection->get_selected();
    if(iter)
    {
      Gtk::TreeModel::iterator iterNext = iter;
      iterNext++;

      Gtk::TreeModel::iterator parent = iter->parent();
      bool is_last = false;
      if(parent)
        is_last = (iterNext == parent->children().end());
      else
        is_last = (iterNext == model->children().end());

      if(!is_last)
      {
        //Swap the sequence values, so that the one before will be after:
        model->iter_swap(iter, iterNext);

        m_modified = true;
      }
    }

  }

  enable_buttons();
}

void Dialog_Layout_Report::on_button_add()
{
  Gtk::TreeView* treeview = get_selected_treeview();
  if(!treeview)
    return;

  Glib::RefPtr<type_model> model = get_selected_model();
  Glib::RefPtr<type_model> model_available = Glib::RefPtr<type_model>::cast_dynamic(m_treeview_available_parts->get_model());

  Gtk::TreeModel::iterator parent = get_selected_group_parent();
  sharedptr<const LayoutItem> pParentPart;
  if(parent)
  {
    sharedptr<LayoutItem> temp = (*parent)[m_model_available_parts_main->m_columns.m_col_item];
    pParentPart = temp;
  }

  Gtk::TreeModel::iterator available = get_selected_available();
  sharedptr<const LayoutItem> pAvailablePart;
  if(available)
  {
    sharedptr<LayoutItem> temp = (*available)[model_available->m_columns.m_col_item];
    pAvailablePart = temp;
  }


  //Check whether the available part may be a child of the selected parent:
  if(pParentPart && !TreeStore_ReportLayout::may_be_child_of(pParentPart, pAvailablePart))
  {
    //Maybe it may be a child of the items' parent instead,
    //to become a sibling of the selected item.
    parent = (*parent).parent();
    if(parent)
    {
      sharedptr<LayoutItem> temp = (*parent)[model_available->m_columns.m_col_item];
      pParentPart = temp;

      if(!TreeStore_ReportLayout::may_be_child_of(pParentPart, pAvailablePart))
        return; //Not allowed either.
    }
  }

  //Copy the available part to the list of parts:
  if(available)
  {
    Gtk::TreeModel::iterator iter;
    if(parent)
    {
      m_treeview_parts_main->expand_row( Gtk::TreeModel::Path(parent), true);
      iter = model->append(parent->children());
    }
    else
      iter = model->append();

    (*iter)[model->m_columns.m_col_item] = sharedptr<LayoutItem>(pAvailablePart->clone());
  }

  if(parent)
    treeview->expand_row( Gtk::TreeModel::Path(parent), true);

  enable_buttons();
}


sharedptr<Relationship> Dialog_Layout_Report::offer_relationship_list()
{
  sharedptr<Relationship> result;

  try
  {
    Glib::RefPtr<Gnome::Glade::Xml> refXml = Gnome::Glade::Xml::create(Utils::get_glade_file_path("glom_developer.glade"), "dialog_choose_relationship");

    Dialog_ChooseRelationship* dialog = 0;
    refXml->get_widget_derived("dialog_choose_relationship", dialog);

    if(dialog)
    {
      dialog->set_document(get_document(), m_table_name);
      dialog->set_transient_for(*this);
      int response = dialog->run();
      dialog->hide();
      if(response == Gtk::RESPONSE_OK)
      {
        //Get the chosen relationship:
        result = dialog->get_relationship_chosen();
      }

      delete dialog;
    }
  }
  catch(const Gnome::Glade::XmlError& ex)
  {
    std::cerr << ex.what() << std::endl;
  }

  return result;
}

Gtk::TreeModel::iterator Dialog_Layout_Report::get_selected_group_parent() const
{
  //Get the selected group, or a suitable parent group, or the first group:

  Gtk::TreeModel::iterator parent;

  Gtk::TreeView* treeview = const_cast<Gtk::TreeView*>(get_selected_treeview());
  if(!treeview)
    return parent;

  Glib::RefPtr<const type_model> model = get_selected_model();


  Glib::RefPtr<Gtk::TreeSelection> refTreeSelection = treeview->get_selection();
  if(refTreeSelection)
  {
    Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
    if(iter)
    {
      Gtk::TreeModel::Row row = *iter;
      sharedptr<LayoutItem> layout_item = row[model->m_columns.m_col_item];
      if(sharedptr<LayoutGroup>::cast_dynamic(layout_item))
      {
        //Add a group under this group:
        parent = iter;
      }
      else
      {
        //Add a group under this item's group:
        parent = iter->parent();
      }
    }
  }

  return parent;
}

Gtk::TreeModel::iterator Dialog_Layout_Report::get_selected_available() const
{
  //Get the selected group, or a suitable parent group, or the first group:

  Gtk::TreeModel::iterator iter;

  Glib::RefPtr<Gtk::TreeSelection> refTreeSelection = m_treeview_available_parts->get_selection();
  if(refTreeSelection)
  {
    iter = refTreeSelection->get_selected();
  }

  return iter;
}


void Dialog_Layout_Report::on_button_formatting()
{
  Gtk::TreeView* treeview = get_selected_treeview();
  if(!treeview)
    return;

  Glib::RefPtr<type_model> model = get_selected_model();

  //Get the selected item:
  Glib::RefPtr<Gtk::TreeSelection> refTreeSelection = treeview->get_selection();
  if(refTreeSelection)
  {
    Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
    if(iter)
    {
      Gtk::TreeModel::Row row = *iter;
      sharedptr<LayoutItem> item = row[model->m_columns.m_col_item];

      sharedptr<LayoutItem_Field> field = sharedptr<LayoutItem_Field>::cast_dynamic(item);
      if(field)
      {
        sharedptr<LayoutItem_Field> field_chosen = offer_field_formatting(field, m_table_name, this);
        if(field_chosen)
        {
          *field = *field_chosen;
          model->row_changed(Gtk::TreeModel::Path(iter), iter);
        }
      }
    }
  }
}

void Dialog_Layout_Report::on_button_edit()
{
  Gtk::TreeView* treeview = get_selected_treeview();
  if(!treeview)
    return;

  Glib::RefPtr<type_model> model = get_selected_model();

  //Get the selected item:
  Glib::RefPtr<Gtk::TreeSelection> refTreeSelection = treeview->get_selection();
  if(refTreeSelection)
  {
    Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
    if(iter)
    {
      //Do something different for each type of item:
      Gtk::TreeModel::Row row = *iter;
      sharedptr<LayoutItem> item = row[model->m_columns.m_col_item];

      sharedptr<LayoutItem_FieldSummary> fieldsummary = sharedptr<LayoutItem_FieldSummary>::cast_dynamic(item);
      if(fieldsummary)
      {
        try
        {
          Glib::RefPtr<Gnome::Glade::Xml> refXml = Gnome::Glade::Xml::create(Utils::get_glade_file_path("glom_developer.glade"), "dialog_field_summary");

          Dialog_FieldSummary* dialog = 0;
          refXml->get_widget_derived("dialog_field_summary", dialog);

          if(dialog)
          {
            add_view(dialog);
            dialog->set_item(fieldsummary, m_table_name);
            dialog->set_transient_for(*this);

            const int response = dialog->run();
            dialog->hide();

            if(response == Gtk::RESPONSE_OK)
            {
              //Get the chosen relationship:
              sharedptr<LayoutItem_FieldSummary> chosenitem = dialog->get_item();
              if(chosenitem)
              {
                *fieldsummary = *chosenitem; //TODO_Performance.

                model->row_changed(Gtk::TreeModel::Path(iter), iter); //TODO: Add row_changed(iter) to gtkmm?
                m_modified = true;
              }
            }

            remove_view(dialog);
            delete dialog;
          }
        }
        catch(const Gnome::Glade::XmlError& ex)
        {
          std::cerr << ex.what() << std::endl;
        }
      }
      else
      {
        sharedptr<LayoutItem_Field> field = sharedptr<LayoutItem_Field>::cast_dynamic(item);
        if(field)
        {
          sharedptr<LayoutItem_Field> chosenitem = offer_field_list(field, m_table_name, this);
          if(chosenitem)
          {
            *field = *chosenitem; //TODO_Performance.
            model->row_changed(Gtk::TreeModel::Path(iter), iter); //TODO: Add row_changed(iter) to gtkmm?
            m_modified = true;
          }
        }
        else
        {
          sharedptr<LayoutItem_Text> layout_item_text = sharedptr<LayoutItem_Text>::cast_dynamic(item);
          if(layout_item_text)
          {
            sharedptr<LayoutItem_Text> chosen = offer_textobject(layout_item_text);
            if(chosen)
            {
              *layout_item_text = *chosen;
              model->row_changed(Gtk::TreeModel::Path(iter), iter); //TODO: Add row_changed(iter) to gtkmm?
              m_modified = true;
            }
          }
          else
          {
            sharedptr<LayoutItem_Image> layout_item_image = sharedptr<LayoutItem_Image>::cast_dynamic(item);
            if(layout_item_image)
            {
              sharedptr<LayoutItem_Image> chosen = offer_imageobject(layout_item_image);
              if(chosen)
              {
                *layout_item_image = *chosen;
                model->row_changed(Gtk::TreeModel::Path(iter), iter); //TODO: Add row_changed(iter) to gtkmm?
                m_modified = true;
              }
            }
            else
            {
              sharedptr<LayoutItem_GroupBy> group_by = sharedptr<LayoutItem_GroupBy>::cast_dynamic(item);
              if(group_by)
              {
                try
                {
                  Glib::RefPtr<Gnome::Glade::Xml> refXml = Gnome::Glade::Xml::create(Utils::get_glade_file_path("glom_developer.glade"), "dialog_group_by");
  
                  Dialog_GroupBy* dialog = 0;
                  refXml->get_widget_derived("dialog_group_by", dialog);
  
                  if(dialog)
                  {
                    add_view(dialog);
                    dialog->set_item(group_by, m_table_name);
                    dialog->set_transient_for(*this);
  
                    const int response = dialog->run();
                    dialog->hide();
  
                    if(response == Gtk::RESPONSE_OK)
                    {
                      //Get the chosen relationship:
                      sharedptr<LayoutItem_GroupBy> chosenitem = dialog->get_item();
                      if(chosenitem)
                      {
                        *group_by = *chosenitem;
                        model->row_changed(Gtk::TreeModel::Path(iter), iter); //TODO: Add row_changed(iter) to gtkmm?
                        m_modified = true;
                      }
                    }
  
                    remove_view(dialog);
                    delete dialog;
                  }
                }
                catch(const Gnome::Glade::XmlError& ex)
                {
                  std::cerr << ex.what() << std::endl;
                }
              }
            }
          }
        }
      }
    }
  }
}

void Dialog_Layout_Report::save_to_document()
{
  Dialog_Layout::save_to_document();
}

void Dialog_Layout_Report::on_treeview_available_parts_selection_changed()
{
  enable_buttons();
}

void Dialog_Layout_Report::on_treeview_parts_selection_changed()
{
  enable_buttons();
}

void Dialog_Layout_Report::on_cell_data_part(Gtk::CellRenderer* renderer, const Gtk::TreeModel::iterator& iter, const Glib::RefPtr<type_model>& model)
{
  //TODO: If we ever use this as a real layout tree, then let's add icons for each type.

  //Set the view's cell properties depending on the model's data:
  Gtk::CellRendererText* renderer_text = dynamic_cast<Gtk::CellRendererText*>(renderer);
  if(renderer_text)
  {
    if(iter)
    {
      Gtk::TreeModel::Row row = *iter;

      sharedptr<LayoutItem> pItem = row[model->m_columns.m_col_item];
      const Glib::ustring part = pItem->get_part_type_name();

      renderer_text->property_text() = part;
      renderer_text->property_editable() = false; //Part names can never be edited.
    }
  }
}

void Dialog_Layout_Report::on_cell_data_details(Gtk::CellRenderer* renderer, const Gtk::TreeModel::iterator& iter, const Glib::RefPtr<type_model>& model)
{
//Set the view's cell properties depending on the model's data:
  Gtk::CellRendererText* renderer_text = dynamic_cast<Gtk::CellRendererText*>(renderer);
  if(renderer_text)
  {
    if(iter)
    {
      Glib::ustring text;

      Gtk::TreeModel::Row row = *iter;
      sharedptr<LayoutItem> pItem = row[model->m_columns.m_col_item];
      renderer_text->property_text() = pItem->get_layout_display_name();
      renderer_text->property_editable() = false;
    }
  }
}


void Dialog_Layout_Report::on_cell_data_available_part(Gtk::CellRenderer* renderer, const Gtk::TreeModel::iterator& iter)
{
  //TODO: If we ever use this as a real layout tree, then let's add icons for each type.

  //Set the view's cell properties depending on the model's data:
  Gtk::CellRendererText* renderer_text = dynamic_cast<Gtk::CellRendererText*>(renderer);
  if(renderer_text)
  {
    if(iter)
    {
      Gtk::TreeModel::Row row = *iter;
      Glib::RefPtr<type_model> model = Glib::RefPtr<type_model>::cast_dynamic(m_treeview_available_parts->get_model());
      sharedptr<LayoutItem> pItem = row[model->m_columns.m_col_item];
      Glib::ustring part = pItem->get_part_type_name();

      renderer_text->property_text() = part;
      renderer_text->property_editable() = false; //Part names can never be edited.
    }
  }
}

Glib::ustring Dialog_Layout_Report::get_original_report_name() const
{
  return m_name_original;
}

sharedptr<Report> Dialog_Layout_Report::get_report()
{
  m_report->set_name( m_entry_name->get_text() );
  m_report->set_title( m_entry_title->get_text() );
  m_report->set_show_table_title( m_checkbutton_table_title->get_active() );

  m_report->m_layout_group->remove_all_items();

  m_report->m_layout_group->remove_all_items();

  //The Header and Footer parts are implicit (they are the whole header or footer treeview)
  sharedptr<LayoutItem_Header> header = sharedptr<LayoutItem_Header>::create();
  sharedptr<LayoutGroup> group_temp = header;
  fill_report_parts(group_temp, m_model_parts_header);
  if(header->get_items_count())
    m_report->m_layout_group->add_item(header);

  fill_report_parts(m_report->m_layout_group, m_model_parts_main);

  sharedptr<LayoutItem_Footer> footer = sharedptr<LayoutItem_Footer>::create();
  group_temp = footer;
  fill_report_parts(group_temp, m_model_parts_footer);
  if(footer->get_items_count())
    m_report->m_layout_group->add_item(footer);

  return m_report;
}

void Dialog_Layout_Report::fill_report_parts(sharedptr<LayoutGroup>& group, const Glib::RefPtr<const type_model> parts_model)
{
  for(Gtk::TreeModel::iterator iter = parts_model->children().begin(); iter != parts_model->children().end(); ++iter)
  {
    //Recurse into a group if necessary:
    sharedptr<LayoutGroup> group_child = fill_group(iter, parts_model);
    if(group_child)
    {
      //Add the group:
      group->add_item(group_child);
    }
    else
    {
      sharedptr<LayoutItem> item = (*iter)[parts_model->m_columns.m_col_item];
      if(item)
      {
        group->add_item(item);
      }
    }
  }
}

void Dialog_Layout_Report::on_notebook_switch_page(GtkNotebookPage*, guint page_number)
{
  //Change the list of available parts, depending on the destination treeview:
  Glib::RefPtr<type_model> model_available_parts;

  switch(page_number)
  {
    case(0): //Header
    case(2): //Footer
      model_available_parts = m_model_available_parts_headerfooter;
      break;
    case(1): //Main
    default:
      model_available_parts = m_model_available_parts_main;
      break;
  }

  m_treeview_available_parts->set_model(model_available_parts);
  m_treeview_available_parts->expand_all();

  //Enable the correct buttons for the now-visible TreeView:
  enable_buttons();
}


} //namespace Glom
