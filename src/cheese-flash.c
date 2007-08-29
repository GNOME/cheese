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

#include "cheese-config.h"

#include "cheese.h"

#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>

struct _cheese_flash cheese_flash;

struct _cheese_flash
{
  XF86VidModeGamma gamma;
  XF86VidModeGamma oldgamma;
  float gammafraction;
};

void
cheese_flash_set_gamma (float fraction)
{
  cheese_flash.gamma.red = 10.0 * fraction + cheese_flash.oldgamma.red * (1.0 - fraction);
  cheese_flash.gamma.green = 10.0 * fraction + cheese_flash.oldgamma.green * (1.0 - fraction);
  cheese_flash.gamma.blue = 10.0 * fraction + cheese_flash.oldgamma.blue * (1.0 - fraction);
  XF86VidModeSetGamma (GDK_DISPLAY (), 0, &cheese_flash.gamma);
}

void
cheese_flash_start ()
{
  XF86VidModeGetGamma (GDK_DISPLAY (), 0, &cheese_flash.oldgamma);
  cheese_flash_set_gamma (1.0);
  cheese_flash.gammafraction = 1.0;
}

void
cheese_flash_stop ()
{
  XF86VidModeSetGamma (GDK_DISPLAY (), 0, &cheese_flash.oldgamma);
}

gboolean
cheese_flash_dim ()
{
  cheese_flash.gammafraction -= 0.1;
  if (cheese_flash.gammafraction > 0.0)
  {
    cheese_flash_set_gamma (cheese_flash.gammafraction);
    return TRUE;
  }
  cheese_flash_stop ();
  return FALSE;
}
