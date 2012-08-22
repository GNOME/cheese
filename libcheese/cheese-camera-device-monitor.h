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


#ifndef __CHEESE_CAMERA_DEVICE_MONITOR_H__
#define __CHEESE_CAMERA_DEVICE_MONITOR_H__

#include <glib-object.h>
#include <cheese-camera-device.h>

G_BEGIN_DECLS

#define CHEESE_TYPE_CAMERA_DEVICE_MONITOR (cheese_camera_device_monitor_get_type ())
#define CHEESE_CAMERA_DEVICE_MONITOR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CHEESE_TYPE_CAMERA_DEVICE_MONITOR, \
                                                                               CheeseCameraDeviceMonitor))
#define CHEESE_CAMERA_DEVICE_MONITOR_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), CHEESE_TYPE_CAMERA_DEVICE_MONITOR, \
                                                                            CheeseCameraDeviceMonitorClass))
#define CHEESE_IS_CAMERA_DEVICE_MONITOR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CHEESE_TYPE_CAMERA_DEVICE_MONITOR))
#define CHEESE_IS_CAMERA_DEVICE_MONITOR_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CHEESE_TYPE_CAMERA_DEVICE_MONITOR))
#define CHEESE_CAMERA_DEVICE_MONITOR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CHEESE_TYPE_CAMERA_DEVICE_MONITOR, \
                                                                              CheeseCameraDeviceMonitorClass))

typedef struct _CheeseCameraDeviceMonitorPrivate CheeseCameraDeviceMonitorPrivate;
typedef struct _CheeseCameraDeviceMonitorClass CheeseCameraDeviceMonitorClass;
typedef struct _CheeseCameraDeviceMonitor CheeseCameraDeviceMonitor;

/**
 * CheeseCameraDeviceMonitor:
 *
 * Use the accessor functions below.
 */
struct _CheeseCameraDeviceMonitor
{
  /*< private >*/
  GObject parent;
  CheeseCameraDeviceMonitorPrivate *priv;
};

/**
 * CheeseCameraDeviceMonitorClass:
 * @added: invoked when a new video capture device is connected
 * @removed: invoked when a video capture device is removed
 *
 * Class for #CheeseCameraDeviceMonitor.
 */
struct _CheeseCameraDeviceMonitorClass
{
  /*< private >*/
  GObjectClass parent_class;

  /*< public >*/
  void (*added)(CheeseCameraDeviceMonitor *monitor,
                CheeseCameraDevice        *device);
  void (*removed)(CheeseCameraDeviceMonitor *monitor, const gchar *uuid);
};

GType                      cheese_camera_device_monitor_get_type (void) G_GNUC_CONST;
CheeseCameraDeviceMonitor *cheese_camera_device_monitor_new (void);
void                       cheese_camera_device_monitor_coldplug (CheeseCameraDeviceMonitor *monitor);

G_END_DECLS

#endif /* __CHEESE_CAMERA_DEVICE_MONITOR_H__ */
