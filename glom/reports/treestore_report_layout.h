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

#ifndef GLOM_MODE_DATA_TREESTORE_REPORT_LAYOUT_H
#define GLOM_MODE_DATA_TREESTORE_REPORT_LAYOUT_H

#include <glom/libglom/data_structure/layout/layoutitem.h>
#include <gtkmm/treestore.h>

namespace Glom
{

class TreeStore_ReportLayout : public Gtk::TreeStore
{
protected:
  TreeStore_ReportLayout();

public:

  //Tree model columns:  
  class ModelColumns : public Gtk::TreeModel::ColumnRecord
  {
  public:

    ModelColumns()
    { add(m_col_item); add(m_col_sequence); }

    Gtk::TreeModelColumn< sharedptr<LayoutItem> > m_col_item;
    Gtk::TreeModelColumn<guint> m_col_sequence;
  };

  ModelColumns m_columns;

  static Glib::RefPtr<TreeStore_ReportLayout> create();

  void fill_sequences();
  void fill_sequences(const iterator& iter);

  static bool may_be_child_of(const sharedptr<const LayoutItem>& parent, const sharedptr<const LayoutItem>& suggested_child);

protected:
  //Overridden virtual functions:
  virtual bool row_draggable_vfunc(const Gtk::TreeModel::Path& path) const;
  virtual bool row_drop_possible_vfunc(const Gtk::TreeModel::Path& dest, const Gtk::SelectionData& selection_data) const;

};

} //namespace Glom

#endif //GLOM_MODE_DATA_TREESTORE_REPORT_LAYOUT_H

