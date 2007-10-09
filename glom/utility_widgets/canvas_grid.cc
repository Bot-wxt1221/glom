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

#include "canvas_grid.h"
#include <math.h>

namespace Glom
{

inline void division_and_remainder(double a, double b, double& whole, double& remainder)
{
  if(b == 0)
  {
    whole = 0;
    remainder = 0;
    return;
  }

  remainder = fmod(a, b);
  whole = (int)(a / b);
}

bool CanvasGrid::is_close(double a, double b) const
{
  return (std::abs((long)(a - b)) < m_grid_sensitivity);

}
CanvasGrid::CanvasGrid()
: m_grid_gap(0.0),
  m_grid_sensitivity(5.0)
{
}

CanvasGrid::~CanvasGrid()
{
}

double CanvasGrid::snap_position_grid(double a) const
{
  double result = a;

  if(m_grid_gap)
  {
    /* Get closest horizontal grid line: */
    double grid_line_num_before = 0;
    double distance_after_grid_line_before = 0;
    division_and_remainder(a, m_grid_gap, grid_line_num_before, distance_after_grid_line_before);
    //printf("grid_line_num_before=%f, distance_after_grid_line_before=%f\n", grid_line_num_before, distance_after_grid_line_before);
    
    if(is_close(0, distance_after_grid_line_before))
    {
      //Snap to the grid line:
      result = grid_line_num_before * m_grid_gap; 
    }
    else
    {
      const double distance_to_next_grid_line = m_grid_gap - distance_after_grid_line_before;
      if(is_close(m_grid_gap, distance_to_next_grid_line))
      {
        //Snap to the grid line:
        result = (grid_line_num_before + 1) * m_grid_gap; 
      }
    }
  }

  return result;
}

void CanvasGrid::snap_position(double& x, double& y) const
{
  //printf("%s: x=%f, y=%f\n", __FUNCTION__, x, y);
  if(m_grid_gap)
  {
    x = snap_position_grid(x);
    y = snap_position_grid(y);
  }
}


} //namespace Glom

