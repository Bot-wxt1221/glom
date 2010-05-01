/* Glom
 *
 * Copyright (C) 2001-2009 Openismus GmbH
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

#ifndef GLOM_UTILS_UI_H
#define GLOM_UTILS_UI_H

#include "config.h"
#include <libglom/data_structure/field.h>
#include <libglom/data_structure/numeric_format.h>

#include <libglom/data_structure/layout/layoutitem_field.h>

#include <gtkmm/dialog.h>
#include <gtkmm/messagedialog.h>

#ifdef GLOM_ENABLE_MAEMO
#include <hildon/hildon-defines.h>
#endif

namespace Glom
{

//TODO: Rename this to UiUtils
namespace Utils
{

enum DefaultSpacings
{
  #ifdef GLOM_ENABLE_MAEMO
  //We use different spacings on Maemo because the screen is smaller:
  DEFAULT_SPACING_LARGE = HILDON_MARGIN_DEFAULT,
  DEFAULT_SPACING_SMALL = HILDON_MARGIN_HALF
  #else
  DEFAULT_SPACING_LARGE = 12,
  DEFAULT_SPACING_SMALL = 6
  #endif //GLOM_ENABLE_MAEMO
};

/**
 * Show the dialog, blocking until there is a non-help response,
 * showing the appropriate help page if the help button is clicked.
 */ 
int dialog_run_with_help(Gtk::Dialog* dialog, const Glib::ustring& id = Glib::ustring());

/**
 * Show the dialog, blocking until there is a non-help response,
 * showing the appropriate help page if the help button is clicked.
 *  This requires the dialog class to have a static 
 * glade_id member variable, which we reuse as the help ID.
 */ 
template<class T_Dialog>
int dialog_run_with_help(T_Dialog* dialog)
{
  return dialog_run_with_help(dialog, T_Dialog::glade_id);
}

// Maemo has no help system since Maemo 5 (Fremantle).
#ifndef GLOM_ENABLE_MAEMO
/** This is a replacement for gnome_help_display(), 
 * to avoid the libgnome dependency.
 * TODO: GTK+ should have a function for this soon.
 */
void show_help(const Glib::ustring& id = Glib::ustring());
#endif //GLOM_ENABLE_MAEMO

void show_ok_dialog(const Glib::ustring& title, const Glib::ustring& message, Gtk::Window& parent, Gtk::MessageType message_type);
void show_ok_dialog(const Glib::ustring& title, const Glib::ustring& message, Gtk::Window* parent, Gtk::MessageType message_type);

void show_window_until_hide(Gtk::Window* window);

/// For instance, to create bold primary text for a dialog box, without marking the markup for translation.
Glib::ustring bold_message(const Glib::ustring& message);

Glib::RefPtr<Gdk::Pixbuf> get_pixbuf_for_gda_value(const Gnome::Gda::Value& value);

/** Get the width required for typical data of this type in the current font.
 * 
 * @widget The widget whose font should be used.
 * @field_layout The layout item whose data type should be used.
 * @or_title If true, check the width of the item's title too, returning the larger of the two values.
 * @result The width in pixels.
 */
int get_suitable_field_width_for_widget(Gtk::Widget& widget, const sharedptr<const LayoutItem_Field>& field_layout, bool or_title = false);

/// Add the @a extension if no extension is there already:
std::string get_filepath_with_extension(const std::string& filepath, const std::string& extension);

Glib::RefPtr<Gdk::Pixbuf> image_scale_keeping_ratio(const Glib::RefPtr<Gdk::Pixbuf>& pixbuf, int target_height, int target_width);

///@result Whether the user would like to find again.
bool show_warning_no_records_found(Gtk::Window& transient_for);

  
} //namespace Utils

} //namespace Glom

#endif //GLOM_UTILS_UI_H

