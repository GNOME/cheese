/*
 * Copyright © 2007,2008 Jaap Haitsma <jaap@haitsma.org>
 * Copyright © 2007-2009 daniel g. siegel <dgsiegel@gnome.org>
 * Copyright © 2008 Ryan Zeigler <zeiglerr@gmail.com>
 * Copyright © 2010 Filippo Argiolas <filippo.argiolas@gmail.com>
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
#include <dbus/dbus-glib-lowlevel.h>
#include <string.h>

#ifdef HAVE_UDEV
  #define G_UDEV_API_IS_SUBJECT_TO_CHANGE 1
  #include <gudev/gudev.h>
#else
  #include <fcntl.h>
  #include <unistd.h>
  #include <sys/ioctl.h>
  #if USE_SYS_VIDEOIO_H > 0
    #include <sys/types.h>
    #include <sys/videoio.h>
  #elif defined (__sun)
    #include <sys/types.h>
    #include <sys/videodev2.h>
  #endif /* USE_SYS_VIDEOIO_H */
#endif

#include "cheese-camera-device-monitor.h"
#include "cheese-marshal.h"

/**
 * SECTION:cheese-camera-device-monitor
 * @short_description: Simple object to enumerate v4l devices
 * @include: cheese/cheese-camera-device-monitor.h
 *
 * #CheeseCameraDeviceMonitor provides a basic interface for
 * video4linux device enumeration and hotplugging.
 *
 * It uses either GUdev or some platform specific code to list video
 * devices.  It is also capable (right now in linux only, with the
 * udev backend) to monitor device plugging and emit a
 * CheeseCameraDeviceMonitor::added or
 * CheeseCameraDeviceMonitor::removed signal when an event happens.
 */

G_DEFINE_TYPE (CheeseCameraDeviceMonitor, cheese_camera_device_monitor, G_TYPE_OBJECT)

#define CHEESE_CAMERA_DEVICE_MONITOR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o),                               \
                                                                                  CHEESE_TYPE_CAMERA_DEVICE_MONITOR, \
                                                                                  CheeseCameraDeviceMonitorPrivate))

#define CHEESE_CAMERA_DEVICE_MONITOR_ERROR cheese_camera_device_monitor_error_quark ()

GST_DEBUG_CATEGORY (cheese_device_monitor_cat);
#define GST_CAT_DEFAULT cheese_device_monitor_cat

enum CheeseCameraDeviceMonitorError
{
  CHEESE_CAMERA_DEVICE_MONITOR_ERROR_UNKNOWN,
  CHEESE_CAMERA_DEVICE_MONITOR_ERROR_ELEMENT_NOT_FOUND
};

