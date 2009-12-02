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

#include <glib-object.h>
#include <libhal.h>
#include <string.h>

/* for ioctl query */
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev.h>

#include "cheese-camera-device-monitor.h"

G_DEFINE_TYPE (CheeseCameraDeviceMonitor, cheese_camera_device_monitor, G_TYPE_OBJECT)

#define CHEESE_CAMERA_DEVICE_MONITOR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
	  CHEESE_TYPE_CAMERA_DEVICE_MONITOR, CheeseCameraDeviceMonitorPrivate))

#define CHEESE_CAMERA_DEVICE_MONITOR_ERROR cheese_camera_device_monitor_error_quark ()

enum CheeseCameraDeviceMonitorError
{
  CHEESE_CAMERA_DEVICE_MONITOR_ERROR_UNKNOWN,
  CHEESE_CAMERA_DEVICE_MONITOR_ERROR_ELEMENT_NOT_FOUND
};

typedef struct
{
  guint filler;
} CheeseCameraDeviceMonitorPrivate;

enum
{
  ADDED,
  REMOVED,
  LAST_SIGNAL
};

static guint monitor_signals[LAST_SIGNAL];

GQuark
cheese_camera_device_monitor_error_quark (void)
{
  return g_quark_from_static_string ("cheese-camera-error-quark");
}

static void
cheese_camera_device_monitor_handle_udi (CheeseCameraDeviceMonitor *monitor,
					 LibHalContext *hal_ctx,
					 const char *udi)
{
  char                   *device_path;
  char                   *parent_udi = NULL;
  char                   *subsystem  = NULL;
  char                   *gstreamer_src, *product_name;
  struct v4l2_capability  v2cap;
  struct video_capability v1cap;
  gint                    vendor_id     = 0;
  gint                    product_id    = 0;
  gchar                  *property_name = NULL;
  CheeseCameraDevice     *device;
  DBusError               error;
  int fd, ok;

  parent_udi = libhal_device_get_property_string (hal_ctx, udi, "info.parent", &error);
  if (dbus_error_is_set (&error))
  {
    g_warning ("error getting parent for %s: %s: %s", udi, error.name, error.message);
    dbus_error_free (&error);
  }

  if (parent_udi != NULL)
  {
    subsystem = libhal_device_get_property_string (hal_ctx, parent_udi, "info.subsystem", NULL);
    if (subsystem == NULL)
      return;
    property_name = g_strjoin (".", subsystem, "vendor_id", NULL);
    vendor_id     = libhal_device_get_property_int (hal_ctx, parent_udi, property_name, &error);
    if (dbus_error_is_set (&error))
    {
      g_warning ("error getting vendor id: %s: %s", error.name, error.message);
      dbus_error_free (&error);
    }
    g_free (property_name);

    property_name = g_strjoin (".", subsystem, "product_id", NULL);
    product_id    = libhal_device_get_property_int (hal_ctx, parent_udi, property_name, &error);
    if (dbus_error_is_set (&error))
    {
      g_warning ("error getting product id: %s: %s", error.name, error.message);
      dbus_error_free (&error);
    }
    g_free (property_name);
    libhal_free_string (subsystem);
    libhal_free_string (parent_udi);
  }

  g_print ("Found device %04x:%04x, getting capabilities...\n", vendor_id, product_id);

  device_path = libhal_device_get_property_string (hal_ctx, udi, "video4linux.device", &error);
  if (dbus_error_is_set (&error))
  {
    g_warning ("error getting V4L device for %s: %s: %s", udi, error.name, error.message);
    dbus_error_free (&error);
    return;
  }

  /* vbi devices support capture capability too, but cannot be used,
   * so detect them by device name */
  if (strstr (device_path, "vbi"))
  {
    g_print ("Skipping vbi device: %s\n", device_path);
    libhal_free_string (device_path);
    return;
  }

  if ((fd = open (device_path, O_RDONLY | O_NONBLOCK)) < 0)
  {
    g_warning ("Failed to open %s: %s", device_path, strerror (errno));
    libhal_free_string (device_path);
    return;
  }
  ok = ioctl (fd, VIDIOC_QUERYCAP, &v2cap);
  if (ok < 0)
  {
    ok = ioctl (fd, VIDIOCGCAP, &v1cap);
    if (ok < 0)
    {
      g_warning ("Error while probing v4l capabilities for %s: %s",
		 device_path, strerror (errno));
      libhal_free_string (device_path);
      close (fd);
      return;
    }
    g_print ("Detected v4l device: %s\n", v1cap.name);
    g_print ("Device type: %d\n", v1cap.type);
    gstreamer_src = "v4lsrc";
    product_name  = v1cap.name;
  }
  else
  {
    guint cap = v2cap.capabilities;
    g_print ("Detected v4l2 device: %s\n", v2cap.card);
    g_print ("Driver: %s, version: %d\n", v2cap.driver, v2cap.version);

    /* g_print ("Bus info: %s\n", v2cap.bus_info); */ /* Doesn't seem anything useful */
    g_print ("Capabilities: 0x%08X\n", v2cap.capabilities);
    if (!(cap & V4L2_CAP_VIDEO_CAPTURE))
    {
      g_print ("Device %s seems to not have the capture capability, (radio tuner?)\n"
	       "Removing it from device list.\n", device_path);
      libhal_free_string (device_path);
      close (fd);
      return;
    }
    gstreamer_src = "v4l2src";
    product_name  = (char *) v2cap.card;
  }

  g_print ("\n");

  device = g_new0 (CheeseCameraDevice, 1);

  device->hal_udi           = g_strdup (udi);
  device->video_device      = g_strdup (device_path);
  device->gstreamer_src     = g_strdup (gstreamer_src);
  device->product_name      = g_strdup (product_name);
  device->num_video_formats = 0;
  device->video_formats     = g_array_new (FALSE, FALSE, sizeof (CheeseVideoFormat));
  device->supported_resolutions =
    g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  //FIXME This will leak a device, we should ref/unref it instead
  g_signal_emit (monitor, monitor_signals[ADDED], 0, device);
  libhal_free_string (device_path);
  close (fd);
}

