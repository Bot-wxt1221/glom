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


#ifndef BASE_DB_TABLE_H
#define BASE_DB_TABLE_H

#include <glom/base_db.h>
#include <libglom/data_structure/field.h>
#include <algorithm> //find_if used in various places.

namespace Glom
{

/** A base class that is a Bakery View with some database functionality 
 * for use with a specific database table.
 */
class Base_DB_Table : public Base_DB
{
public: 
  Base_DB_Table();
  virtual ~Base_DB_Table();

  virtual bool init_db_details(const Glib::ustring& table_name);

  Glib::ustring get_table_name() const;

protected:    
  Glib::ustring m_table_name;
};

} //namespace Glom

#endif //BASE_DB_TABLE_H
