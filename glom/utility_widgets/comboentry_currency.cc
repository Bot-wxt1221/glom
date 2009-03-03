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

#include "comboentry_currency.h"
#include <gtk/gtkcomboboxentry.h>
#include <libglom/data_structure/iso_codes.h>

namespace Glom
{

ComboEntry_Currency::ComboEntry_Currency(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& /* refGlade */)
: Gtk::ComboBoxEntry(cobject)
{
  m_model = Gtk::ListStore::create(m_model_columns);

  //Fill the model:
  const IsoCodes::type_list_currencies list_currencies = IsoCodes::get_list_of_currency_symbols();
  for(IsoCodes::type_list_currencies::const_iterator iter = list_currencies.begin(); iter != list_currencies.end(); ++iter)
  {
    Gtk::TreeModel::iterator tree_iter = m_model->append();
    Gtk::TreeModel::Row row = *tree_iter;

    const IsoCodes::Currency& currency = *iter;
    row[m_model_columns.m_symbol] = currency.m_symbol;
    row[m_model_columns.m_name] = currency.m_name;
  }

  set_model(m_model);
  set_text_column(m_model_columns.m_symbol);

  //Show this too.
  pack_start(m_model_columns.m_name);
}


ComboEntry_Currency::~ComboEntry_Currency()
{

}

} //namespace Glom




