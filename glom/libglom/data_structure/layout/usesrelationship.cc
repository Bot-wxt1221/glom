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

#include <libglom/data_structure/layout/fieldformatting.h>
#include <glibmm/i18n.h>

namespace Glom
{

UsesRelationship::UsesRelationship()
{
}

UsesRelationship::UsesRelationship(const UsesRelationship& src)
: m_relationship(src.m_relationship),
  m_related_relationship(src.m_related_relationship)
{
}

UsesRelationship::~UsesRelationship()
{
}

bool UsesRelationship::operator==(const UsesRelationship& src) const
{
  return (m_relationship == src.m_relationship)
         && (m_related_relationship == src.m_related_relationship);
}


UsesRelationship& UsesRelationship::operator=(const UsesRelationship& src)
{
  m_relationship = src.m_relationship;
  m_related_relationship = src.m_related_relationship;

  return *this;
}

bool UsesRelationship::get_has_relationship_name() const
{
  if(!m_relationship)
    return false;
  else
    return !(m_relationship->get_name().empty());
}

bool UsesRelationship::get_has_related_relationship_name() const
{
  if(!m_related_relationship)
    return false;
  else
    return !(m_related_relationship->get_name().empty());
}


Glib::ustring UsesRelationship::get_relationship_name() const
{
  if(m_relationship)
    return m_relationship->get_name();
  else
    return Glib::ustring();
}

Glib::ustring UsesRelationship::get_related_relationship_name() const
{
  if(m_related_relationship)
    return m_related_relationship->get_name();
  else
    return Glib::ustring();
}

sharedptr<Relationship> UsesRelationship::get_relationship() const
{
  return m_relationship;
}

void UsesRelationship::set_relationship(const sharedptr<Relationship>& relationship)
{
  m_relationship = relationship;
}

sharedptr<Relationship> UsesRelationship::get_related_relationship() const
{
  return m_related_relationship;
}

void UsesRelationship::set_related_relationship(const sharedptr<Relationship>& relationship)
{
  m_related_relationship = relationship;
}

Glib::ustring UsesRelationship::get_sql_table_or_join_alias_name(const Glib::ustring& parent_table) const
{
  if(get_has_relationship_name() || get_has_related_relationship_name())
  {
    const Glib::ustring result = get_sql_join_alias_name();
    if(result.empty())
    {
      //Non-linked-fields relationship:
      return get_table_used(parent_table);
    }
    else
      return result;
  }
  else
    return parent_table;
}

Glib::ustring UsesRelationship::get_table_used(const Glib::ustring& parent_table) const
{
  //std::cout << "UsesRelationship::get_table_used(): relationship=" << glom_get_sharedptr_name(m_relationship) << "related_relationship=" << glom_get_sharedptr_name(m_related_relationship) << std::endl;

  if(m_related_relationship)
    return m_related_relationship->get_to_table();
  else if(m_relationship)
    return m_relationship->get_to_table();
  else
    return parent_table;
}

Glib::ustring UsesRelationship::get_title_used(const Glib::ustring& parent_table_title) const
{
  if(m_related_relationship)
    return m_related_relationship->get_title_or_name();
  else if(m_relationship)
    return m_relationship->get_title_or_name();
  else
    return parent_table_title;
}

Glib::ustring UsesRelationship::get_title_singular_used(const Glib::ustring& parent_table_title) const
{
  sharedptr<Relationship> used = m_related_relationship;
  if(!used)
    used = m_relationship;

  if(!used)
    return Glib::ustring();

  const Glib::ustring result = used->get_title_singular();
  if(!result.empty())
    return result;
  else
    return get_title_used(parent_table_title);
}

Glib::ustring UsesRelationship::get_to_field_used() const
{
  if(m_related_relationship)
    return m_related_relationship->get_to_field();
  else if(m_relationship)
    return m_relationship->get_to_field();
  else
    return Glib::ustring();
}

Glib::ustring UsesRelationship::get_relationship_name_used() const
{
  if(m_related_relationship)
    return m_related_relationship->get_name();
  else if(m_relationship)
    return m_relationship->get_name();
  else
    return Glib::ustring();
}

bool UsesRelationship::get_relationship_used_allows_edit() const
{
  if(m_related_relationship)
    return m_related_relationship->get_allow_edit();
  else if(m_relationship)
    return m_relationship->get_allow_edit();
  else
    return false; /* Arbitrary default. */
}

Glib::ustring UsesRelationship::get_sql_join_alias_name() const
{
  Glib::ustring result;

  if(get_has_relationship_name() && m_relationship->get_has_fields()) //relationships that link to tables together via a field
  {
    //We use relationship_name.field_name instead of related_table_name.field_name,
    //because, in the JOIN below, will specify the relationship_name as an alias for the related table name
    result += ("relationship_" + m_relationship->get_name());

    /*
    const Glib::ustring field_table_name = relationship->get_to_table();
    if(field_table_name.empty())
    {
      g_warning("get_sql_join_alias_name(): field_table_name is null. relationship name = %s", relationship->get_name().c_str());
    }
    */

    if(get_has_related_relationship_name() && m_related_relationship->get_has_fields())
    {
      result += ('_' + m_related_relationship->get_name());
    }
  }

  return result;
}

void UsesRelationship::add_sql_join_alias_definition(const Glib::RefPtr<Gnome::Gda::SqlBuilder>& builder) const
{
  // Specify an alias, to avoid ambiguity when using 2 relationships to the same table.
  const Glib::ustring alias_name = get_sql_join_alias_name();
  const guint to_target_id = builder->select_add_target(m_relationship->get_to_table(), alias_name);

  // Add the JOIN:
  if(!get_has_related_relationship_name())
  {
    builder->select_join_targets(
      builder->select_add_target(m_relationship->get_from_table()),
      to_target_id,
      Gnome::Gda::SQL_SELECT_JOIN_LEFT,
      builder->add_cond(
        Gnome::Gda::SQL_OPERATOR_TYPE_EQ,
        builder->add_id("\"" + m_relationship->get_from_table() + "\".\"" + m_relationship->get_from_field() + "\""),
        builder->add_id("\"" + alias_name + "\".\"" + m_relationship->get_to_field() + "\"") ) );
  }
  else
  {
     UsesRelationship parent_relationship;
     parent_relationship.set_relationship(m_relationship);

     builder->select_join_targets(
      builder->select_add_target(m_relationship->get_from_table()), //TODO: Must we use the ID from select_add_target_id()?
      to_target_id,
      Gnome::Gda::SQL_SELECT_JOIN_LEFT,
      builder->add_cond(
        Gnome::Gda::SQL_OPERATOR_TYPE_EQ,
        builder->add_id("\"" + parent_relationship.get_sql_join_alias_name() + "\".\"" + m_related_relationship->get_from_field() + "\""),
        builder->add_id("\"" + alias_name + "\".\"" + m_relationship->get_to_field() + "\"") ) );
  }
}

} //namespace Glom
