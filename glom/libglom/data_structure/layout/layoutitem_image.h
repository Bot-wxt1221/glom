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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef GLOM_DATASTRUCTURE_LAYOUTITEM_IMAGE_H
#define GLOM_DATASTRUCTURE_LAYOUTITEM_IMAGE_H

#include "layoutitem.h"
#include <libgdamm/value.h>
#include <gdkmm/pixbuf.h>

namespace Glom
{

  //JPEG seems to give ugly results when saved to the database and shown again.
  //#define GLOM_IMAGE_FORMAT "jpeg"
  //#define GLOM_IMAGE_FORMAT_MIME_TYPE "image/jpeg"
  #define GLOM_IMAGE_FORMAT "png"
  #define GLOM_IMAGE_FORMAT_MIME_TYPE "image/png"

class LayoutItem_Image 
 : public LayoutItem
{
public:
  LayoutItem_Image();
  LayoutItem_Image(const LayoutItem_Image& src);
  LayoutItem_Image& operator=(const LayoutItem_Image& src);
  virtual ~LayoutItem_Image();

  virtual LayoutItem* clone() const;

  bool operator==(const LayoutItem_Image& src) const;

  virtual Glib::ustring get_part_type_name() const;
  virtual Glib::ustring get_report_part_id() const;

  /** Get the image that will be shown on each record.
   */
  Gnome::Gda::Value get_image() const;

  /** Set the image that will be shown on each record.
   */
  void set_image(const Gnome::Gda::Value& image);

  //Saves the image to a temporary file and provides the file URI.
  Glib::ustring create_local_image_uri() const;

//private:
//This is public, for performance:
  Gnome::Gda::Value m_image;
};

} //namespace Glom

#endif //GLOM_DATASTRUCTURE_LAYOUTITEM_IMAGE_H



