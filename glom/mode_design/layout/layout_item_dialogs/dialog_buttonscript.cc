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


#include "dialog_buttonscript.h"
#include <glom/python_embed/glom_python.h>
#include <glom/utils_ui.h>
#include <libglom/data_structure/glomconversions.h>
#include <gtksourceviewmm/sourcelanguagemanager.h>


//#include <libgnome/gnome-i18n.h>
#include <glibmm/i18n.h>

namespace Glom
{

const char* Dialog_ButtonScript::glade_id("window_button_script");
const bool Dialog_ButtonScript::glade_developer(true);

Dialog_ButtonScript::Dialog_ButtonScript(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
: Gtk::Dialog(cobject)
{
  builder->get_widget("textview_calculation",  m_text_view_script);
  builder->get_widget("button_test",  m_button_test_script);
  builder->get_widget("entry_title",  m_entry_title);

  m_button_test_script->signal_clicked().connect( sigc::mem_fun(*this, &Dialog_ButtonScript::on_button_test_script) );

  // Set a monospace font
  m_text_view_script->modify_font(Pango::FontDescription("Monospace"));
  //on_foreach_connect(*this);

  //Dialog_Properties::set_modified(false);

  //Tell the SourceView to do syntax highlighting for Python:
  Glib::RefPtr<gtksourceview::SourceLanguageManager> languages_manager = 
    gtksourceview::SourceLanguageManager::get_default();

  Glib::RefPtr<gtksourceview::SourceLanguage> language = 
    languages_manager->get_language("python"); //This is the GtkSourceView language ID.
  if(language)
  {
     //Create a new buffer and set it, instead of getting the default buffer, in case libglade has tried to set it, using the wrong buffer type:
     Glib::RefPtr<gtksourceview::SourceBuffer> buffer = gtksourceview::SourceBuffer::create(language);
     buffer->set_highlight_syntax();
     m_text_view_script->set_buffer(buffer);
  }

  show_all_children();
}

Dialog_ButtonScript::~Dialog_ButtonScript()
{
}

void Dialog_ButtonScript::set_script(const sharedptr<const LayoutItem_Button>& script, const Glib::ustring& table_name)
{
  //set_blocked();

  m_script = glom_sharedptr_clone(script); //Remember it so we save any details that are not in our UI.
  m_table_name = table_name;  //Used for lookup combo boxes.

  m_text_view_script->get_buffer()->set_text( script->get_script() );

  m_entry_title->set_text(script->get_title());
  //set_blocked(false);

  //Dialog_Properties::set_modified(false);
}

sharedptr<LayoutItem_Button> Dialog_ButtonScript::get_script() const
{
  sharedptr<LayoutItem_Button> result = glom_sharedptr_clone(m_script); //Start with the old details, to preserve anything that is not in our UI.

  get_script(result);

  return result;
}

void Dialog_ButtonScript::get_script(const sharedptr<LayoutItem_Button>& script) const
{
  script->set_script(m_text_view_script->get_buffer()->get_text() );
  script->set_title(m_entry_title->get_text());
}

void Dialog_ButtonScript::on_button_test_script()
{
  const Glib::ustring calculation = m_text_view_script->get_buffer()->get_text();

  type_map_fields field_values;

  Document* document = get_document();
  if(document)
  {
    const Document::type_vec_fields fields = document->get_table_fields(m_table_name);
    for(Document::type_vec_fields::const_iterator iter = fields.begin(); iter != fields.end(); ++iter)
    {
      const sharedptr<const Field> field = *iter;
      const Gnome::Gda::Value example_value = Conversions::get_example_value(field->get_glom_type());
      field_values[field->get_name()] = example_value;
    }
  }

  //We need the connection when we run the script, so that the script may use it.
  sharedptr<SharedConnection> sharedconnection = connect_to_server(this /* parent window */);

  Glib::ustring error_message;
  PythonUICallbacks callbacks;
  glom_execute_python_function_implementation(calculation,
    field_values, //TODO: Maybe use the field's type here.
    document,
    m_table_name,
    sharedptr<Field>(), Gnome::Gda::Value(), // primary key - only used when setting values in the DB, which we would not encourage in a test.
    sharedconnection->get_gda_connection(),
    callbacks,
    error_message);
    
  if(!error_message.empty())
    Utils::show_ok_dialog(_("Calculation failed"), _("The calculation failed with this error:\n") + error_message, *this, Gtk::MESSAGE_ERROR);
}

} //namespace Glom
