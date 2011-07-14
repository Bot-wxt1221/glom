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

#ifndef GLOM_UTILITY_WIDGETS_IMAGE_GLOM_H
#define GLOM_UTILITY_WIDGETS_IMAGE_GLOM_H

#include <gtkmm.h>
#include <libglom/data_structure/field.h>
#include "layoutwidgetfield.h"
#include <gtkmm/builder.h>
#include <evince-view.h>

namespace Glom
{

class Application;

class ImageGlom
: public Gtk::EventBox,
  public LayoutWidgetField
{
public:
  ImageGlom();
  explicit ImageGlom(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);


  virtual ~ImageGlom();
  
  virtual void set_layout_item(const sharedptr<LayoutItem>& layout_item, const Glib::ustring& table_name);

  virtual void set_value(const Gnome::Gda::Value& value);
  virtual Gnome::Gda::Value get_value() const;
  virtual bool get_has_original_data() const;

  //Optionally use this instead of set_value(), to avoid creating an unnecessary Value.
  //void set_pixbuf(const Glib::RefPtr<Gdk::Pixbuf>& pixbuf);

  void do_choose_image();

  void set_read_only(bool read_only = true);

  void on_ev_job_finished(EvJob* job);
  
private:
  void init();

  virtual void on_size_allocate(Gtk::Allocation& allocation);

  virtual bool on_button_press_event(GdkEventButton *event);

  void on_menupopup_activate_open_file();
  void on_menupopup_activate_open_file_with();
  void on_menupopup_activate_save_file();
  void on_menupopup_activate_select_file();
  void on_menupopup_activate_copy();
  void on_menupopup_activate_paste();
  void on_menupopup_activate_clear();

  void on_clipboard_get(Gtk::SelectionData& selection_data, guint /* info */);
  void on_clipboard_clear();
  void on_clipboard_received_image(const Glib::RefPtr<Gdk::Pixbuf>& pixbuf);

  virtual Application* get_application();

  void setup_menu_usermode();
  void show_image_data();
  
  //Get a pixbuf scaled down to the current size allocation:
  Glib::RefPtr<Gdk::Pixbuf> get_scaled_image();
  
  Glib::ustring save_to_temp_file(bool show_progress = true);
  bool save_file(const Glib::ustring& uri);
  bool save_file_sync(const Glib::ustring& uri);
  void open_with(const Glib::RefPtr<Gio::AppInfo>& app_info =  Glib::RefPtr<Gio::AppInfo>());

  Glib::ustring get_mime_type() const;
  static void fill_evince_supported_mime_types();
  static void fill_gdkpixbuf_supported_mime_types();
 
  mutable Gnome::Gda::Value m_original_data; // Original file data (mutable so that we can create it in get_value() if it does not exist yet)

  Gtk::Frame m_frame;
  
  //For anything supported by Evince:
  EvView* m_ev_view;
  EvDocumentModel* m_ev_document_model;
  
  //For anything supported by GdkPixbuf:
  Gtk::Image m_image;
  Glib::RefPtr<Gdk::Pixbuf> m_pixbuf_original; //Only stored temporarily, because it could be big.
  Glib::RefPtr<Gdk::Pixbuf> m_pixbuf_clipboard; //When copy is used, store it here until it is pasted.
  
  //For anything else:
  Glib::RefPtr<Gdk::Pixbuf> m_pixbuf_thumbnail;

  Gtk::Menu* m_pMenuPopup_UserMode;
  Glib::RefPtr<Gtk::ActionGroup> m_refActionGroup_UserModePopup;
  Glib::RefPtr<Gtk::UIManager> m_refUIManager_UserModePopup;
  Glib::RefPtr<Gtk::Action> m_refActionOpenFile, m_refActionOpenFileWith, 
    m_refActionSaveFile, m_refActionSelectFile, m_refActionCopy, m_refActionPaste, m_refActionClear;

  bool m_read_only;
  
  typedef std::vector<Glib::ustring> type_vec_ustrings;
  static type_vec_ustrings m_evince_supported_mime_types;
  static type_vec_ustrings m_gdkpixbuf_supported_mime_types;
};

} //namespace Glom

#endif //GLOM_UTILITY_WIDGETS_COMBOENTRY_GLOM_H

