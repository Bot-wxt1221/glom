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

#include "filechooser_export.h"
#include <glom/mode_design/layout/dialog_layout_export.h>
#include <glom/utils_ui.h>
#include <glom/glade_utils.h>
#include <gtkmm/stock.h>
#include <glibmm/i18n.h>

namespace Glom
{

FileChooser_Export::FileChooser_Export()
: Gtk::FileChooserDialog(_("Export to File"), Gtk::FILE_CHOOSER_ACTION_SAVE),
  m_extra_widget(false, Utils::DEFAULT_SPACING_SMALL),
#ifndef GLOM_ENABLE_CLIENT_ONLY
  m_button_format(_("Define Data _Format"), true /* use mnenomic */),
  m_pDialogLayout(0),
#endif //GLOM_ENABLE_CLIENT_ONLY
  m_document(0)
{
  set_icon_name("glom");

  add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  add_button(_("_Export"), Gtk::RESPONSE_OK);

  m_extra_widget.pack_start(m_button_format, Gtk::PACK_SHRINK);

#ifndef GLOM_ENABLE_CLIENT_ONLY
  m_button_format.signal_clicked().connect(
    sigc::mem_fun(*this, &FileChooser_Export::on_button_define_layout) );
  m_button_format.show();
#endif

  set_extra_widget(m_extra_widget);
  m_extra_widget.show();

#ifndef GLOM_ENABLE_CLIENT_ONLY
  //TODO: Use a generic layout dialog?
#ifdef GLIBMM_EXCEPTIONS_ENABLED  
  Glib::RefPtr<Gtk::Builder> refXml = Gtk::Builder::create_from_file(Utils::get_glade_file_path("glom_developer.glade"), "window_data_layout_export");
#else
  std::auto_ptr<Glib::Error> error;  
  Glib::RefPtr<Gtk::Builder> refXml = Gtk::Builder::create_from_file(Utils::get_glade_file_path("glom_developer.glade"), "window_data_layout_export", error);
#endif

  if(refXml)
  {
    Dialog_Layout_Export* dialog = 0;
    refXml->get_widget_derived("window_data_layout_export", dialog);
    if(dialog)
    {
      m_pDialogLayout = dialog;
      m_pDialogLayout->set_icon_name("glom");
      //add_view(m_pDialogLayout); //Give it access to the document.
      m_pDialogLayout->signal_hide().connect( sigc::mem_fun(*this, &FileChooser_Export::on_dialog_layout_hide) );
    }
  }
#endif //GLOM_ENABLE_CLIENT_ONLY
}

FileChooser_Export::~FileChooser_Export()
{
#ifndef GLOM_ENABLE_CLIENT_ONLY
  delete m_pDialogLayout;
  m_pDialogLayout = 0;
#endif //GLOM_ENABLE_CLIENT_ONLY
}

void FileChooser_Export::set_export_layout(const Document::type_list_layout_groups& layout_groups, const Glib::ustring& table_name, Document* document)
{
  m_layout_groups = layout_groups;
  m_table_name = table_name;
  m_document = document;
  if(!m_document)
    std::cerr << "FileChooser_Export::set_export_layout() document is NULL." << std::endl;
}

//We only allow a full export in client-only mode, 
//to avoid building a large part of the layout definition code.
#ifndef GLOM_ENABLE_CLIENT_ONLY
void FileChooser_Export::on_button_define_layout()
{
  if(!m_pDialogLayout)
    return;

  m_pDialogLayout->set_layout_groups(m_layout_groups, m_document, m_table_name); //TODO: Use m_TableFields?
  m_pDialogLayout->set_transient_for(*this);
  set_modal(false);
  m_pDialogLayout->set_modal();
  m_pDialogLayout->show();
}

void FileChooser_Export::on_dialog_layout_hide()
{
  if(m_pDialogLayout)
    m_pDialogLayout->get_layout_groups(m_layout_groups);
}
#endif //GLOM_ENABLE_CLIENT_ONLY

void FileChooser_Export::get_layout_groups(Document::type_list_layout_groups& layout_groups) const
{
  layout_groups = m_layout_groups;
}

} //namespace Glom


