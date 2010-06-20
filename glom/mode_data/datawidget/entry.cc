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

#include "entry.h"
#include <libglom/data_structure/glomconversions.h>
#include <gtkmm/messagedialog.h>
#include <glom/dialog_invalid_data.h>
#include <libglom/data_structure/glomconversions.h>
#include <glom/application.h>
#include <glibmm/i18n.h>
//#include <sstream> //For stringstream

#include <locale>     // for locale, time_put
#include <ctime>     // for struct tm
#include <iostream>   // for cout, endl

namespace Glom
{

namespace DataWidgetChildren
{

Entry::Entry(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& /* builder */)
:
#ifdef GLOM_ENABLE_MAEMO
  Hildon::Entry(cobject),
#else
  Gtk::Entry(cobject),
#endif
  m_glom_type(Field::TYPE_TEXT)
{
#ifndef GLOM_ENABLE_CLIENT_ONLY
  setup_menu();
#endif // !GLOM_ENABLE_CLIENT_ONLY
  init();
}

Entry::Entry(Field::glom_field_type glom_type)
:
#ifdef GLOM_ENABLE_MAEMO
  Hildon::Entry(Gtk::Hildon::SIZE_AUTO),
#endif
 m_glom_type(glom_type)
{
#ifndef GLOM_ENABLE_CLIENT_ONLY
  setup_menu();
#endif // !GLOM_ENABLE_CLIENT_ONLY

  init();
}

Entry::~Entry()
{
}

void Entry::init()
{
}

void Entry::set_layout_item(const sharedptr<LayoutItem>& layout_item, const Glib::ustring& table_name)
{
  LayoutWidgetField::set_layout_item(layout_item, table_name);
#ifdef GTKMM_ATKMM_ENABLED
  get_accessible()->set_name(layout_item->get_name());
#endif

  //Horizontal Alignment:
  FieldFormatting::HorizontalAlignment alignment = 
    FieldFormatting::HORIZONTAL_ALIGNMENT_LEFT;
  sharedptr<LayoutItem_Field> layout_field =
    sharedptr<LayoutItem_Field>::cast_dynamic(get_layout_item());
  if(layout_field)
    alignment = layout_field->get_formatting_used_horizontal_alignment();

  const float x_align = (alignment == FieldFormatting::HORIZONTAL_ALIGNMENT_LEFT ? 0.0 : 1.0);
  set_alignment(x_align);
}

void Entry::set_glom_type(Field::glom_field_type glom_type)
{
  m_glom_type = glom_type;
}

void Entry::check_for_change()
{
  Glib::ustring new_text = get_text();
  if(new_text != m_old_text)
  {
    //Validate the input:
    bool success = false;

    sharedptr<const LayoutItem_Field> layout_item = sharedptr<const LayoutItem_Field>::cast_dynamic(get_layout_item());
    Gnome::Gda::Value value = Conversions::parse_value(m_glom_type, get_text(), layout_item->get_formatting_used().m_numeric_format, success);

    if(success)
    {
      //Actually show the canonical text:
      set_value(value);
      m_signal_edited.emit(); //The text was edited, so tell the client code.
    }
    else
    {
      //Tell the user and offer to revert or try again:
      bool revert = glom_show_dialog_invalid_data(m_glom_type);
      if(revert)
      {
        set_text(m_old_text);
      }
      else
        grab_focus(); //Force the user back into the same field, so that the field can be checked again and eventually corrected or reverted.
    }
  }
}

bool Entry::on_focus_out_event(GdkEventFocus* event)
{
  const bool result = Gtk::Entry::on_focus_out_event(event);

  //The user has finished editing.
  check_for_change();

  //Call base class:
  return result;
}

void Entry::on_activate()
{
  //Call base class:
  Gtk::Entry::on_activate();

  //The user has finished editing.
  check_for_change();
}

void Entry::on_changed()
{
  //The text is being edited, but the user has not finished yet.

  //Call base class:
  Gtk::Entry::on_changed();
}

void Entry::set_value(const Gnome::Gda::Value& value)
{
  sharedptr<const LayoutItem_Field> layout_item = sharedptr<LayoutItem_Field>::cast_dynamic(get_layout_item());
  if(!layout_item)
    return;

  const Glib::ustring text = Conversions::get_text_for_gda_value(m_glom_type, value, layout_item->get_formatting_used().m_numeric_format);
  set_text(text);

  //Show a different color if the value is numeric, if that's specified:
  if(layout_item->get_glom_type() == Field::TYPE_NUMERIC)
  {
    const Glib::ustring fg_color = 
    layout_item->get_formatting_used().get_text_format_color_foreground_to_use(value);
    if(!fg_color.empty())
      modify_text(Gtk::STATE_NORMAL, Gdk::Color(fg_color));
    else
      modify_text(Gtk::STATE_NORMAL, Gdk::Color());
  }
}

void Entry::set_text(const Glib::ustring& text)
{
  m_old_text = text;

  //Call base class:
  Gtk::Entry::set_text(text);
}

Gnome::Gda::Value Entry::get_value() const
{
  bool success = false;

  sharedptr<const LayoutItem_Field> layout_item = sharedptr<const LayoutItem_Field>::cast_dynamic(get_layout_item());
  return Conversions::parse_value(m_glom_type, get_text(), layout_item->get_formatting_used().m_numeric_format, success);
}

#ifndef GLOM_ENABLE_CLIENT_ONLY
bool Entry::on_button_press_event(GdkEventButton *event)
{
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

  return Gtk::Entry::on_button_press_event(event);
}
#endif // !GLOM_ENABLE_CLIENT_ONLY

Application* Entry::get_application()
{
  Gtk::Container* pWindow = get_toplevel();
  //TODO: This only works when the child widget is already in its parent.

  return dynamic_cast<Application*>(pWindow);
}

void Entry::set_read_only(bool read_only)
{
  set_editable(!read_only);
}

} //namespace DataWidetChildren
} //namespace Glom
