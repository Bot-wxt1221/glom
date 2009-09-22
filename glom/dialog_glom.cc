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
 
#include <glom/dialog_glom.h>
#include <glom/utils_ui.h>

namespace Glom
{

Dialog_Glom::Dialog_Glom(Box_WithButtons* pBox, const Glib::ustring& title)
{
  if(!title.empty())
    set_title(title);

  set_border_width(Utils::DEFAULT_SPACING_SMALL);

  m_pBox = pBox;
  g_assert(m_pBox);

  m_pBox->signal_cancelled.connect(sigc::mem_fun(*this, &Dialog_Glom::on_box_cancelled));

  add(*m_pBox);
  m_pBox->show();

  //Set the default button, if there is one:
  Gtk::Widget* default_button = m_pBox->get_default_button();
  if(default_button)
    set_default(*default_button);
}

Dialog_Glom::~Dialog_Glom()
{
}

void Dialog_Glom::on_box_cancelled()
{
  hide();
}

} //namespace Glom
