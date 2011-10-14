/* Glom
 *
 * Copyright (C) 2001-2011 Openismus GmbH
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

#include <glom/print_layout/print_layout_utils.h>
#include <glom/print_layout/canvas_print_layout.h>
#include <glom/print_layout/printoperation_printlayout.h>
#include <iostream>

namespace Glom
{

namespace PrintLayoutUtils
{

double get_page_height(const Glib::RefPtr<const Gtk::PageSetup>& page_setup, Gtk::Unit units)
{
  double margin_top = 0;
  double margin_bottom = 0;
  return get_page_height(page_setup, units, margin_top, margin_bottom);
}

double get_page_height(const Glib::RefPtr<const Gtk::PageSetup>& page_setup, Gtk::Unit units, double& margin_top, double& margin_bottom)
{
  //Initialize output parameters:
  margin_top = 0;
  margin_bottom = 0;

  const Gtk::PaperSize paper_size = page_setup->get_paper_size();
  
  double page_height = 0;
  if(page_setup->get_orientation() == Gtk::PAGE_ORIENTATION_PORTRAIT) //TODO: Handle the reverse orientations too?
  {
    page_height = paper_size.get_height(units);
    margin_top = page_setup->get_top_margin(units);
    margin_bottom = page_setup->get_bottom_margin(units);
  }
  else
  {
    page_height = paper_size.get_width(units);
    margin_top = page_setup->get_left_margin(units);
    margin_bottom = page_setup->get_right_margin(units);
  }

  return page_height;
}

/* Get the start and end of the page, inside the margins.
 */
static void get_page_y_start_and_end(const Glib::RefPtr<const Gtk::PageSetup>& page_setup, Gtk::Unit units, guint page_number, double& y1, double& y2)
{
  y1 = 0;
  y2 = 0;
  
  const Gtk::PaperSize paper_size = page_setup->get_paper_size();
  
  double margin_top = 0;
  double margin_bottom = 0;
  const double page_height = get_page_height(page_setup, units, 
    margin_top, margin_bottom);
    
  //y1:
  y1 = page_height * (page_number);  
  double y_border = margin_top;
  while(y1 <= y_border)
    y1 += GRID_GAP;
  
  //y2:
  y2 = page_height * (page_number + 1);  
  y2 -= margin_bottom;

  //std::cout << G_STRFUNC << "page_number=" << page_number << ", y1=" << y1 << "y2=" << y2 << std::endl;
}

double get_offset_to_move_fully_to_next_page(const Glib::RefPtr<const Gtk::PageSetup>& page_setup, Gtk::Unit units, double y, double height)
{
  double top_margin = 0;
  double bottom_margin = 0;
  const double page_height = get_page_height(page_setup, units, top_margin, bottom_margin);

  const guint current_page = PrintLayoutUtils::get_page_for_y(page_setup, units, y);
  const double usable_page_start = current_page * page_height + top_margin + GRID_GAP;
  //std::cout << G_STRFUNC << ": debug: current_page=" << current_page << ", usable_page_start =" << usable_page_start << std::endl;

  if(y < usable_page_start) //If it is in the top margin:
  {
    return usable_page_start - y;
  }

  const double usable_page_end = (current_page + 1) * page_height - bottom_margin - GRID_GAP;
  if((y + height) > usable_page_end) //If it is in the bottom margin:
  {
    //Move it to the start of the next page:
    const double start_next_page_y = (current_page + 1) * page_height + top_margin + GRID_GAP;
    return start_next_page_y - y;
  }

  return 0;
}

static double move_fully_to_page(const Glib::RefPtr<const Gtk::PageSetup>& page_setup, Gtk::Unit units, double y, double height)
{
  double top_margin = 0;
  double bottom_margin = 0;
  const double page_height = get_page_height(page_setup, units, top_margin, bottom_margin);

  //Ignore items that would not overlap even if they had the same y:
  //Note that we try to keep an extra GRID_GAP from the edge of the margin:
  const double usable_page_height = page_height - top_margin - bottom_margin - GRID_GAP * 2;
  if(height > usable_page_height)
    return y; //It will always be in a margin because it is so big. We could never move it somewhere where it would not be.

  const double offset = get_offset_to_move_fully_to_next_page(page_setup, units, y, height);
  return y + offset;
}

bool needs_move_fully_to_page(const Glib::RefPtr<const Gtk::PageSetup>& page_setup, Gtk::Unit units, const Glib::RefPtr<const CanvasItemMovable>& item)
{
  double x = 0;
  double y = 0;
  item->get_xy(x, y);
  
  double width = 0;
  double height = 0;
  item->get_width_height(width, height);
  
  //We don't actually move it, because items would then group together 
  //at the top of pages.
  //Instead, the caller will discover an offset to apply to all items:
  const double y_new = move_fully_to_page(page_setup, units, y, height);
  if(y_new != y)
    return true;

  return false;
}

