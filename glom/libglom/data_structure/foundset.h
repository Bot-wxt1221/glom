/* Glom
 *
 * Copyright (C) 2001-2008 Murray Cumming
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

#ifndef GLOM_DATASTRUCTURE_FOUNDSET_H
#define GLOM_DATASTRUCTURE_FOUNDSET_H

#include <glibmm/ustring.h>
#include <vector>
#include <utility> //For std::pair
#include <libglom/data_structure/layout/layoutitem_field.h>
#include <libglom/sharedptr.h>

namespace Glom
{

/** A grouping of information about a view of a table, 
 * including what records are viewed (the where clause),
 * how the are  sorted (the sort clause).
 */
class FoundSet
{
public:
  FoundSet();
  FoundSet(const FoundSet& src);
  FoundSet& operator=(const FoundSet& src);

  bool operator==(const FoundSet& src) const;

  Glib::ustring m_table_name;
  sharedptr<const Relationship> m_extra_join; // Only used for doubly-related related records (portals), in which case the WHERE clause is also slightly different.
  Gnome::Gda::SqlExpr m_where_clause;

  //TODO: Avoid duplication with types in Formatting.
  ///field, ascending
  typedef std::pair< sharedptr<const LayoutItem_Field>, bool> type_pair_sort_field;
  typedef std::vector<type_pair_sort_field> type_sort_clause;
  type_sort_clause m_sort_clause;
};

} //namespace Glom

#endif //GLOM_DATASTRUCTURE_FOUNDSET_H

