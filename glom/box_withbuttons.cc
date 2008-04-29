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

#include "box_withbuttons.h"
#include "application.h" //App_Glom.
#include <glom/libglom/appstate.h>
//#include <libgnomeui/gnome-app-helper.h>

#include <sstream> //For stringstream

namespace Glom
{

Box_WithButtons::Box_WithButtons()
: m_Box_Buttons(false, Utils::DEFAULT_SPACING_SMALL),
  m_Button_Cancel(Gtk::Stock::CANCEL)
{
  //m_pDocument = 0;

  set_border_width(Utils::DEFAULT_SPACING_SMALL);
  set_spacing(Utils::DEFAULT_SPACING_SMALL);

  //Connect signals:
  m_Button_Cancel.signal_clicked().connect(sigc::mem_fun(*this, &Box_WithButtons::on_Button_Cancel));
}

Box_WithButtons::Box_WithButtons(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& /* refGlade */)
: Gtk::VBox(cobject),
  m_Box_Buttons(false, Utils::DEFAULT_SPACING_SMALL),
  m_Button_Cancel(Gtk::Stock::CANCEL)
{
  //m_pDocument = 0;

  set_border_width(Utils::DEFAULT_SPACING_SMALL);
  set_spacing(Utils::DEFAULT_SPACING_SMALL);

  //Connect signals:
  m_Button_Cancel.signal_clicked().connect(sigc::mem_fun(*this, &Box_WithButtons::on_Button_Cancel));
}

Box_WithButtons::Box_WithButtons(BaseObjectType* cobject)
: Gtk::VBox(cobject),
  m_Box_Buttons(false, Utils::DEFAULT_SPACING_SMALL),
  m_Button_Cancel(Gtk::Stock::CANCEL)
{
}

Box_WithButtons::~Box_WithButtons()
{
}

void Box_WithButtons::on_Button_Cancel()
{
  //Tell the parent dialog that the user has clicked [Cancel]:
  signal_cancelled.emit();
}

const Gtk::Window* Box_WithButtons::get_app_window() const
{
  Box_WithButtons* nonconst = const_cast<Box_WithButtons*>(this);
  return nonconst->get_app_window();
}
  
Gtk::Window* Box_WithButtons::get_app_window()
{
  return dynamic_cast<Gtk::Window*>(get_toplevel());
/*

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
*/
}

/*
void Box_WithButtons::show_hint()
{
  hint_set(m_strHint);
}
*/

void Box_WithButtons::set_button_cancel(Gtk::Button& button)
{
  button.signal_clicked().connect(sigc::mem_fun(*this, &Box_WithButtons::on_Button_Cancel));
}

Gtk::Widget* Box_WithButtons::get_default_button()
{
  return 0; //Override this if the box has a default button.
}


} //namespace Glom

