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

#include "layoutgroup.h"
#include "layoutitem_field.h"
#include "layoutitem_portal.h"
#include <glibmm/i18n.h>

namespace Glom
{

LayoutGroup::LayoutGroup()
: m_columns_count(1), //A sensible default
  m_border_width(0)
{
}

LayoutGroup::LayoutGroup(const LayoutGroup& src)
: LayoutItem(src),
  m_columns_count(src.m_columns_count),
  m_border_width(src.m_border_width)
{
  //Deep copy of the items map:
  for(type_list_items::const_iterator iter = src.m_list_items.begin(); iter != src.m_list_items.end(); ++iter)
  {
    if(*iter)
      m_list_items.push_back( glom_sharedptr_clone(*iter) );
  }
}

LayoutGroup::~LayoutGroup()
{
  remove_all_items();
}

void LayoutGroup::remove_all_items()
{
  //Delete the items:
  m_list_items.clear();
}

LayoutItem* LayoutGroup::clone() const
{
  return new LayoutGroup(*this);
}


LayoutGroup& LayoutGroup::operator=(const LayoutGroup& src)
{
  if(this != &src)
  {
    LayoutItem::operator=(src);

    m_columns_count = src.m_columns_count;

    m_border_width = src.m_border_width;

    //Deep copy of the items map:
    remove_all_items();
    for(type_list_items::const_iterator iter = src.m_list_items.begin(); iter != src.m_list_items.end(); ++iter)
    {
      if(*iter)
        m_list_items.push_back( glom_sharedptr_clone(*iter) );
    }
  }

  return *this;
}

bool LayoutGroup::has_field(const Glib::ustring& field_name) const
{
  for(type_list_items::const_iterator iter = m_list_items.begin(); iter != m_list_items.end(); ++iter)
  {
    sharedptr<LayoutItem> item = *iter;
    sharedptr<LayoutItem_Field> field_item = sharedptr<LayoutItem_Field>::cast_dynamic(item);
    if(field_item)
    {
      if(field_item->get_name() == field_name)
        return true;
    }
    else
    {
      //Recurse into the child groups:
      sharedptr<LayoutGroup> group_item = sharedptr<LayoutGroup>::cast_dynamic(item);
      if(group_item)
      {
        if(group_item->has_field(field_name))
          return true;
      }
    }
  }

  return false;
}

void LayoutGroup::add_item(const sharedptr<LayoutItem>& item)
{
  m_list_items.push_back(item);
}

void LayoutGroup::add_item(const sharedptr<LayoutItem>& item, const sharedptr<const LayoutItem>& position)
{
  //Find the position of the item.
  sharedptr<LayoutItem> unconst = sharedptr<LayoutItem>::cast_const(position);
  type_list_items::iterator iter = std::find(m_list_items.begin(), m_list_items.end(), unconst);
  if(iter == m_list_items.end())
    return; //position isn't an item already in the list.

  //std::vector::insert() adds before rather than after:
  ++iter;
 
  m_list_items.insert(iter, item);
}

LayoutGroup::type_list_items LayoutGroup::get_items()
{
  return m_list_items;
}

LayoutGroup::type_list_const_items LayoutGroup::get_items() const
{
  //Get a const map from the non-const map:
  //TODO_Performance: Surely we should not need to copy the structure just to constize it?
  type_list_const_items result;

  for(type_list_items::const_iterator iter = m_list_items.begin(); iter != m_list_items.end(); ++iter)
  {
    result.push_back(*iter);
  }

  return result;
}

void LayoutGroup::remove_relationship(const sharedptr<const Relationship>& relationship)
{
  LayoutGroup::type_list_items::iterator iterItem = m_list_items.begin();
  while(iterItem != m_list_items.end())
  {
    sharedptr<LayoutItem> item = *iterItem;
    sharedptr<UsesRelationship> uses_rel = sharedptr<UsesRelationship>::cast_dynamic(item);
    if(uses_rel)
    {
      if(uses_rel->get_has_relationship_name())
      {
        if(*(uses_rel->get_relationship()) == *relationship) //TODO_Performance: Slow if there are lots of translations.
        {
          m_list_items.erase(iterItem);
          iterItem = m_list_items.begin(); //Start again, because we changed the container.AddDel 
          continue;
        }
      }
    }

    sharedptr<LayoutGroup> sub_group = sharedptr<LayoutGroup>::cast_dynamic(item);
    if(sub_group)
      sub_group->remove_relationship(relationship);

    ++iterItem;
  }
}

void LayoutGroup::remove_field(const Glib::ustring& field_name)
{
  //Look at each item:
  LayoutGroup::type_list_items::iterator iterItem = m_list_items.begin();
  while(iterItem != m_list_items.end())
  {
    sharedptr<LayoutItem> item = *iterItem;
    sharedptr<LayoutItem_Field> field_item = sharedptr<LayoutItem_Field>::cast_dynamic(item);
    if(field_item)
    {
      if(!(field_item->get_has_relationship_name())) //If it's not a related table.
      {
        if(field_item->get_name() == field_name)
        {
          m_list_items.erase(iterItem);
          iterItem = m_list_items.begin(); //Start again, because we changed the container.AddDel 
          continue;
        }
      }
    }
    else
    {
      sharedptr<LayoutItem_Portal> sub_portal = sharedptr<LayoutItem_Portal>::cast_dynamic(item);
      if(!sub_portal) //It could only be a related field in a portal - use remove_field(table, field) for that.
      {
        sharedptr<LayoutGroup> sub_group = sharedptr<LayoutGroup>::cast_dynamic(item);
        if(sub_group)
          sub_group->remove_field(field_name);
      }
    }

    ++iterItem;
  }
}

void LayoutGroup::remove_field(const Glib::ustring& table_name, const Glib::ustring& field_name)
{
  //Look at each item:
  LayoutGroup::type_list_items::iterator iterItem = m_list_items.begin();
  while(iterItem != m_list_items.end())
  {
    sharedptr<LayoutItem> item = *iterItem;
    sharedptr<LayoutItem_Field> field_item = sharedptr<LayoutItem_Field>::cast_dynamic(item);
    if(field_item)
    {
      if(field_item->get_table_used(Glib::ustring()) == table_name)
      {
        if(field_item->get_name() == field_name)
        {
          m_list_items.erase(iterItem);
          iterItem = m_list_items.begin(); //Start again, because we changed the container.AddDel 
          continue;
        }
      }
    }
    else
    {
      sharedptr<LayoutGroup> sub_group = sharedptr<LayoutGroup>::cast_dynamic(item);
      if(sub_group)
        sub_group->remove_field(table_name, field_name);
    }

    ++iterItem;
  }
}

void LayoutGroup::change_related_field_item_name(const Glib::ustring& table_name, const Glib::ustring& field_name, const Glib::ustring& field_name_new)
{
  //Look at each item:
  for(LayoutGroup::type_list_items::iterator iterItem = m_list_items.begin(); iterItem != m_list_items.end(); ++iterItem)
  {
    sharedptr<LayoutItem> item = *iterItem;
    sharedptr<LayoutItem_Field> field_item = sharedptr<LayoutItem_Field>::cast_dynamic(item);
    if(field_item)
    {
      if(field_item->get_has_relationship_name()) //If it's related table.
      {
        sharedptr<const Relationship> relationship = field_item->get_relationship();
        if(relationship)
        {
          if(relationship->get_to_table() == table_name)
          {
            if(field_item->get_name() == field_name)
              field_item->set_name(field_name_new); //Change it.
          }
        }
      }
    }
    else
    {
      sharedptr<LayoutGroup> sub_group = sharedptr<LayoutGroup>::cast_dynamic(item);
      if(sub_group)
        sub_group->change_field_item_name(table_name, field_name, field_name_new);
    }
  }
}

void LayoutGroup::change_field_item_name(const Glib::ustring& table_name, const Glib::ustring& field_name, const Glib::ustring& field_name_new)
{
  //Look at each item:
  for(LayoutGroup::type_list_items::iterator iterItem = m_list_items.begin(); iterItem != m_list_items.end(); ++iterItem)
  {
    sharedptr<LayoutItem> item = *iterItem;
    sharedptr<LayoutItem_Field> field_item = sharedptr<LayoutItem_Field>::cast_dynamic(item);
    if(field_item)
    {
      if(field_item->get_has_relationship_name()) //If it's a related table (this would be a self-relationship)
      {
        sharedptr<const Relationship> rel = field_item->get_relationship();
        if(rel)
        {
          if(rel->get_to_table() == table_name)
          {
            if(field_item->get_name() == field_name)
              field_item->set_name(field_name_new); //Change it.
          }
        }
      }
      else
      {
        if(field_item->get_name() == field_name)
          field_item->set_name(field_name_new); //Change it.
      }

      field_item->m_formatting.change_field_name(table_name, field_name, field_name_new);
    }
    else
    {
      sharedptr<LayoutGroup> sub_group = sharedptr<LayoutGroup>::cast_dynamic(item);
      if(sub_group)
        sub_group->change_field_item_name(table_name, field_name, field_name_new);
    }
  }
}

/*
void LayoutGroup::change_relationship_name(const Glib::ustring& table_name, const Glib::ustring& name, const Glib::ustring& name_new)
{
  //Look at each item:
  for(LayoutGroup::type_list_items::iterator iterItem = m_list_items.begin(); iterItem != m_list_items.end(); ++iterItem)
  {
    LayoutItem_Field* field_item = dynamic_cast<LayoutItem_Field*>(*iterItem);
    if(field_item)
    {
      if(field_item->get_has_relationship_name())
      {
        if(field_item->get_relationship_name() == name)
        {
          field_item->set_relationship_name(name_new);
        }
      }

      field_item->m_formatting.change_relationship_name(table_name, name, name_new);
    }
    else
    {
      LayoutGroup* sub_group = dynamic_cast<LayoutGroup*>(*iterItem);
      if(sub_group)
        sub_group->change_relationship_name(table_name, name, name_new);
    }
  }
}
*/


Glib::ustring LayoutGroup::get_part_type_name() const
{
  return _("Group");
}

Glib::ustring LayoutGroup::get_report_part_id() const
{
  return "group";
}

guint LayoutGroup::get_items_count() const
{
  return m_list_items.size();
}

/*
void LayoutGroup::debug(guint level) const
{
  for(int i = 0; i < level; ++i)
    std::cout << " ";

  std::cout << "LayoutGroup::debug() level =" << level << std::endl;

  for(type_list_items::const_iterator iter = m_list_items.begin(); iter != m_list_items.end(); ++iter)
  {
    sharedptr<LayoutGroup> group = sharedptr<LayoutGroup>::cast_dynamic(*iter);
    if(group)
      group->debug(level + 1);
    else
    {
      sharedptr<LayoutItem_Field> field = sharedptr<LayoutItem_Field>::cast_dynamic(*iter);
      if(field)
      {
        for(int i = 0; i < level; ++i)
          std::cout << " ";

        std::cout << " field: name=" << field->get_name() << ", relationship=" << field->get_relationship_name() << std::endl;
      }
    }
  }
}
*/


double LayoutGroup::get_border_width() const
{
  return m_border_width;
}

void LayoutGroup::set_border_width(double border_width)
{
  m_border_width = border_width;
}

} //namespace Glom
