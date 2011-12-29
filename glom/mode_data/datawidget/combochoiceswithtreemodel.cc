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

#include "combochoiceswithtreemodel.h"
#include <glom/mode_data/datawidget/treemodel_db_withextratext.h>
#include <libglom/data_structure/glomconversions.h>
#include <glom/utils_ui.h>
#include <gtkmm/liststore.h>
#include <glibmm/i18n.h>
//#include <sstream> //For stringstream

#include <locale>     // for locale, time_put
#include <ctime>     // for struct tm
#include <iostream>   // for cout, endl


namespace Glom
{

namespace DataWidgetChildren
{

ComboChoicesWithTreeModel::ComboChoicesWithTreeModel()
: m_fixed_cell_height(0)
{
  init();
}

ComboChoicesWithTreeModel::~ComboChoicesWithTreeModel()
{
  delete_model();
}

void ComboChoicesWithTreeModel::init()
{
  ComboChoices::init();
}

int ComboChoicesWithTreeModel::get_fixed_model_text_column() const
{
  const int count = m_refModel->get_n_columns();
  if(count > 0)
    return count -1;
  else
   return 0; //An error, but better than a negative number.
}

void ComboChoicesWithTreeModel::create_model_non_db(guint columns_count)
{
  delete_model();

  Gtk::TreeModel::ColumnRecord record;

  //Create the TreeModelColumns, adding them to the ColumnRecord:
    
  m_vec_model_columns_value_fixed.resize(columns_count, 0);
  for(guint i = 0; i < columns_count; ++i)
  {    
    //Create a value column for all columns
    //for instance for later value comparison.
    type_model_column_value_fixed* model_column = new type_model_column_value_fixed();
   
    //Store it so we can use it and delete it later:
    m_vec_model_columns_value_fixed[i] = model_column;

    record.add(*model_column);
  }
  
  //Create a text column, for use by a GtkComboBox with has-entry, which allows no other column type:
  //Note that get_fixed_model_text_column() assumes that this is the last column:
  m_vec_model_columns_string_fixed.resize(1, 0);
  if(columns_count > 0)
  {
    type_model_column_string_fixed* model_column = new type_model_column_string_fixed();
   
    //Store it so we can use it and delete it later:
    m_vec_model_columns_string_fixed.push_back(model_column);

    record.add(*model_column);
  }

  //Create the model:
  m_refModel = Gtk::ListStore::create(record);
}

void ComboChoicesWithTreeModel::delete_model()
{
  //Delete the vector's items:
  for(type_vec_model_columns_string_fixed::iterator iter = m_vec_model_columns_string_fixed.begin(); iter != m_vec_model_columns_string_fixed.end(); ++iter)
  {
    type_model_column_string_fixed* model_column = *iter;
    delete model_column;
  }
  m_vec_model_columns_string_fixed.clear();
  
  //Delete the vector's items:
  for(type_vec_model_columns_value_fixed::iterator iter = m_vec_model_columns_value_fixed.begin(); iter != m_vec_model_columns_value_fixed.end(); ++iter)
  {
    type_model_column_value_fixed* model_column = *iter;
    delete model_column;
  }
  m_vec_model_columns_value_fixed.clear();

  m_refModel.reset();
}

/* TODO: Remove this
void ComboChoicesWithTreeModel::set_choices_with_second(const type_list_values_with_second& list_values)
{
  //Recreate the entire model:
  guint columns_count = 1; //For the main field.
  if(!list_values.empty())
  {
    type_list_values_with_second::const_iterator iter= list_values.begin();
    if(iter != list_values.end())
    {
      const type_list_values& second = iter->second;
      columns_count += second.size();
    }
  }
  create_model(columns_count);

  //Fill the model with data:
  sharedptr<LayoutItem_Field> layout_item =
    sharedptr<LayoutItem_Field>::cast_dynamic(get_layout_item());
  const FieldFormatting& format = layout_item->get_formatting_used();
  sharedptr<const Relationship> choice_relationship;
  sharedptr<const LayoutItem_Field> layout_choice_first;
  sharedptr<const LayoutGroup> layout_choice_extra;
  bool choice_show_all = false;
  format.get_choices_related(choice_relationship, layout_choice_first, layout_choice_extra, choice_show_all);

  LayoutGroup::type_list_const_items extra_fields;
  if(layout_choice_extra)
    extra_fields = layout_choice_extra->get_items_recursive();

  Glib::RefPtr<Gtk::ListStore> list_store = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(m_refModel);
  if(!list_store)
  {
    std::cerr << G_STRFUNC << ": list_store is null." << std::endl;
    return;
  }

  for(type_list_values_with_second::const_iterator iter = list_values.begin(); iter != list_values.end(); ++iter)
  {
    Gtk::TreeModel::iterator iterTree = list_store->append();
    Gtk::TreeModel::Row row = *iterTree;

    if(layout_choice_first)
    {
      const Glib::ustring text =
        Conversions::get_text_for_gda_value(layout_choice_first->get_glom_type(), iter->first, layout_choice_first->get_formatting_used().m_numeric_format);
      row.set_value(0, text);

      const type_list_values extra_values = iter->second;
      if(layout_choice_extra && !extra_values.empty())
      {
        guint model_index = 1; //0 is for the main field.
        type_list_values::const_iterator iterValues = extra_values.begin();
        for(LayoutGroup::type_list_const_items::const_iterator iterExtra = extra_fields.begin();
          iterExtra != extra_fields.end(); ++iterExtra)
        {
          if(model_index >= columns_count)
            break;

          if(iterValues == extra_values.end())
            break;

          const sharedptr<const LayoutItem> item = *iterExtra;
          const sharedptr<const LayoutItem_Field> item_field = sharedptr<const LayoutItem_Field>::cast_dynamic(item);
          if(item_field)
          {
            const Gnome::Gda::Value value = *iterValues;
            const Glib::ustring text =
              Conversions::get_text_for_gda_value(item_field->get_glom_type(), value, item_field->get_formatting_used().m_numeric_format);
            row.set_value(model_index, text);
          }

          ++model_index;
          ++iterValues;
        }
      }
    }
  }
}
*/


void ComboChoicesWithTreeModel::set_choices_fixed(const FieldFormatting::type_list_values& list_values)
{
  create_model_non_db(1); //Use a regular ListStore without a dynamic column?

  Glib::RefPtr<Gtk::ListStore> list_store = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(m_refModel);
  if(!list_store)
  {
    std::cerr << G_STRFUNC << ": list_store is null." << std::endl;
    return;
  }

  for(FieldFormatting::type_list_values::const_iterator iter = list_values.begin(); iter != list_values.end(); ++iter)
  {
    Gtk::TreeModel::iterator iterTree = list_store->append();
    Gtk::TreeModel::Row row = *iterTree;

    sharedptr<const LayoutItem_Field> layout_item = sharedptr<LayoutItem_Field>::cast_dynamic(get_layout_item());
    if(layout_item)
    {
      const Gnome::Gda::Value value = *iter;
      row.set_value(0, value);
      
      const Glib::ustring text = Conversions::get_text_for_gda_value(layout_item->get_glom_type(), value, layout_item->get_formatting_used().m_numeric_format);
      row.set_value(1, text);
    }
  }

  //The derived class's (virtual) implementation calls this base method and
  //then sets up the view, using the model.
}

void ComboChoicesWithTreeModel::set_choices_related(const Document* document, const sharedptr<const LayoutItem_Field>& layout_field, const Gnome::Gda::Value& foreign_key_value)
{
  if(!document)
  {
    std::cerr << G_STRFUNC << ": document is null." << std::endl;
    return;
  }

  const FieldFormatting& format = layout_field->get_formatting_used();
  sharedptr<const Relationship> choice_relationship;
  sharedptr<const LayoutItem_Field> layout_choice_first;
  sharedptr<const LayoutGroup> layout_choice_extra;
  FieldFormatting::type_list_sort_fields choice_sort_fields;
  bool choice_show_all = false;
  format.get_choices_related(choice_relationship, layout_choice_first, layout_choice_extra, choice_sort_fields, choice_show_all);
  if(layout_choice_first->get_glom_type() == Field::TYPE_INVALID)
    std::cerr << G_STRFUNC << ": layout_choice_first has invalid type. field name: " << layout_choice_first->get_name() << std::endl;

  //Set full field details, cloning the group to avoid the constness:
  sharedptr<LayoutGroup> layout_choice_extra_full = glom_sharedptr_clone(layout_choice_extra);
  document->fill_layout_field_details(choice_relationship->get_to_table(), layout_choice_extra_full);

  //Get the list of fields to show:
  LayoutGroup::type_list_items extra_fields;
  if(layout_choice_extra_full)
    extra_fields = layout_choice_extra_full->get_items_recursive();

  LayoutGroup::type_list_const_items layout_items;
  layout_items.push_back(layout_choice_first);
  layout_items.insert(layout_items.end(), extra_fields.begin(), extra_fields.end());

  //Build the FoundSet:
  const Glib::ustring to_table = choice_relationship->get_to_table();
  FoundSet found_set;
  found_set.m_table_name = to_table;

  if(!foreign_key_value.is_null())
  {
    const sharedptr<const Field> to_field = document->get_field(to_table, choice_relationship->get_to_field());

    found_set.m_where_clause = Utils::build_simple_where_expression(
      to_table, to_field, foreign_key_value);
  }

  found_set.m_sort_clause = choice_sort_fields;
  if(found_set.m_sort_clause.empty())
  {
    //Sort by the first field, because that is better than so sort at all.
    found_set.m_sort_clause.push_back( FoundSet::type_pair_sort_field(layout_choice_first, true /* ascending */) );
  }

  m_db_layout_items.clear();

  //We create DbTreeModelWithExtraText rather than just DbTreeModel, 
  //because Combo(has_entry) needs it.
  m_refModel = DbTreeModelWithExtraText::create(found_set, layout_items, true /* allow_view */, false /* find mode */, m_db_layout_items);
  if(!m_refModel)
  {
    std::cerr << G_STRFUNC << ": DbTreeModel::create() returned a null model." << std::endl;
  }

  //The derived class's (virtual) implementation calls this base method and
  //then sets up the view, using the model.
}

Glib::RefPtr<Gtk::TreeModel> ComboChoicesWithTreeModel::get_choices_model()
{
  return m_refModel;
}

void ComboChoicesWithTreeModel::on_cell_data(const Gtk::TreeModel::iterator& iter, Gtk::CellRenderer* cell, guint model_column_index)
{
  //std::cout << G_STRFUNC << ": DEBUG: model_column_index=" << model_column_index << std::endl;
  if(model_column_index >= m_db_layout_items.size())
  {
    std::cerr << G_STRFUNC << ": model_column_index (" << model_column_index << ") is out of range. size=" << m_db_layout_items.size() << std::endl;
    return;
  }
   
  if(!cell)
  {
    std::cerr << G_STRFUNC << ": cell is null." << std::endl;
    return;
  }

  if(iter)
  {
    const sharedptr<const LayoutItem>& layout_item = m_db_layout_items[model_column_index];

    sharedptr<const LayoutItem_Field> field = sharedptr<const LayoutItem_Field>::cast_dynamic(layout_item);
    if(field)
    {
      Gtk::TreeModel::Row treerow = *iter;
      Gnome::Gda::Value value;
      treerow->get_value(model_column_index, value);

      const Field::glom_field_type type = field->get_glom_type();
      switch(type)
      {
        case(Field::TYPE_BOOLEAN):
        {
          Gtk::CellRendererToggle* pDerived = dynamic_cast<Gtk::CellRendererToggle*>(cell);
          if(pDerived)
            pDerived->set_active( (value.get_value_type() == G_TYPE_BOOLEAN) && value.get_boolean() );

          break;
        }
        case(Field::TYPE_IMAGE):
        {
          Gtk::CellRendererPixbuf* pDerived = dynamic_cast<Gtk::CellRendererPixbuf*>(cell);
          if(pDerived)
          {
            const Glib::RefPtr<Gdk::Pixbuf> pixbuf = Utils::get_pixbuf_for_gda_value(value);

            //Scale it down to a sensible size.
            //TODO: if(pixbuf)
            //  pixbuf = Utils::image_scale_keeping_ratio(pixbuf,  get_fixed_cell_height(), pixbuf->get_width());
            g_object_set(pDerived->gobj(), "pixbuf", pixbuf ? pixbuf->gobj() : 0, (gpointer)0);
          }
          else
            std::cerr << "Field::sql(): glom_type is TYPE_IMAGE but gda type is not VALUE_TYPE_BINARY" << std::endl;

          break;
        }
        default:
        {
          //TODO: Maybe we should have custom cellcells for time, date, and numbers.
          Gtk::CellRendererText* pDerived = dynamic_cast<Gtk::CellRendererText*>(cell);
          if(pDerived)
          {
            //std::cout << "debug: " << G_STRFUNC << ": field name=" << field->get_name() << ", glom type=" << field->get_glom_type() << std::endl;
            const Glib::ustring text = Conversions::get_text_for_gda_value(field->get_glom_type(), value, field->get_formatting_used().m_numeric_format);
            pDerived->property_text() = text;
          }
          else
          {
             std::cerr << G_STRFUNC << ": cell has an unexpected type: " << typeid(cell).name() << std::endl;
          }

          //Show a different color if the value is numeric, if that's specified:
          if(type == Field::TYPE_NUMERIC)
          {
             const Glib::ustring fg_color =
               field->get_formatting_used().get_text_format_color_foreground_to_use(value);
             if(!fg_color.empty())
                 g_object_set(pDerived->gobj(), "foreground", fg_color.c_str(), (gpointer)0);
             else
                 g_object_set(pDerived->gobj(), "foreground", (const char*)0, (gpointer)0);
          }

          break;
        }
      }
    }
  }
}

void ComboChoicesWithTreeModel::cell_connect_cell_data_func(Gtk::CellLayout* celllayout, Gtk::CellRenderer* cell, guint model_column_index)
{
  if(model_column_index >= m_db_layout_items.size())
  {
    std::cerr << G_STRFUNC << ": model_column_index (" << model_column_index << ") is out of range. size=" << m_db_layout_items.size() << std::endl;
    return;
  }
  
  celllayout->set_cell_data_func(*cell,
    sigc::bind( sigc::mem_fun(*this, &ComboChoicesWithTreeModel::on_cell_data), cell, model_column_index));
}

int ComboChoicesWithTreeModel::get_fixed_cell_height(Gtk::Widget& widget)
{
  if(m_fixed_cell_height <= 0)
  {
    // Discover a suitable height, and cache it,
    // by looking at the heights of all columns:
    // Note that this is usually calculated during construct_specified_columns(),
    // when all columns are known.

    //Get a default:
    const Glib::RefPtr<const Pango::Layout> refLayout = widget.create_pango_layout("example");
    int width = 0;
    int height = 0;
    refLayout->get_pixel_size(width, height);
    m_fixed_cell_height = height;

    //Look at each column:
    for(type_vec_const_layout_items::iterator iter = m_db_layout_items.begin(); iter != m_db_layout_items.end(); ++iter)
    {
      Glib::ustring font_name;

      const sharedptr<const LayoutItem_WithFormatting> item_withformatting = sharedptr<const LayoutItem_WithFormatting>::cast_dynamic(*iter);
      if(item_withformatting)
      {
         const FieldFormatting& formatting = item_withformatting->get_formatting_used();
         font_name = formatting.get_text_format_font();
      }

      if(font_name.empty())
        continue;

      // Translators: This is just some example text used to discover an appropriate height for user-entered text in the UI. This text itself is never shown to the user.
      Glib::RefPtr<Pango::Layout> refLayout = widget.create_pango_layout(_("Example"));
      const Pango::FontDescription font(font_name);
      refLayout->set_font_description(font);
      int width = 0;
      int height = 0;
      refLayout->get_pixel_size(width, height);

      if(height > m_fixed_cell_height)
        m_fixed_cell_height = height;
    }
  }

  return m_fixed_cell_height;
}

} //namespace DataWidetChildren
} //namespace Glom
