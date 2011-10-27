/* main.cc
 *
 * Copyright (C) 2010 The Glom development team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <gtkmm/builder.h>
#include <gtkmm/dialog.h>
#include <gtkmm/main.h>
#include <gtksourceviewmm/init.h>
#include <libxml++/libxml++.h>

#include <iostream>

static bool attempt_instantiation(const std::string& filepath, const xmlpp::Element* child)
{
  const Glib::ustring id = child->get_attribute_value("id");
  const Glib::ustring gclassname = child->get_attribute_value("class");

  // Try to instantiate the object:
  Glib::RefPtr<Gtk::Builder> builder;
  try
  {
    builder = Gtk::Builder::create_from_file(filepath, id);
  }
  catch(const Glib::Error& ex)
  {
    std::cerr << "Exception from Gtk::Builder::create_from_file() with id=" << id << " from file " << filepath << std::endl;
    std::cerr << "  Error: " << ex.what() << std::endl;
    return -1;
  }

  // Try to get the widget, checking that it has the correct type:
  Gtk::Widget* widget = 0;
  if(gclassname == "GtkWindow")
  {
    Gtk::Window* window = 0;
    builder->get_widget(id, window);
    widget = window;
  }
  else if(gclassname == "GtkDialog")
  {
    Gtk::Dialog* dialog = 0;
    builder->get_widget(id, dialog);
    widget = dialog;
  }
  else
  {
    //We try to avoid using non-window top-level widgets in Glom.
    std::cerr << "Non-window top-level object in Glade file (unexpected by Glom): id=" << id << " from file " << filepath << std::endl;

    //But let's try this anyway:
    Glib::RefPtr<Glib::Object> object = builder->get_object(id);

    return false;
  }

  if(!widget)
  {
    std::cerr << "Failed to instantiate object with id=" << id << " from file " << filepath << std::endl;
    return false;
  }

  //Check that it is not visible by default,
  //because applications generally want to separate instantiation from showing.
  if(widget->get_visible())
  {
     std::cerr << "Top-level window is visible by default (unwanted by Glom): id=" << id << " from file " << filepath << std::endl;
     return false;
  }

  //std::cout << "Successful instantiation of object with id=" << id << " from file " << filepath << std::endl;

  delete widget;
  return true;
}

int main(int argc, char* argv[])
{
  Gtk::Main kit(argc, argv);
  gtksourceview::init(); //Our .glade files contain gtksourceview widgets too.

  std::string filepath;
  if(argc > 1 )
    filepath = argv[1]; //Allow the user to specify a different XML file to parse.
  else
  {
    std::cerr << "Usage: glade_toplevels_instantiation filepath" << std::endl;
    return -1;
  }

  #ifdef LIBXMLCPP_EXCEPTIONS_ENABLED
  try
  {
  #endif //LIBXMLCPP_EXCEPTIONS_ENABLED
    xmlpp::DomParser parser;
    //parser.set_validate();
    parser.set_substitute_entities(); //We just want the text to be resolved/unescaped automatically.
    parser.parse_file(filepath);
    if(!parser)
      return -1;

    const xmlpp::Node* root = parser.get_document()->get_root_node(); //deleted by DomParser.
    if(!root)
      return -1;

    const xmlpp::Node::NodeList children = root->get_children("object");
    for(xmlpp::Node::NodeList::const_iterator iter = children.begin(); iter != children.end(); ++iter)
    {
       const xmlpp::Element* child = dynamic_cast<const xmlpp::Element*>(*iter);

       //Try to instante the object with Gtk::Builder:
       if(child && !attempt_instantiation(filepath, child))
         return -1;
    }

  #ifdef LIBXMLCPP_EXCEPTIONS_ENABLED
  }
  catch(const std::exception& ex)
  {
    std::cout << "Exception caught: " << ex.what() << std::endl;
  }
  #endif //LIBXMLCPP_EXCEPTIONS_ENABLED

  return 0;
}
