/*
 * Copyright © 2007,2008 Jaap Haitsma <jaap@haitsma.org>
 * Copyright © 2007-2009 daniel g. siegel <dgsiegel@gnome.org>
 * Copyright © 2008 Ryan Zeigler <zeiglerr@gmail.com>
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
#ifdef HAVE_CONFIG_H
  #include <cheese-config.h>
#endif

#include "cheese-camera-device.h"

void
cheese_camera_device_free (CheeseCameraDevice *device)
{
  guint j;

  for (j = 0; j < device->num_video_formats; j++)
  {
    g_free (g_array_index (device->video_formats, CheeseVideoFormat, j).framerates);
    g_free (g_array_index (device->video_formats, CheeseVideoFormat, j).mimetype);
  }
  g_free (device->video_device);
  g_free (device->id);
  g_free (device->gstreamer_src);
  g_free (device->product_name);
  g_array_free (device->video_formats, TRUE);
  if (device->supported_resolutions != NULL)
    g_hash_table_destroy (device->supported_resolutions);
}

