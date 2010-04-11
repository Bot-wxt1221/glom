/* Glom
 *
 * Copyright (C) 2008 Openismus GmbH
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

#include "checkbutton.h"
#include <gtkmm/messagedialog.h>
#include <glom/dialog_invalid_data.h>
#include <glom/application.h>
#include <glibmm/i18n.h>

#include <iostream>   // for cout, endl

namespace Glom
{

namespace DataWidgetChildren
{

CheckButton::CheckButton(const Glib::ustring& title)
  : Gtk::CheckButton(title)
{
#ifndef GLOM_ENABLE_CLIENT_ONLY
  setup_menu();
#endif // !GLOM_ENABLE_CLIENT_ONLY

  init();
}

CheckButton::~CheckButton()
{
}

void CheckButton::init()
{

}

#ifndef GLOM_ENABLE_CLIENT_ONLY
bool CheckButton::on_button_press_event(GdkEventButton *event)
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

    //Only show this popup in developer mode, so operators still see the default GtkCheckButton context menu.
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
  return Gtk::CheckButton::on_button_press_event(event);
}
#endif // !GLOM_ENABLE_CLIENT_ONLY

Application* CheckButton::get_application()
{
  Gtk::Container* pWindow = get_toplevel();
  //TODO: This only works when the child widget is already in its parent.

  return dynamic_cast<Application*>(pWindow);
}

void CheckButton::set_value(const Gnome::Gda::Value& value)
{
  set_active (value.get_boolean());
}

Gnome::Gda::Value CheckButton::get_value() const
{
  return Gnome::Gda::Value(get_active());
}

} //namespace DataWidetChildren
} //namespace Glom
