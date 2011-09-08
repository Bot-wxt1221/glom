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

#include "combo.h"
#include <libglom/data_structure/glomconversions.h>
#include <gtkmm/messagedialog.h>
#include <glom/mode_data/datawidget/cellcreation.h>
#include <glom/dialog_invalid_data.h>
#include <glom/mode_data/datawidget/treemodel_db_withextratext.h>
#include <libglom/data_structure/glomconversions.h>
#include <libglom/db_utils.h>
#include <glom/application.h>
#include <glom/utils_ui.h>
#include <glibmm/i18n.h>
//#include <sstream> //For stringstream

#include <locale>     // for locale, time_put
#include <ctime>     // for struct tm
#include <iostream>   // for cout, endl

namespace Glom
{

namespace DataWidgetChildren
{

ComboGlom::ComboGlom(bool has_entry)
: Gtk::ComboBox(has_entry),
  ComboChoicesWithTreeModel()
{
#ifndef GLOM_ENABLE_CLIENT_ONLY
  setup_menu();
#endif // !GLOM_ENABLE_CLIENT_ONLY

  //if(m_glom_type == Field::TYPE_NUMERIC)
   // get_entry()->set_alignment(1.0); //Align numbers to the right.

  //Let the combo be big enough:
  set_popup_fixed_width(false);
}

ComboGlom::~ComboGlom()
{
}

void ComboGlom::set_choices_fixed(const FieldFormatting::type_list_values& list_values)
{
  ComboChoicesWithTreeModel::set_choices_fixed(list_values);

  Glib::RefPtr<Gtk::TreeModel> model = get_choices_model();
  if(!model)
  {
    std::cerr << G_STRFUNC << ": model is null." << std::endl;
    return;
  }

  //Show the model in the view:
  set_model(model);

  if(get_has_entry())
  {
    set_entry_text_column( get_fixed_model_text_column() );
  }
  else
  {
    clear(); //This breaks GtkCombo with has-entry.
  }

  Glib::RefPtr<Gtk::CellAreaBox> cell_area = 
    Glib::RefPtr<Gtk::CellAreaBox>::cast_dynamic(get_area());
  if(!cell_area)
  {
    std::cerr << G_STRFUNC << ": Unexpected or null CellArea type." << std::endl;
    return;
  }
  
  guint columns_count = model->get_n_columns();
  if(columns_count)
    columns_count -= 1; //The last one is the just the extra text-equivalent of the first one, for GtkComboBox wth has-entry=true.

  for(guint i = 0; i < columns_count; ++i)
  {
    //set_entry_text_column() adds its own CellRenderer,
    //which we cannot replace without confusing (and crashing) GtkComboBox.
    //We used the special get_fixed_model_text_column() column for that,
    //so we don't need to add another cell renderer for the value-equivalent of that column:
    if(i == 0 && get_has_entry())
      continue;

    Gtk::CellRendererText* cell = Gtk::manage(new Gtk::CellRendererText);
    cell->property_xalign() = 0.0f;

    //Use the renderer:
    cell_area->pack_start(*cell, true /* expand */, true /* align */, true /* fixed */);

    //Make the renderer render the column:
    add_attribute(*cell, "text", i);
  }
}

void ComboGlom::set_choices_related(const Document* document, const sharedptr<const LayoutItem_Field>& layout_field, const Gnome::Gda::Value& foreign_key_value)
{
  ComboChoicesWithTreeModel::set_choices_related(document, layout_field, foreign_key_value);

  Glib::RefPtr<Gtk::TreeModel> model = get_choices_model();
  if(!model)
  {
    std::cerr << G_STRFUNC << ": model is null." << std::endl;
    return;
  }

  //Show the model in the view:
  set_model(model);

  if(get_has_entry())
  {
    Glib::RefPtr<DbTreeModelWithExtraText> model_db =
      Glib::RefPtr<DbTreeModelWithExtraText>::cast_dynamic(model);
    if(model_db)
    {
      const int text_col = model_db->get_text_column();
      //const GType debug_type = model_db->get_column_type(text_col);
      //std::cout << "DEBUG: text_col=" << text_col << ", debug_type=" << g_type_name(debug_type) << std::endl;
      set_entry_text_column(text_col);
    }
    else
    {
      std::cerr << G_STRFUNC << ": The model is not a DbTreeModelWithExtraText." << std::endl;
      return;
    }
  }
  else
  {
    clear(); //This breaks GtkCombo with has-entry.
  }

  guint model_column_index = 0;
  for(type_vec_const_layout_items::const_iterator iter = m_db_layout_items.begin(); iter != m_db_layout_items.end(); ++iter)
  {
    const sharedptr<const LayoutItem> layout_item = *iter;
    if(!layout_item) //column_info.m_visible)
    {
      ++model_column_index;
      continue;
    }

    //set_entry_text_column() adds its own CellRenderer,
    //which we cannot replace without confusing (and crashing) GtkComboBox.
    if(model_column_index == 0 && get_has_entry())
    {
       ++model_column_index;
      continue;
    }

    Gtk::CellRenderer* cell = create_cell(layout_item, m_table_name, document, get_fixed_cell_height(*this));

    //Add the ViewColumn:
    if(cell)
    {
      //Use the renderer:
      //We don't expand the first column, so we can align the other columns.
      //Otherwise the other columns appear center-aligned.
      //This bug is relevant: https://bugzilla.gnome.org/show_bug.cgi?id=629133
      pack_start(*cell, false);

      cell_connect_cell_data_func(this, cell, model_column_index);
    }

     ++model_column_index;
  } //for
}

void ComboGlom::check_for_change()
{
  m_signal_edited.emit();
}

void ComboGlom::set_value(const Gnome::Gda::Value& value)
{
  sharedptr<const LayoutItem_Field> layout_item = sharedptr<const LayoutItem_Field>::cast_dynamic(get_layout_item());
  if(!layout_item)
    return;

  m_old_value = value;

  Glib::RefPtr<Gtk::TreeModel> model = get_choices_model();
  if(!model)
  {
    std::cerr << G_STRFUNC << ": model is null." << std::endl;
    return;
  }

  bool found = false;
  for(Gtk::TreeModel::iterator iter = model->children().begin(); iter != model->children().end(); ++iter)
  {
    const Gtk::TreeModel::Row row = *iter;
    Gnome::Gda::Value this_value;
    row.get_value(0, this_value);

    if(this_value == value)
    {
      found = true;
      set_active(iter);
      break;
    }
  }

  if(!found)
  {
    //Not found, so mark it as blank:
    unset_active();
  }

  //Show a different color if the value is numeric, if that's specified:
  if(layout_item->get_glom_type() == Field::TYPE_NUMERIC)
  {
    std::vector<Gtk::CellRenderer*> cells = get_cells();
    if(cells.empty())
      return;

    Gtk::CellRendererText* cell = dynamic_cast<Gtk::CellRendererText*>(cells[0]);
    if(!cell)
      return;

    const Glib::ustring fg_color =
    layout_item->get_formatting_used().get_text_format_color_foreground_to_use(value);
    if(fg_color.empty())
    {
      //GtkComboBox doesn't interpret "" as an unset. TODO: Fix that?
      cell->property_foreground_set() = false;
    }
    else
      cell->property_foreground() = fg_color;
  }
}

Gnome::Gda::Value ComboGlom::get_value() const
{
   //Get the active row:
   Gtk::TreeModel::iterator iter = get_active();

   if(iter)
   {
     const Gtk::TreeModel::Row row = *iter;
     Gnome::Gda::Value value;
     row.get_value(0, value);
     return value;
  }

  return Gnome::Gda::Value();
}

#ifndef GLOM_ENABLE_CLIENT_ONLY
bool ComboGlom::on_button_press_event(GdkEventButton *event)
{
g_warning("ComboGlom::on_button_press_event()");

  //Enable/Disable items.
  //We did this earlier, but get_application is more likely to work now:
  Application* pApp = get_application();
  if(pApp)
  {
    pApp->add_developer_action(m_refContextLayout); //So that it can be disabled when not in developer mode.
    pApp->add_developer_action(m_refContextAddField);
    pApp->add_developer_action(m_refContextAddRelatedRecords);
    pApp->add_developer_action(m_refContextAddGroup);

    pApp->update_userlevel_ui(); //Update our action's sensitivity.

    //Only show this popup in developer mode, so operators still see the default GtkEntry context menu.
    //TODO: It would be better to add it somehow to the standard context menu.
    if(pApp->get_userlevel() == AppState::USERLEVEL_DEVELOPER)
    {
      GdkModifierType mods;
      gdk_window_get_pointer( gtk_widget_get_window (Gtk::Widget::gobj()), 0, 0, &mods );
      if(mods & GDK_BUTTON3_MASK)
      {
        //Give user choices of actions on this item:
        m_pMenuPopup->popup(event->button, event->time);
        return true; //We handled this event.
      }
    }

  }

  return Gtk::ComboBox::on_button_press_event(event);
}
#endif // !GLOM_ENABLE_CLIENT_ONLY

Application* ComboGlom::get_application()
{
  Gtk::Container* pWindow = get_toplevel();
  //TODO: This only works when the child widget is already in its parent.

  return dynamic_cast<Application*>(pWindow);
}


void ComboGlom::on_changed()
{
  //Call base class:
  Gtk::ComboBox::on_changed();

  //This signal is emitted for every key press, but sometimes it's just to say that the active item has changed to "no active item",
  //if the text is not in the dropdown list:
  Gtk::TreeModel::iterator iter = get_active();

  if(iter)
  {
    //This is either a choice from the dropdown menu, or someone has typed in something that is in the drop-down menu.
    //TODO: If both ab, and abc, are in the menu, we are responding twice if the user types abc.
    check_for_change();
  }
  //Entry of text that is not in the menu will be handled by the ->get_entry() signal handlers._
}

void ComboGlom::set_read_only(bool /* read_only */)
{
  //TODO
}

} //namespace DataWidetChildren
} //namespace Glom
