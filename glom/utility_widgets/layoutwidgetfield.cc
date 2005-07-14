/* Glom
 *
 * Copyright (C) 2001-2005 Murray Cumming
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

#include "layoutwidgetfield.h"

LayoutWidgetField::LayoutWidgetField()
: m_entered_data_stored(false)
{
}

LayoutWidgetField::~LayoutWidgetField()
{
}

LayoutWidgetField::type_signal_edited LayoutWidgetField::signal_edited()
{
  return m_signal_edited;
}

bool LayoutWidgetField::get_has_original_data() const
{
  //Most widgets always have the original data.
  //Override this method for widgets that don't.
  return true;
}