typedef struct
{
#ifdef HAVE_UDEV
  GUdevClient *client;
#else
  guint filler;
#endif /* HAVE_UDEV */
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

#ifdef HAVE_UDEV
static void
cheese_camera_device_monitor_added (CheeseCameraDeviceMonitor *monitor,
                                    GUdevDevice               *udevice)
{
  const char         *device_file;
  const char         *product_name;
  const char         *vendor;
  const char         *product;
  const char         *bus;
  gint                vendor_id   = 0;
  gint                product_id  = 0;
  gint                v4l_version = 0;

  const gchar *devpath = g_udev_device_get_property (udevice, "DEVPATH");

  GST_INFO ("Checking udev device '%s'", devpath);

  bus = g_udev_device_get_property (udevice, "ID_BUS");
  if (g_strcmp0 (bus, "usb") == 0)
  {
    vendor = g_udev_device_get_property (udevice, "ID_VENDOR_ID");
    if (vendor != NULL)
      vendor_id = g_ascii_strtoll (vendor, NULL, 16);
    product = g_udev_device_get_property (udevice, "ID_MODEL_ID");
    if (product != NULL)
      product_id = g_ascii_strtoll (product, NULL, 16);
    if (vendor_id == 0 || product_id == 0)
    {
      GST_WARNING ("Error getting vendor and product id");
    }
    else
    {
      GST_INFO ("Found device %04x:%04x, getting capabilities...", vendor_id, product_id);
    }
  }
  else
  {
    GST_INFO ("Not an usb device, skipping vendor and model id retrieval");
  }

  device_file = g_udev_device_get_device_file (udevice);
  if (device_file == NULL)
  {
    GST_WARNING ("Error getting V4L device");
    return;
  }

  /* vbi devices support capture capability too, but cannot be used,
   * so detect them by device name */
  if (strstr (device_file, "vbi"))
  {
    GST_INFO ("Skipping vbi device: %s", device_file);
    return;
  }

  v4l_version = g_udev_device_get_property_as_int (udevice, "ID_V4L_VERSION");
  if (v4l_version == 2 || v4l_version == 1)
  {
    const char *caps;

    caps = g_udev_device_get_property (udevice, "ID_V4L_CAPABILITIES");
    if (caps == NULL || strstr (caps, ":capture:") == NULL)
    {
      GST_WARNING ("Device %s seems to not have the capture capability, (radio tuner?)"
                   "Removing it from device list.", device_file);
      return;
    }
    product_name  = g_udev_device_get_property (udevice, "ID_V4L_PRODUCT");
  }
  else if (v4l_version == 0)
  {
    GST_ERROR ("Fix your udev installation to include v4l_id, ignoring %s", device_file);
    return;
  }
  else
  {
    g_assert_not_reached ();
  }

  g_signal_emit (monitor, monitor_signals[ADDED], 0,
                 devpath,
                 device_file,
                 product_name,
                 v4l_version);
}

static void
cheese_camera_device_monitor_removed (CheeseCameraDeviceMonitor *monitor,
                                      GUdevDevice               *udevice)
{
  g_signal_emit (monitor, monitor_signals[REMOVED], 0,
                 g_udev_device_get_property (udevice, "DEVPATH"));
}

static void
cheese_camera_device_monitor_uevent_cb (GUdevClient               *client,
                                        const gchar               *action,
                                        GUdevDevice               *udevice,
                                        CheeseCameraDeviceMonitor *monitor)
{
  if (g_str_equal (action, "remove"))
    cheese_camera_device_monitor_removed (monitor, udevice);
  else if (g_str_equal (action, "add"))
    cheese_camera_device_monitor_added (monitor, udevice);
}

/**
 * cheese_camera_device_monitor_coldplug:
 * @monitor: a #CheeseCameraDeviceMonitor object.
 *
 * Will actively look for plugged in cameras and emit
 * ::added for those new cameras.
 * This is only required when your program starts, so as to connect
 * to those signals before they are emitted.
 */
void
cheese_camera_device_monitor_coldplug (CheeseCameraDeviceMonitor *monitor)
{
  CheeseCameraDeviceMonitorPrivate *priv = CHEESE_CAMERA_DEVICE_MONITOR_GET_PRIVATE (monitor);
  GList                            *devices, *l;
  gint                              i = 0;

  if (priv->client == NULL)
    return;

  GST_INFO ("Probing devices with udev...");

  devices = g_udev_client_query_by_subsystem (priv->client, "video4linux");

  /* Initialize camera structures */
  for (l = devices; l != NULL; l = l->next)
  {
    cheese_camera_device_monitor_added (monitor, l->data);
    g_object_unref (l->data);
    i++;
  }
  g_list_free (devices);

  if (i == 0) GST_WARNING ("No device found");
}

#else /* HAVE_UDEV */
void
cheese_camera_device_monitor_coldplug (CheeseCameraDeviceMonitor *monitor)
{
  #if 0
  CheeseCameraDeviceMonitorPrivate *priv = CHEESE_CAMERA_DEVICE_MONITOR_GET_PRIVATE (monitor);
  struct v4l2_capability            v2cap;
  struct video_capability           v1cap;
  int                               fd, ok;

  if ((fd = open (device_path, O_RDONLY | O_NONBLOCK)) < 0)
  {
    g_warning ("Failed to open %s: %s", device_path, strerror (errno));
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
      close (fd);
      return;
    }
    gstreamer_src = "v4l2src";
    product_name  = (char *) v2cap.card;
  }
  close (fd);

  GList *devices, *l;

  g_print ("Probing devices with udev...\n");

  if (priv->client == NULL)
    return;

  devices = g_udev_client_query_by_subsystem (priv->client, "video4linux");

  /* Initialize camera structures */
  for (l = devices; l != NULL; l = l->next)
  {
    cheese_camera_device_monitor_added (monitor, l->data);
    g_object_unref (l->data);
  }
  g_list_free (devices);
  #endif
}

