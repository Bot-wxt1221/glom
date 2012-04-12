/* Glom
 *
 * Copyright (C) 2001-2006 Murray Cumming
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 */

#ifndef GLOM_DATASTRUCTURE_LAYOUTITEM_TEXT_H
#define GLOM_DATASTRUCTURE_LAYOUTITEM_TEXT_H

#include <libglom/data_structure/layout/layoutitem_withformatting.h>
#include <libglom/data_structure/layout/formatting.h>

namespace Glom
{

class LayoutItem_Text 
 : public LayoutItem_WithFormatting
{
public:

  LayoutItem_Text();
  LayoutItem_Text(const LayoutItem_Text& src);
  LayoutItem_Text& operator=(const LayoutItem_Text& src);
  virtual ~LayoutItem_Text();

  virtual LayoutItem* clone() const;

  bool operator==(const LayoutItem_Text& src) const;

  virtual Glib::ustring get_part_type_name() const;
  virtual Glib::ustring get_report_part_id() const;

  /** Get the text that will be shown on each record.
   */
  Glib::ustring get_text(const Glib::ustring& locale) const;

  /** Set the text that will be shown on each record.
   */
  void set_text(const Glib::ustring& text, const Glib::ustring& locale);

  /** Set the text's original (non-translated, usually English) text.
   * This is the same as calling set_text() with an empty locale parameter.
   */
  void set_text_original(const Glib::ustring& text);

  sharedptr<TranslatableItem> m_text; //Reuse the title concept of this class to give us translatable text.
};

} //namespace Glom

#endif //GLOM_DATASTRUCTURE_LAYOUTITEM_TEXT_H



