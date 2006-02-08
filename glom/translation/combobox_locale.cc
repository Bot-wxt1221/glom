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

#include "combobox_locale.h"
#include <gtk/gtkcomboboxentry.h>
#include "../data_structure/iso_codes.h"

ComboBox_Locale::ComboBox_Locale(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& /* refGlade */)
: Gtk::ComboBox(cobject)
{
  m_model = Gtk::ListStore::create(m_model_columns);

  //Fill the model:
  const IsoCodes::type_list_locales list_locales = IsoCodes::get_list_of_locales();
  for(IsoCodes::type_list_locales::const_iterator iter = list_locales.begin(); iter != list_locales.end(); ++iter)
  {
    Gtk::TreeModel::iterator tree_iter = m_model->append();
    Gtk::TreeModel::Row row = *tree_iter;

    const IsoCodes::Locale& the_locale = *iter;
    row[m_model_columns.m_identifier] = the_locale.m_identifier;
    row[m_model_columns.m_name] = the_locale.m_name;
  }

  m_model->set_sort_column(m_model_columns.m_name, Gtk::SORT_ASCENDING);

  set_model(m_model);

  //Do not show the non-human-readable ID: pack_start(m_model_columns.m_identifier);

  //Show this too.
  //Create the cell renderer manually, so we can specify the alignment:
  Gtk::CellRendererText* cell = Gtk::manage(new Gtk::CellRendererText());
  cell->property_xalign() = 0.0f;
  pack_start(*cell);
  add_attribute(cell->property_text(), m_model_columns.m_name);
}


ComboBox_Locale::~ComboBox_Locale()
{

}

Glib::ustring ComboBox_Locale::get_selected_locale() const
{
  Gtk::TreeModel::iterator iter = get_active();
  if(iter)
  {
    Gtk::TreeModel::Row row = *iter;
    return row[m_model_columns.m_identifier];
  }
  else
    return Glib::ustring();
}

void ComboBox_Locale::set_selected_locale(const Glib::ustring& locale)
{
  //Look for the row with this text, and activate it:
  Glib::RefPtr<Gtk::TreeModel> model = get_model();
  if(model)
  {
    for(Gtk::TreeModel::iterator iter = model->children().begin(); iter != model->children().end(); ++iter)
    {
      const Glib::ustring& this_text = (*iter)[m_model_columns.m_identifier];

      if(this_text == locale)
      {
        set_active(iter);
        return; //success
      }
    }
  }

  //Not found, so mark it as blank:
  std::cerr << "ComboBox_Locale::set_selected_locale(): locale not found in list: " << locale << std::endl;
  unset_active();
}