#endif /* HAVE_UDEV */

static void
cheese_camera_device_monitor_finalize (GObject *object)
{
#ifdef HAVE_UDEV
  CheeseCameraDeviceMonitor *monitor;

  monitor = CHEESE_CAMERA_DEVICE_MONITOR (object);
  CheeseCameraDeviceMonitorPrivate *priv = CHEESE_CAMERA_DEVICE_MONITOR_GET_PRIVATE (monitor);

  if (priv->client != NULL)
  {
    g_object_unref (priv->client);
    priv->client = NULL;
  }
#endif /* HAVE_UDEV */
  G_OBJECT_CLASS (cheese_camera_device_monitor_parent_class)->finalize (object);
}

static void
cheese_camera_device_monitor_class_init (CheeseCameraDeviceMonitorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  if (cheese_device_monitor_cat == NULL)
    GST_DEBUG_CATEGORY_INIT (cheese_device_monitor_cat,
                             "cheese-device-monitor",
                             0, "Cheese Camera Device Monitor");

  object_class->finalize = cheese_camera_device_monitor_finalize;

  /**
   * CheeseCameraDeviceMonitor::added:
   * @device: A private object representing the newly added camera.
   * @id: Device unique identifier.
   * @device: Device file name  (e.g. /dev/video2).
   * @product_name: Device product name (human readable, intended to be displayed in a UI).
   * @api_version: Supported video4linux API: 1 for v4l, 2 for v4l2.
   *
   * The ::added signal is emitted when a camera is added, or on start-up
   * after #cheese_camera_device_monitor_colplug is called.
   **/
  monitor_signals[ADDED] = g_signal_new ("added", G_OBJECT_CLASS_TYPE (klass),
                                         G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                         G_STRUCT_OFFSET (CheeseCameraDeviceMonitorClass, added),
                                         NULL, NULL,
                                         _cheese_marshal_VOID__STRING_STRING_STRING_INT,
                                         G_TYPE_NONE, 4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);

  /**
   * CheeseCameraDeviceMonitor::removed:
   * @device: A private object representing the newly added camera
   * @id: Device unique identifier.
   *
   * The ::removed signal is emitted when a camera is un-plugged, or
   * disabled on the system.
   **/
  monitor_signals[REMOVED] = g_signal_new ("removed", G_OBJECT_CLASS_TYPE (klass),
                                           G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                           G_STRUCT_OFFSET (CheeseCameraDeviceMonitorClass, removed),
                                           NULL, NULL,
                                           g_cclosure_marshal_VOID__STRING,
                                           G_TYPE_NONE, 1, G_TYPE_STRING);

  g_type_class_add_private (klass, sizeof (CheeseCameraDeviceMonitorPrivate));
}

static void
cheese_camera_device_monitor_init (CheeseCameraDeviceMonitor *monitor)
{
#ifdef HAVE_UDEV
  CheeseCameraDeviceMonitorPrivate *priv         = CHEESE_CAMERA_DEVICE_MONITOR_GET_PRIVATE (monitor);
  const gchar *const                subsystems[] = {"video4linux", NULL};

  priv->client = g_udev_client_new (subsystems);
  g_signal_connect (G_OBJECT (priv->client), "uevent",
                    G_CALLBACK (cheese_camera_device_monitor_uevent_cb), monitor);
#endif /* HAVE_UDEV */
}

/**
 * cheese_camera_device_monitor_new:
 *
 * Returns a new #CheeseCameraDeviceMonitor object.
 *
 * Return value: a new #CheeseCameraDeviceMonitor object.
 **/
CheeseCameraDeviceMonitor *
cheese_camera_device_monitor_new (void)
{
  return g_object_new (CHEESE_TYPE_CAMERA_DEVICE_MONITOR, NULL);
}

/*
 * vim: sw=2 ts=8 cindent noai bs=2
 */
