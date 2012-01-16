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

#include <glom/mode_design/layout/combobox_relationship.h>
#include <glom/application.h>
#include <glibmm/i18n.h>

namespace Glom
{

ComboBox_Relationship::ComboBox_Relationship(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& /* builder */)
: Gtk::ComboBox(cobject),
  m_renderer_title(0),
  m_renderer_fromfield(0)
{
  m_model = Gtk::TreeStore::create(m_model_columns);

  set_model(m_model);

  //Add name column:
  //m_renderer_name = Gtk::manage(new Gtk::CellRendererText());
  //pack_start(*m_renderer_name);
  //set_cell_data_func(*m_renderer_name, sigc::mem_fun(*this, &ComboBox_Relationship::on_cell_data_name));

  //Add title column:
  m_renderer_title = Gtk::manage(new Gtk::CellRendererText());
  pack_start(*m_renderer_title);
  set_cell_data_func(*m_renderer_title, sigc::mem_fun(*this, &ComboBox_Relationship::on_cell_data_title));

  //Add from field (as hint) column:
  m_renderer_fromfield = Gtk::manage(new Gtk::CellRendererText());
  pack_start(*m_renderer_fromfield);
  m_renderer_fromfield->set_property("xalign", 0.0f);
  set_cell_data_func(*m_renderer_fromfield, sigc::mem_fun(*this, &ComboBox_Relationship::on_cell_data_fromfield));

  set_row_separator_func(sigc::mem_fun(*this, &ComboBox_Relationship::on_row_separator));
}

ComboBox_Relationship::~ComboBox_Relationship()
{

}

sharedptr<Relationship> ComboBox_Relationship::get_selected_relationship() const
{
  Gtk::TreeModel::iterator iter = get_active();
  if(iter)
  {
    Gtk::TreeModel::Row row = *iter;
    return row[m_model_columns.m_relationship];
  }
  else
    return sharedptr<Relationship>();
}

sharedptr<Relationship> ComboBox_Relationship::get_selected_relationship(sharedptr<Relationship>& related_relationship) const
{
  Gtk::TreeModel::iterator iter = get_active();
  if(iter)
  {
    Gtk::TreeModel::Row row = *iter;
    Gtk::TreeModel::iterator iterParent = row.parent();
    if(iterParent)
    {
      //It's a related relationship:
      related_relationship = row[m_model_columns.m_relationship];
      return (*iterParent)[m_model_columns.m_relationship];
    }
    else
      return row[m_model_columns.m_relationship];
  }
  else
    return sharedptr<Relationship>();
}

void ComboBox_Relationship::set_selected_relationship(const sharedptr<const Relationship>& relationship)
{
  if(relationship)
    set_selected_relationship(relationship->get_name());
  else
    set_selected_relationship(Glib::ustring());
}

void ComboBox_Relationship::set_selected_relationship(const sharedptr<const Relationship>& relationship, const sharedptr<const Relationship>& related_relationship)
{
  if(relationship)
    set_selected_relationship(relationship->get_name(), glom_get_sharedptr_name(related_relationship));
  else
    set_selected_relationship(Glib::ustring());
}

void ComboBox_Relationship::set_selected_relationship(const Glib::ustring& relationship_name, const Glib::ustring& related_relationship_name)
{
  //Look for the row with this text, and activate it:
  Glib::RefPtr<Gtk::TreeModel> model = get_model();
  if(model)
  {
    for(Gtk::TreeModel::iterator iter = model->children().begin(); iter != model->children().end(); ++iter)
    {
      Gtk::TreeModel::Row row = *iter;
      sharedptr<Relationship> relationship = row[m_model_columns.m_relationship];
      const Glib::ustring this_name = glom_get_sharedptr_name(relationship);

      //(An empty name means Select the parent table item.)
      if(this_name == relationship_name)
      {
        if(related_relationship_name.empty())
        {
           set_active(iter);
           return; //success
        }
        else
        {
          for(Gtk::TreeModel::iterator iterChildren = iter->children().begin(); iterChildren != iter->children().end(); ++iterChildren)
          {
            Gtk::TreeModel::Row row = *iterChildren;
            sharedptr<Relationship> relationship = row[m_model_columns.m_relationship];
            const Glib::ustring this_name = glom_get_sharedptr_name(relationship);
            if(this_name == related_relationship_name)
            {
              set_active(iterChildren);
              return; //success
            }
          }
        }
      }
    }
  }

  //Not found, so mark it as blank:
  //std::cerr << G_STRFUNC << ": relationship not found in list: " << relationship_name << std::endl;

  //Avoid calling unset_active() if nothing is selected, because it triggers the changed signal unnecessarily.
  if(get_active()) //If something is active (selected).
    unset_active();
}

void ComboBox_Relationship::set_relationships(Document* document, const Glib::ustring parent_table_name, bool show_related_relationships, bool show_parent_table_name)
{
  if(!document)
    return;

  const Document::type_vec_relationships relationships = document->get_relationships(parent_table_name, true /* plus system properties */);

  m_model->clear();

  if(show_parent_table_name)
    set_display_parent_table(parent_table_name, document->get_table_title(parent_table_name, Application::get_current_locale()));

  //Fill the model:
  for(type_vec_relationships::const_iterator iter = relationships.begin(); iter != relationships.end(); ++iter)
  {
    Gtk::TreeModel::iterator tree_iter = m_model->append();
    Gtk::TreeModel::Row row = *tree_iter;

    sharedptr<Relationship> rel = *iter;
    row[m_model_columns.m_relationship] = rel;
    row[m_model_columns.m_separator] = false;

    //Children:
    if(show_related_relationships && !Document::get_relationship_is_system_properties(rel))
    {
      const Document::type_vec_relationships sub_relationships = document->get_relationships(rel->get_to_table(), false /* plus system properties */);
      for(type_vec_relationships::const_iterator iter = sub_relationships.begin(); iter != sub_relationships.end(); ++iter)
      {
        Gtk::TreeModel::iterator tree_iter_child = m_model->append(tree_iter->children());
        Gtk::TreeModel::Row row = *tree_iter_child;

        sharedptr<Relationship> rel = *iter;
        row[m_model_columns.m_relationship] = rel;
        row[m_model_columns.m_separator] = false;
      }
    }
  }
}

void ComboBox_Relationship::set_relationships(const type_vec_relationships& relationships, const Glib::ustring& parent_table_name, const Glib::ustring& parent_table_title)
{
  m_model->clear();

  set_display_parent_table(parent_table_name, parent_table_title);

  //Fill the model:
  for(type_vec_relationships::const_iterator iter = relationships.begin(); iter != relationships.end(); ++iter)
  {
    Gtk::TreeModel::iterator tree_iter = m_model->append();
    Gtk::TreeModel::Row row = *tree_iter;

    row[m_model_columns.m_relationship] = *iter;
    row[m_model_columns.m_separator] = false;
  }
}

/*
void ComboBox_Relationship::on_cell_data_name(const Gtk::TreeModel::const_iterator& iter)
{
  Gtk::TreeModel::Row row = *iter;
  sharedptr<Relationship> relationship = row[m_model_columns.m_relationship];
  if(relationship)
    m_renderer_name->property_text() = relationship->get_name();
  else if(get_has_parent_table())
    m_renderer_name->property_text() = m_extra_table_name;
}
*/

void ComboBox_Relationship::on_cell_data_title(const Gtk::TreeModel::const_iterator& iter)
{
  Gtk::TreeModel::Row row = *iter;
  sharedptr<Relationship> relationship = row[m_model_columns.m_relationship];
  if(relationship)
  {
    Gtk::TreeModel::iterator iterParent = row->parent();
    if(iterParent)
    {
      //related relationship:
      sharedptr<Relationship> parent_relationship = (*iterParent)[m_model_columns.m_relationship];
      if(relationship)
        m_renderer_title->set_property("text", parent_relationship->get_title_or_name(Application::get_current_locale()) + "::" + relationship->get_title_or_name(Application::get_current_locale()));
    }
    else
      m_renderer_title->set_property("text", relationship->get_title_or_name(Application::get_current_locale()));
  }
  else if(get_has_parent_table())
  {
    m_renderer_title->set_property("text", (m_extra_table_title.empty() ? m_extra_table_name : m_extra_table_title));
  }
  else
  {
    //std::cerr << G_STRFUNC << ": empty relationship and no m_extra_table_name. m_extra_table_name=" << m_extra_table_name << std::endl;
  }
}

bool ComboBox_Relationship::on_row_separator(const Glib::RefPtr<Gtk::TreeModel>& /* model */, const Gtk::TreeModel::const_iterator& iter)
{
  Gtk::TreeModel::Row row = *iter;
  const bool separator = row[m_model_columns.m_separator];
  return separator;
}

void ComboBox_Relationship::on_cell_data_fromfield(const Gtk::TreeModel::const_iterator& iter)
{
  Gtk::TreeModel::Row row = *iter;
  sharedptr<Relationship> relationship = row[m_model_columns.m_relationship];
  if(relationship && relationship->get_has_fields())
  {
    Gtk::TreeModel::iterator iterParent = iter->parent();
    if(iterParent)
    {
      sharedptr<Relationship> parent_relationship = (*iterParent)[m_model_columns.m_relationship];
      if(parent_relationship)
        m_renderer_fromfield->set_property("text", Glib::ustring::compose(_(" Via: %1::%2"), parent_relationship->get_title(Application::get_current_locale()), relationship->get_from_field()));
    }
    else
    {
      m_renderer_fromfield->set_property("text", Glib::ustring::compose(_(" Via: %1"), relationship->get_to_field()));
    }
  }
  else
    m_renderer_fromfield->set_property("text", Glib::ustring());
}

void ComboBox_Relationship::set_display_parent_table(const Glib::ustring& table_name, const Glib::ustring& table_title)
{
  if(table_name.empty())
    return;

  const bool already_added = get_has_parent_table() && !(m_model->children().empty());

  //We don't need to recreate the model row when these change, because the callback just uses the new values.
  m_extra_table_name = table_name;
  m_extra_table_title = table_title;

  if(!already_added)
  {
    //Add a separator row after the table name:
    Gtk::TreeModel::iterator tree_iter = m_model->prepend();
    Gtk::TreeModel::Row row = *tree_iter;
    row[m_model_columns.m_separator] = true;

    tree_iter = m_model->prepend();
    row = *tree_iter;
    row[m_model_columns.m_relationship] = sharedptr<Relationship>(); //A marker for the parent table's item. See the on_data_* signal handlers.
    row[m_model_columns.m_separator] = false;
  }
}

void ComboBox_Relationship::set_selected_parent_table(const Glib::ustring& table_name, const Glib::ustring& table_title)
{
  //Save the values:
  set_display_parent_table(table_name, table_title);

  set_selected_relationship(sharedptr<Relationship>()); //Select the extra item.
}

bool ComboBox_Relationship::get_has_parent_table() const
{
  return !(m_extra_table_name.empty()) || !(m_extra_table_title.empty());
}

} //namespace Glom




