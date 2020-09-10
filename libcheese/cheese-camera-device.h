/*
 * Copyright © 2007,2008 Jaap Haitsma <jaap@haitsma.org>
 * Copyright © 2007-2009 daniel g. siegel <dgsiegel@gnome.org>
 * Copyright © 2008 Ryan zeigler <zeiglerr@gmail.com>
 * Copyright © 2009 Filippo Argiolas <filippo.argiolas@gmail.com>
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

#ifndef CHEESE_CAMERA_DEVICE_H_
#define CHEESE_CAMERA_DEVICE_H_

#include <glib-object.h>
#include <gst/gst.h>

G_BEGIN_DECLS

/**
 * CheeseCameraDevice:
 *
 * Use the accessor functions below.
 */
struct _CheeseCameraDevice
{
  /*< private >*/
  GObject parent;
  void *unused;
};

#define CHEESE_TYPE_VIDEO_FORMAT (cheese_video_format_get_type ())

/**
 * CheeseVideoFormat:
 * @width: the width of of the video, in pixels
 * @height: the height of the video, in pixels
 *
 * A description of the resolution, in pixels, of the format to capture with a
 * #CheeseCameraDevice.
 */
struct _CheeseVideoFormat
{
  /*< public >*/
  gint width;
  gint height;
};

typedef struct _CheeseVideoFormat CheeseVideoFormat;

GType cheese_video_format_get_type (void);

#define CHEESE_TYPE_CAMERA_DEVICE (cheese_camera_device_get_type ())
G_DECLARE_FINAL_TYPE (CheeseCameraDevice, cheese_camera_device, CHEESE, CAMERA_DEVICE, GObject)

CheeseCameraDevice *cheese_camera_device_new (GstDevice   *device,
                                              GError     **error);

GstCaps *cheese_camera_device_get_caps_for_format (CheeseCameraDevice *device,
                                                   CheeseVideoFormat  *format);
CheeseVideoFormat *cheese_camera_device_get_best_format (CheeseCameraDevice *device);
GList *            cheese_camera_device_get_format_list (CheeseCameraDevice *device);

const gchar *cheese_camera_device_get_name (CheeseCameraDevice *device);
const gchar *cheese_camera_device_get_path (CheeseCameraDevice *device);
GstElement * cheese_camera_device_get_src (CheeseCameraDevice *device);

GstCaps * cheese_camera_device_supported_format_caps (void);

G_END_DECLS

#endif /* CHEESE_CAMERA_DEVICE_H_ */
