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

#ifndef GLOM_TRANSLATION_COMBOBOX_RELATIONSHIP_HH
#define GLOM_TRANSLATION_COMBOBOX_RELATIONSHIP_HH

#include <gtkmm/combobox.h>
#include <libglademm.h>
#include <libglom/data_structure/relationship.h>
#include <libglom/document/document_glom.h>
#include <libglom/sharedptr.h>

#include <gtkmm/treestore.h>

namespace Glom
{

class ComboBox_Relationship : public Gtk::ComboBox
{
public:
  ComboBox_Relationship(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& refGlade);
  virtual ~ComboBox_Relationship();

  typedef std::vector< sharedptr<Relationship> > type_vecRelationships;
  void set_relationships(const type_vecRelationships& relationships, const Glib::ustring& parent_table_name = Glib::ustring(), const Glib::ustring& parent_table_title = Glib::ustring());

  void set_relationships(Document_Glom* document, const Glib::ustring parent_table_name, bool show_related_relationships = false, bool show_parent_table = true);

  void set_selected_relationship(const sharedptr<const Relationship>& relationship);
  void set_selected_relationship(const sharedptr<const Relationship>& relationship, const sharedptr<const Relationship>& related_relationship);
  void set_selected_relationship(const Glib::ustring& name, const Glib::ustring& related_relationship_name = Glib::ustring());
  sharedptr<Relationship> get_selected_relationship() const;
  sharedptr<Relationship> get_selected_relationship(sharedptr<Relationship>& related_relatioship) const;

  //Sometimes we want to show the parent table as an option too, instead of just relationships:

  ///Whether the parent table should be in the list.
  void set_display_parent_table(const Glib::ustring& table_name, const Glib::ustring& table_title = Glib::ustring());

  ///Select the parent table.
  void set_selected_parent_table(const Glib::ustring& table_name, const Glib::ustring& table_title = Glib::ustring());

private:

  //void on_cell_data_name(const Gtk::TreeModel::const_iterator& iter);
  void on_cell_data_title(const Gtk::TreeModel::const_iterator& iter);
  void on_cell_data_fromfield(const Gtk::TreeModel::const_iterator& iter);
  bool on_row_separator(const Glib::RefPtr<Gtk::TreeModel>& model, const Gtk::TreeModel::const_iterator& iter);

  bool get_has_parent_table() const;

  //Tree model columns:
  //These columns are used by the model that is created by the default constructor
  class ModelColumns : public Gtk::TreeModel::ColumnRecord
  {
  public:
    ModelColumns()
    { add(m_relationship); add(m_separator); }

    Gtk::TreeModelColumn< sharedptr<Relationship> > m_relationship;
    Gtk::TreeModelColumn<bool> m_separator;
  };

  ModelColumns m_model_columns;
  Glib::RefPtr<Gtk::TreeStore> m_model;

  //Gtk::CellRendererText* m_renderer_name;
  Gtk::CellRendererText* m_renderer_title;
  Gtk::CellRendererText* m_renderer_fromfield;

  Glib::ustring m_extra_table_name, m_extra_table_title;
};

} //namespace Glom

#endif //GLOM_TRANSLATION_COMBOBOX_RELATIONSHIP_HH

