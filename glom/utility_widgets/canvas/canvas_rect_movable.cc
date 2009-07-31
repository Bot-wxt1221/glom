/* Glom
 *
 * Copyright (C) 2007 Murray Cumming
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

#include "canvas_rect_movable.h"
#include <goocanvasmm/canvas.h>
#include <gdkmm/cursor.h>
#include <iostream>

namespace Glom
{


CanvasRectMovable::CanvasRectMovable()
: Goocanvas::Rect(0.0, 0.0, 0.0, 0.0),
  CanvasItemMovable(),
  m_snap_corner(CORNER_ALL)
{
  init();
}

CanvasRectMovable::CanvasRectMovable(double x, double y, double width, double height)
: Goocanvas::Rect(x, y, width, height),
  m_snap_corner(CORNER_ALL)
{
  init();
}

CanvasRectMovable::~CanvasRectMovable()
{
}

void CanvasRectMovable::init()
{
  signal_motion_notify_event().connect(sigc::mem_fun(*this, &CanvasItemMovable::on_motion_notify_event));
  signal_button_press_event().connect(sigc::mem_fun(*this, &CanvasItemMovable::on_button_press_event));
  signal_button_release_event().connect(sigc::mem_fun(*this, &CanvasItemMovable::on_button_release_event));

  signal_enter_notify_event().connect(sigc::mem_fun(*this, &CanvasItemMovable::on_enter_notify_event));
  signal_leave_notify_event().connect(sigc::mem_fun(*this, &CanvasItemMovable::on_leave_notify_event));
}

Glib::RefPtr<CanvasRectMovable> CanvasRectMovable::create()
{
  return Glib::RefPtr<CanvasRectMovable>(new CanvasRectMovable());
}

Glib::RefPtr<CanvasRectMovable> CanvasRectMovable::create(double x, double y, double width, double height)
{
  return Glib::RefPtr<CanvasRectMovable>(new CanvasRectMovable(x, y, width, height));
}

void CanvasRectMovable::get_xy(double& x, double& y) const
{
#ifdef GLIBMM_PROPERTIES_ENABLED
  x = property_x();
  y = property_y();
#else
  get_property("x", x);
  get_property("y", y);
#endif
}

void CanvasRectMovable::set_xy(double x, double y)
{
#ifdef GLIBMM_PROPERTIES_ENABLED
  property_x() = x;
  property_y() = y;
#else
  set_property("x", x);
  set_property("y", y);
#endif
}

void CanvasRectMovable::get_width_height(double& width, double& height) const
{
#ifdef GLIBMM_PROPERTIES_ENABLED
  width = property_width();
  height = property_height();
#else
  get_property("width", width);
  get_property("height", height);
#endif
}

void CanvasRectMovable::set_width_height(double width, double height)
{
#ifdef GLIBMM_PROPERTIES_ENABLED
  property_width() = width;
  property_height() = height;
#else
  set_property("width", width);
  set_property("height", height);
#endif
}

void CanvasRectMovable::snap_position_one_corner(Corners corner, double& x, double& y) const
{
  //Choose the offset of the part to snap to the grid:
  double corner_x_offset = 0;
  double corner_y_offset = 0;
  switch(corner)
  {
    case CORNER_TOP_LEFT:
      corner_x_offset = 0;
      corner_y_offset = 0;
      break;
    case CORNER_TOP_RIGHT:
#ifdef GLIBMM_PROPERTIES_ENABLED    
      corner_x_offset = property_width();
#else
      get_property("width", corner_x_offset);
#endif            
      corner_y_offset = 0;
      break;
    case CORNER_BOTTOM_LEFT:
      corner_x_offset = 0;
#ifdef GLIBMM_PROPERTIES_ENABLED    
      corner_y_offset = property_height();
#else
      get_property("height", corner_y_offset);
#endif            
      break;
    case CORNER_BOTTOM_RIGHT:
#ifdef GLIBMM_PROPERTIES_ENABLED    
      corner_x_offset = property_width();
#else
      get_property("width", corner_x_offset);
#endif            
#ifdef GLIBMM_PROPERTIES_ENABLED    
      corner_y_offset = property_height();
#else
      get_property("height", corner_y_offset);
#endif
      break;
    default:
      break;
  }

  //Snap that point to the grid:
  const double x_to_snap = x + corner_x_offset;
  const double y_to_snap = y + corner_y_offset;
  double corner_x_snapped = x_to_snap;
  double corner_y_snapped = y_to_snap;
  CanvasItemMovable::snap_position(corner_x_snapped, corner_y_snapped);

  //Discover what offset the snapping causes:
  const double snapped_offset_x = corner_x_snapped - x_to_snap;
  const double snapped_offset_y = corner_y_snapped - y_to_snap;

  //Apply that offset to the regular position:
  x += snapped_offset_x;
  y += snapped_offset_y;
}

void CanvasRectMovable::snap_position_all_corners(double& x, double& y) const
{
  double offset_x_min = 0;
  double offset_y_min = 0;

  //Try snapping each corner, to choose the one that snapped closest:
  for(int i = CORNER_TOP_LEFT; i < CORNER_COUNT; ++i)
  {
    const Corners corner = (Corners)i;
    double temp_x = x;
    double temp_y = y;
    snap_position_one_corner(corner, temp_x, temp_y);

    const double offset_x = temp_x -x;
    const double offset_y = temp_y - y;

    //Use the smallest offset, preferring some offset to no offset:
    if(offset_x && ((std::abs(offset_x) < std::abs(offset_x_min)) || !offset_x_min))
      offset_x_min = offset_x;

    if(offset_y && ((std::abs(offset_y) < std::abs(offset_y_min)) || !offset_y_min))
      offset_y_min = offset_y;
  }

  x += offset_x_min;
  y += offset_y_min;
}

void CanvasRectMovable::snap_position(double& x, double& y) const
{
  if(m_snap_corner == CORNER_ALL)
    return snap_position_all_corners(x, y);
  else
    return snap_position_one_corner(m_snap_corner, x, y);
}

Goocanvas::Canvas* CanvasRectMovable::get_parent_canvas_widget()
{
  return get_canvas();
}

void CanvasRectMovable::set_snap_corner(Corners corner)
{
  m_snap_corner = corner;
}

} //namespace Glom