/*
static double move_fully_to_page(const Glib::RefPtr<const Gtk::PageSetup>& page_setup, Gtk::Unit units, const sharedptr<LayoutItem>& item)
{
  double x = 0;
  double y = 0;
  double width = 0;
  double height = 0;
  item->get_print_layout_position(x, y, width, height);
  
  const double y_new = move_fully_to_page(page_setup, units, y, height);
  if(y_new != y)
    item->set_print_layout_position(x, y_new, width, height);
    
  return y_new;
}
*/

static void create_standard(const sharedptr<const LayoutGroup>& layout_group, const sharedptr<LayoutGroup>& print_layout_group, const Glib::RefPtr<const Gtk::PageSetup>& page_setup, Gtk::Unit units, double x, double& y)
{
  if(!layout_group || !print_layout_group)
  {
    return;
  }

  const double gap = GRID_GAP;

  const Gtk::PaperSize paper_size = page_setup->get_paper_size();
  const double item_width = paper_size.get_width(units) - x -
    page_setup->get_right_margin(units) - gap;
  const double field_height = ITEM_HEIGHT;

  //Show the group's title
  //(but do not fall back to the name, because unnamed groups are really wanted sometimes.)
  const Glib::ustring title = layout_group->get_title();
  if(!title.empty())
  {
    sharedptr<LayoutItem_Text> text = sharedptr<LayoutItem_Text>::create();
    text->set_text(title);
    text->m_formatting.set_text_format_font("Sans Bold 10");

    y = move_fully_to_page(page_setup, units, y, field_height);
    text->set_print_layout_position(x, y, item_width, field_height); //TODO: Enough and no more.
    y += field_height + gap; //padding.

    print_layout_group->add_item(text);
  }

  //Deal with a portal group: 
  const sharedptr<const LayoutItem_Portal> portal = sharedptr<const LayoutItem_Portal>::cast_dynamic(layout_group); 
  if(portal)
  {
    sharedptr<LayoutItem_Portal> portal_clone = glom_sharedptr_clone(portal);
    portal_clone->set_print_layout_row_height(field_height); //Otherwise it will be 0, which is useless.

    //We ignore the rows count for the details layout's portal,
    //because that is only suitable for the on-screen layout,
    //and because, on the print layout, we want to show (almost) all rows: 
    portal_clone->set_rows_count(1 /* min */, 100 /* max */);

    //Set an initial default height, though this will be changed
    //when we fill it with data:
    y = move_fully_to_page(page_setup, units, y, field_height);
    portal_clone->set_print_layout_position(x, y, item_width, field_height);
    y += field_height + gap; //padding.

    print_layout_group->add_item(portal_clone);

    return;
  }

  //Deal with a regular group:
  //Recurse into the group's child items:
  for(LayoutGroup::type_list_items::const_iterator iter = layout_group->m_list_items.begin(); iter != layout_group->m_list_items.end(); ++iter)
  {
    const sharedptr<const LayoutItem> item = *iter;
    if(!item)
      continue;

    const sharedptr<const LayoutGroup> group = sharedptr<const LayoutGroup>::cast_dynamic(item);
    if(group && !portal)
    {
      //Recurse:
      create_standard(group, print_layout_group, page_setup, units, x, y);
    }
    else
    {
      //Add field titles, if necessary:
      sharedptr<LayoutItem_Text> text_title;
      const double title_width = ITEM_WIDTH_WIDE; //TODO: Calculate it based on the widest in the column. Or just halve the column to start.
      const sharedptr<const LayoutItem_Field> field = sharedptr<const LayoutItem_Field>::cast_dynamic(item);
      if(field)
      {
        text_title = sharedptr<LayoutItem_Text>::create();
        text_title->set_text(field->get_title_or_name() + ":");
        y = move_fully_to_page(page_setup, units, y, field_height);
        text_title->set_print_layout_position(x, y, title_width, field_height); //TODO: Enough and no more.
        text_title->m_formatting.set_text_format_font("Sans 10");

        print_layout_group->add_item(text_title);
      }

      //Add the item, such as a field:
      sharedptr<LayoutItem> clone = glom_sharedptr_clone(item);

      double item_x = x;
      if(field)
        item_x += (title_width + gap);

      const double field_width = item_width - title_width - gap;

      //Make multi-line fields bigger:
      //TODO: Add an auto-expand feature:
      double this_field_height = field_height;
      if(field)
      {
        const FieldFormatting& formatting = field->get_formatting_used();
        if(formatting.get_text_format_multiline())
        {
          const guint lines = formatting.get_text_format_multiline_height_lines();
          if(lines)
            this_field_height = field_height * lines;
        }
      }

      y = move_fully_to_page(page_setup, units, y, this_field_height); //TODO: Move the title down too, if this was moved.
      clone->set_print_layout_position(item_x, y, field_width, this_field_height); //TODO: Enough and no more.
      
      //Make sure that the title is still aligned, even if this was moved by move_fully_to_page().
      if(text_title)
        text_title->set_print_layout_position_y(y);
      
      y += this_field_height + gap; //padding.

      print_layout_group->add_item(clone);
    }
  }
}

