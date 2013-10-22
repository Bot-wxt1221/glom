/* Glom
 *
 * Copyright (C) 2011 Murray Cumming
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 */

#include "notebook_noframe.h"
#include <glom/utils_ui.h>
#include <gtkmm/label.h>
#include <gtkmm/togglebutton.h>
#include <glibmm/i18n.h>
//#include <sstream> //For stringstream

namespace Glom
{

NotebookNoFrame::NotebookNoFrame()
{
  set_orientation(Gtk::ORIENTATION_VERTICAL);
  set_spacing(Utils::DEFAULT_SPACING_SMALL);

  m_box_top.set_orientation(Gtk::ORIENTATION_HORIZONTAL);
  m_box_top.set_spacing(Utils::DEFAULT_SPACING_SMALL);
  m_box_top.pack_start(m_box_action_left, Gtk::PACK_SHRINK);
  m_box_top.pack_start(m_box_tabs);
  m_box_top.pack_end(m_box_action_right, Gtk::PACK_SHRINK);
  pack_start(m_box_top, Gtk::PACK_SHRINK);

  m_box_top.show();

  //Let the StackSwitcher switch the Stack:
  m_box_tabs.set_stack(m_box_pages);
  m_box_pages.property_visible_child().signal_changed().connect(
    sigc::mem_fun(*this, &NotebookNoFrame::on_visible_child_changed));

  //m_box_tabs.set_spacing(Utils::DEFAULT_SPACING_SMALL);

  m_box_tabs.show();

  pack_start(m_box_pages);
  m_box_pages.show();
}

NotebookNoFrame::~NotebookNoFrame()
{
}

NotebookNoFrame::type_signal_switch_page NotebookNoFrame::signal_switch_page()
{
  return m_signal_switch_page;
}

Glib::ustring NotebookNoFrame::get_visible_child_name() const
{
  return m_box_pages.get_visible_child_name();
}

Gtk::Widget* NotebookNoFrame::get_visible_child()
{
  return m_box_pages.get_visible_child();
}

void NotebookNoFrame::set_visible_child(const Glib::ustring& name)
{
  m_box_pages.set_visible_child(name);
}

void NotebookNoFrame::append_page(Widget& child, const Glib::ustring& name, const Glib::ustring& tab_label)
{
  m_box_pages.add(child, name, tab_label);
}

std::vector<Gtk::Widget*> NotebookNoFrame::get_page_children()
{
  return m_box_pages.get_children();
}

void NotebookNoFrame::on_visible_child_changed()
{
  Gtk::Widget* widget = get_visible_child();
  m_signal_switch_page.emit(widget);
}

void NotebookNoFrame::set_action_widget(Gtk::Widget* widget, Gtk::PackType pack_type)
{
  if(pack_type == Gtk::PACK_START)
  {
    m_box_action_left.pack_start(*widget, Gtk::PACK_SHRINK);
    m_box_action_left.show();
  }
  else
  {
    m_box_action_right.pack_end(*widget, Gtk::PACK_SHRINK);
    m_box_action_right.show();
  }
}


} //namespace Glom
