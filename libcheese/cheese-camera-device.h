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

#ifndef __CHEESE_CAMERA_DEVICE_H__
#define __CHEESE_CAMERA_DEVICE_H__

#include <glib-object.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define CHEESE_TYPE_CAMERA_DEVICE (cheese_camera_device_get_type ())
#define CHEESE_CAMERA_DEVICE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CHEESE_TYPE_CAMERA_DEVICE, \
                                                                       CheeseCameraDevice))
#define CHEESE_CAMERA_DEVICE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), CHEESE_TYPE_CAMERA_DEVICE, \
                                                                    CheeseCameraDeviceClass))
#define CHEESE_IS_CAMERA_DEVICE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CHEESE_TYPE_CAMERA_DEVICE))
#define CHEESE_IS_CAMERA_DEVICE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CHEESE_TYPE_CAMERA_DEVICE))
#define CHEESE_CAMERA_DEVICE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CHEESE_TYPE_CAMERA_DEVICE, \
                                                                      CheeseCameraDeviceClass))

typedef struct _CheeseCameraDevicePrivate CheeseCameraDevicePrivate;
typedef struct _CheeseCameraDeviceClass CheeseCameraDeviceClass;
typedef struct _CheeseCameraDevice CheeseCameraDevice;

/**
 * CheeseCameraDeviceClass:
 *
 * Use the accessor functions below.
 */
struct _CheeseCameraDeviceClass
{
  /*< private >*/
  GObjectClass parent_class;
};

/**
 * CheeseCameraDevice:
 *
 * Use the accessor functions below.
 */
struct _CheeseCameraDevice
{
  /*< private >*/
  GObject parent;
  CheeseCameraDevicePrivate *priv;
};

#define CHEESE_TYPE_VIDEO_FORMAT (cheese_video_format_get_type ())

typedef struct _CheeseVideoFormat CheeseVideoFormat;

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

GType cheese_video_format_get_type (void) G_GNUC_CONST;

GType cheese_camera_device_get_type (void) G_GNUC_CONST;

CheeseCameraDevice *cheese_camera_device_new (const gchar *uuid,
                                              const gchar *device_node,
                                              const gchar *name,
                                              guint        v4l_api_version,
                                              GError     **error);

GstCaps *cheese_camera_device_get_caps_for_format (CheeseCameraDevice *device,
                                                   CheeseVideoFormat  *format);
CheeseVideoFormat *cheese_camera_device_get_best_format (CheeseCameraDevice *device);
GList *            cheese_camera_device_get_format_list (CheeseCameraDevice *device);

const gchar *cheese_camera_device_get_name (CheeseCameraDevice *device);
const gchar *cheese_camera_device_get_src (CheeseCameraDevice *device);
const gchar *cheese_camera_device_get_uuid (CheeseCameraDevice *device);
const gchar *cheese_camera_device_get_device_node (CheeseCameraDevice *device);



G_END_DECLS

#endif /* __CHEESE_CAMERA_DEVICE_H__ */
