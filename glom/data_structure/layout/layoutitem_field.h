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
 
#ifndef GLOM_DATASTRUCTURE_LAYOUTITEM_FIELD_H
#define GLOM_DATASTRUCTURE_LAYOUTITEM_FIELD_H

#include "layoutitem.h"
#include "../field.h"
#include "../numeric_format.h"
#include "../relationship.h"

class LayoutItem_Field : public LayoutItem
{
public:

  LayoutItem_Field();
  LayoutItem_Field(const LayoutItem_Field& src);
  LayoutItem_Field& operator=(const LayoutItem_Field& src);
  virtual ~LayoutItem_Field();

  virtual LayoutItem* clone() const;

  bool operator==(const LayoutItem_Field& src) const;

  virtual void set_name(const Glib::ustring& name);
  virtual Glib::ustring get_name() const; //For use with our std::find_if() predicate.

  virtual Glib::ustring get_title_or_name() const;

  //virtual Glib::ustring get_table_name() const;
  //virtual void set_table_name(const Glib::ustring& table_name);

  /** Get a text representation for the field, such as Relationship::FieldName.
   */
  virtual Glib::ustring get_layout_display_name() const;

  virtual bool get_has_relationship_name() const;
  virtual Glib::ustring get_relationship_name() const;
  //virtual void set_relationship_name(const Glib::ustring& relationship_name);

  virtual Glib::ustring get_part_type_name() const;

  //This is filled in by looking at the database structure:
  Field m_field;
  //TODO: This might occasionally be different on different layouts: Glib::ustring m_title;


  bool get_editable_and_allowed() const;

  /// For extra fields, needed for SQL queries. The user should never be able to made an item hidden - he can just remove it.
  bool get_hidden() const;
  void set_hidden(bool val = true);

  //Not saved to the document:
  bool m_priv_view;
  bool m_priv_edit;

  //TODO_Performance: This is just cached data, so we don't need to always lookup the relationship details from the document, from the name.
  //Maybe use a smartpointer?
  Relationship m_relationship;

  FieldFormatting m_formatting;

  bool get_formatting_use_default() const;
  void set_formatting_use_default(bool use_default = true);

  const FieldFormatting& get_formatting_used() const;

protected:

  //Glib::ustring m_relationship_name; //bool m_related;
  bool m_hidden;
  bool m_formatting_use_default;

};

#endif //GLOM_DATASTRUCTURE_LAYOUTITEM_FIELD_H



