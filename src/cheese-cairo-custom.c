/*
 * Copyright (C) 2007 Copyright (C) 2007 daniel g. siegel <dgsiegel@gmail.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cheese-cairo-custom.h"

void
cairo_rectangle_round (cairo_t *cr,
                       double x0,    double y0,
                       double width, double height,
                       double radius)
{
  double x1,y1;

  x1=x0+width;
  y1=y0+height;

  if (!width || !height)
    return;
  if (width/2<radius)
    {
      if (height/2<radius)
        {
          cairo_move_to  (cr, x0, (y0 + y1)/2);
          cairo_curve_to (cr, x0 ,y0, x0, y0, (x0 + x1)/2, y0);
          cairo_curve_to (cr, x1, y0, x1, y0, x1, (y0 + y1)/2);
          cairo_curve_to (cr, x1, y1, x1, y1, (x1 + x0)/2, y1);
          cairo_curve_to (cr, x0, y1, x0, y1, x0, (y0 + y1)/2);
        }
      else
        {
          cairo_move_to  (cr, x0, y0 + radius);
          cairo_curve_to (cr, x0 ,y0, x0, y0, (x0 + x1)/2, y0);
          cairo_curve_to (cr, x1, y0, x1, y0, x1, y0 + radius);
          cairo_line_to (cr, x1 , y1 - radius);
          cairo_curve_to (cr, x1, y1, x1, y1, (x1 + x0)/2, y1);
          cairo_curve_to (cr, x0, y1, x0, y1, x0, y1- radius);
        }
    }
  else
    {
      if (height/2<radius)
        {
          cairo_move_to  (cr, x0, (y0 + y1)/2);
          cairo_curve_to (cr, x0 , y0, x0 , y0, x0 + radius, y0);
          cairo_line_to (cr, x1 - radius, y0);
          cairo_curve_to (cr, x1, y0, x1, y0, x1, (y0 + y1)/2);
          cairo_curve_to (cr, x1, y1, x1, y1, x1 - radius, y1);
          cairo_line_to (cr, x0 + radius, y1);
          cairo_curve_to (cr, x0, y1, x0, y1, x0, (y0 + y1)/2);
        }
      else
        {
          cairo_move_to  (cr, x0, y0 + radius);
          cairo_curve_to (cr, x0 , y0, x0 , y0, x0 + radius, y0);
          cairo_line_to (cr, x1 - radius, y0);
          cairo_curve_to (cr, x1, y0, x1, y0, x1, y0 + radius);
          cairo_line_to (cr, x1 , y1 - radius);
          cairo_curve_to (cr, x1, y1, x1, y1, x1 - radius, y1);
          cairo_line_to (cr, x0 + radius, y1);
          cairo_curve_to (cr, x0, y1, x0, y1, x0, y1- radius);
        }
    }

  cairo_close_path (cr);
}