void
cheese_camera_device_monitor_coldplug (CheeseCameraDeviceMonitor *monitor)
{
//  CheeseCameraDeviceMonitorPrivate *priv = CHEESE_CAMERA_DEVICE_MONITOR_GET_PRIVATE (camera);

  int            i;
  int            num_udis = 0;
  char         **udis;
  LibHalContext *hal_ctx;
  DBusError      error;

  g_print ("Probing devices with HAL...\n");

  dbus_error_init (&error);
  hal_ctx = libhal_ctx_new ();
  if (hal_ctx == NULL)
  {
    g_warning ("Could not create libhal context");
    dbus_error_free (&error);
    return;
  }

  if (!libhal_ctx_set_dbus_connection (hal_ctx, dbus_bus_get (DBUS_BUS_SYSTEM, &error)))
  {
    g_warning ("libhal_ctx_set_dbus_connection: %s: %s", error.name, error.message);
    dbus_error_free (&error);
    return;
  }

  if (!libhal_ctx_init (hal_ctx, &error))
  {
    if (dbus_error_is_set (&error))
    {
      g_warning ("libhal_ctx_init: %s: %s", error.name, error.message);
      dbus_error_free (&error);
    }
    g_warning ("Could not initialise connection to hald.\n"
               "Normally this means the HAL daemon (hald) is not running or not ready");
    return;
  }

  udis = libhal_find_device_by_capability (hal_ctx, "video4linux", &num_udis, &error);

  if (dbus_error_is_set (&error))
  {
    g_warning ("libhal_find_device_by_capability: %s: %s", error.name, error.message);
    dbus_error_free (&error);
    return;
  }

  /* Initialize camera structures */
  for (i = 0; i < num_udis; i++)
    cheese_camera_device_monitor_handle_udi (monitor, hal_ctx, udis[i]);
  libhal_free_string_array (udis);
}

static void
cheese_camera_device_monitor_finalize (GObject *object)
{
#if 0
  CheeseCameraDeviceMonitor *camera;

  camera = CHEESE_CAMERA_DEVICE_MONITOR (object);
  CheeseCameraDeviceMonitorPrivate *priv = CHEESE_CAMERA_DEVICE_MONITOR_GET_PRIVATE (camera);
#endif
  G_OBJECT_CLASS (cheese_camera_device_monitor_parent_class)->finalize (object);
}

static void
cheese_camera_device_monitor_class_init (CheeseCameraDeviceMonitorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = cheese_camera_device_monitor_finalize;

  monitor_signals[ADDED] = g_signal_new ("added", G_OBJECT_CLASS_TYPE (klass),
					 G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
					 G_STRUCT_OFFSET (CheeseCameraDeviceMonitorClass, added),
					 NULL, NULL,
					 g_cclosure_marshal_VOID__POINTER,
					 G_TYPE_NONE, 1, G_TYPE_POINTER);

  monitor_signals[REMOVED] = g_signal_new ("removed", G_OBJECT_CLASS_TYPE (klass),
					   G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
					   G_STRUCT_OFFSET (CheeseCameraDeviceMonitorClass, removed),
					   NULL, NULL,
					   g_cclosure_marshal_VOID__STRING,
					   G_TYPE_NONE, 1, G_TYPE_STRING);

  g_type_class_add_private (klass, sizeof (CheeseCameraDeviceMonitorPrivate));
}

static void
cheese_camera_device_monitor_init (CheeseCameraDeviceMonitor *camera)
{
//  CheeseCameraDeviceMonitorPrivate *priv = CHEESE_CAMERA_DEVICE_MONITOR_GET_PRIVATE (camera);

}

CheeseCameraDeviceMonitor *
cheese_camera_device_monitor_new (void)
{
  return g_object_new (CHEESE_TYPE_CAMERA_DEVICE_MONITOR, NULL);
}

/*
 * vim: sw=2 ts=8 cindent noai bs=2
 */
