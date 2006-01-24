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
 
#include "report.h"

Report::Report()
{
  m_translatable_item_type = TRANSLATABLE_TYPE_REPORT;
}

Report::Report(const Report& src)
: TranslatableItem(src),
  m_layout_group(src.m_layout_group)
{
}

Report& Report::operator=(const Report& src)
{
  TranslatableItem::operator=(src);

  m_layout_group = src.m_layout_group;

  return *this;
}

