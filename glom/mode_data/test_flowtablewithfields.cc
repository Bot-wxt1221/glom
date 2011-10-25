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

#include <gtkmm/window.h>
#include <glom/mode_data/flowtablewithfields.h>
#include <iostream>


//#include "dragwindow.h"

/*
void on_drag_data_get_label(const Glib::RefPtr<Gdk::DragContext>&, Gtk::SelectionData& selection_data, guint, guint)
{
  selection_data.set(selection_data.get_target(), 8, (const guchar*)"label", 5);
}

void on_drag_data_get_entry(const Glib::RefPtr<Gdk::DragContext>&, Gtk::SelectionData& selection_data, guint, guint)
{
  selection_data.set(selection_data.get_target(), 8, (const guchar*)"entry", 5);
}
*/

static void fill_flowtable(Glom::FlowTableWithFields& flowtable)
{
  {
    Glom::sharedptr<Glom::LayoutItem_Text> item =
      Glom::sharedptr<Glom::LayoutItem_Text>::create();
    item->set_text("test static text 1");
    flowtable.add_layout_item(item);
  }

  {
    Glom::sharedptr<Glom::LayoutItem_Text> item =
      Glom::sharedptr<Glom::LayoutItem_Text>::create();
    item->set_text("test static text 2");
    item->set_title("title for text 2");
    flowtable.add_layout_item(item);
  }

  {
    Glom::sharedptr<Glom::LayoutItem_Image> item =
      Glom::sharedptr<Glom::LayoutItem_Image>::create();
    //item->set_image(somevalue);
    item->set_title("title for image");
    flowtable.add_layout_item(item);
  }
  
  Glom::sharedptr<Glom::LayoutGroup> group = 
    Glom::sharedptr<Glom::LayoutGroup>::create();
  Glom::sharedptr<Glom::LayoutItem_Text> item =
    Glom::sharedptr<Glom::LayoutItem_Text>::create();
  item->set_text("inner text 1");
  group->add_item(item);
  item =
    Glom::sharedptr<Glom::LayoutItem_Text>::create();
  item->set_text("inner text 2");
  group->add_item(item);
  flowtable.add_layout_item(group);
}

static void clear_flowtable(Glom::FlowTableWithFields& flowtable)
{
  flowtable.remove_all();
}

int
main(int argc, char* argv[])
{
  Gtk::Main mainInstance(argc, argv);

  Gtk::Window window;
  //Gtk::Box flowtable;
  Glom::FlowTableWithFields flowtable;
  flowtable.set_lines(2);
  flowtable.set_horizontal_spacing(6);
  flowtable.set_vertical_spacing(6);

  fill_flowtable(flowtable);
  clear_flowtable(flowtable);
  fill_flowtable(flowtable);

  window.add(flowtable);
  flowtable.set_design_mode();
  flowtable.show();
  
  flowtable.set_enable_drag_and_drop(true);
  //flowtable.set_drag_enabled(EGG_DRAG_FULL);
  //flowtable.set_drop_enabled(true);

  Gtk::Main::run(window);

  return 0;
}