guint get_page_for_y(const Glib::RefPtr<const Gtk::PageSetup>& page_setup, Gtk::Unit units, double y)
{
  const double page_height = get_page_height(page_setup, units);
  if(!page_height)
    return 0; //Avoid a division by zero.

  const double pages = y / (double)page_height;
  double pages_integral = 0;
  modf(pages, &pages_integral);
  return pages_integral;
}

sharedptr<PrintLayout> create_standard(const Glib::RefPtr<const Gtk::PageSetup>& page_setup, const Glib::ustring& table_name, const Document* document)
{
  const Gtk::Unit units = Gtk::UNIT_MM;
  sharedptr<PrintLayout> print_layout = sharedptr<PrintLayout>::create();  
  
  //Start inside the border, on the next grid line:
  double y = 0;
  double max_y = 0; //ignored
  get_page_y_start_and_end(page_setup, units, 0, y, max_y);
  
  double x = 0;
  double x_border = 0;
  if(page_setup)
    x_border = page_setup->get_left_margin(units);
  while(x <= x_border)
    x += GRID_GAP;
  
  //The table title:
  const Glib::ustring title = document->get_table_title_singular(table_name);
  if(!title.empty())
  {
    sharedptr<LayoutItem_Text> text = sharedptr<LayoutItem_Text>::create();
    text->set_text(title);
    text->m_formatting.set_text_format_font("Sans Bold 12");

    const double field_height = ITEM_HEIGHT;
    text->set_print_layout_position(x, y, ITEM_WIDTH_WIDE, field_height); //TODO: Enough and no more.
    y += field_height + GRID_GAP; //padding.

    print_layout->m_layout_group->add_item(text);
  }

  //The layout:
  //TODO: Use fill_layout_group_field_info()?
  const Document::type_list_layout_groups layout_groups = 
    document->get_data_layout_groups("details", table_name); //TODO: layout_platform.
  for(Document::type_list_layout_groups::const_iterator iter = layout_groups.begin(); iter != layout_groups.end(); ++iter)
  {
    const sharedptr<const LayoutGroup> group = *iter;
    if(!group)
      continue;

    create_standard(group, print_layout->m_layout_group, page_setup, units, x, y);
  }

  //Add extra pages if necessary:
  //y is probably _after_ the last item, not exactly at the bottom of the last item, so we subtract gap.
  //TODO: However, this might not be reliable,
  //so this could lead to an extra blank page.
  const guint page_number = get_page_for_y(page_setup, units, y - GRID_GAP);
  if(page_number >= print_layout->get_page_count())
  {
    print_layout->set_page_count(page_number + 1);
  }
  
  return print_layout;
}

void do_print_layout(const sharedptr<const PrintLayout>& print_layout, const FoundSet& found_set, bool preview, const Document* document, Gtk::Window* transient_for)
{
  if(!print_layout)
  {
    std::cerr << G_STRFUNC << ": print_layout was null" << std::endl;
    return;
  }
  
  if(!document)
  {
    std::cerr << G_STRFUNC << ": document was null" << std::endl;
    return;
  }
  
  //TODO: All this to be null when we allow that in Gtk::PrintOperation::run().
  if(!transient_for)
  {
    std::cerr << G_STRFUNC << ":  transient_for was null" << std::endl;
    return;
  }

  Canvas_PrintLayout canvas;
  canvas.set_document(const_cast<Document*>(document)); //We const_cast because, for this use, it will not be changed.
  canvas.set_print_layout(found_set.m_table_name, print_layout);

  //Do not show things that are only for editing the print layout:
  canvas.remove_grid();
  canvas.set_rules_visibility(false);
  canvas.set_outlines_visibility(false);

  //Create a new PrintOperation with our PageSetup and PrintSettings:
  //(We use our derived PrintOperation class)
  Glib::RefPtr<PrintOperationPrintLayout> print = PrintOperationPrintLayout::create();
  print->set_canvas(&canvas);

  print->set_track_print_status();

  //TODO: Put this in a utility function.
  Glib::RefPtr<Gtk::PageSetup> page_setup;
  const Glib::ustring key_file_text = print_layout->get_page_setup();
  if(!key_file_text.empty())
  {
    Glib::KeyFile key_file;
    key_file.load_from_data(key_file_text);
    page_setup = Gtk::PageSetup::create_from_key_file(key_file);
  }

  print->set_default_page_setup(page_setup);

  //print->set_print_settings(m_refSettings);

  //print->signal_done().connect(sigc::bind(sigc::mem_fun(*this,
  //                &ExampleWindow::on_printoperation_done), print));

  canvas.fill_with_data(found_set);

  try
  {
    print->run(
      (preview ? Gtk::PRINT_OPERATION_ACTION_PREVIEW : Gtk::PRINT_OPERATION_ACTION_PRINT_DIALOG),
      *transient_for);
  }
  catch (const Gtk::PrintError& ex)
  {
    //See documentation for exact Gtk::PrintError error codes.
    std::cerr << "An error occurred while trying to run a print operation:"
        << ex.what() << std::endl;
  }
}

} //namespace PrintLayoutUtils

} //namespace Glom

