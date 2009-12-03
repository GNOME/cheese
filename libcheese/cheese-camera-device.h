/*
 * Copyright © 2007,2008 Jaap Haitsma <jaap@haitsma.org>
 * Copyright © 2007-2009 daniel g. siegel <dgsiegel@gnome.org>
 * Copyright © 2008 Ryan zeigler <zeiglerr@gmail.com>
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


#ifndef __CHEESE_CAMERA_DEVICE_H__
#define __CHEESE_CAMERA_DEVICE_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct
{
  int numerator;
  int denominator;
} CheeseFramerate;

typedef struct
{
  char *mimetype;
  int   width;
  int   height;
  int   num_framerates;
  CheeseFramerate *framerates;
  CheeseFramerate  highest_framerate;
} CheeseVideoFormat;

typedef struct
{
  char *video_device;
  char *id;
  char *gstreamer_src;
  char *product_name;
  int   num_video_formats;
  GArray *video_formats;

  /* Hash table for resolution based lookup of video_formats */
  GHashTable *supported_resolutions;
} CheeseCameraDevice;

void cheese_camera_device_free (CheeseCameraDevice *device);

G_END_DECLS

#endif /* __CHEESE_CAMERA_DEVICE_H__ */
