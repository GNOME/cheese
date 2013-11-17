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

/**
 * SECTION:cheese-camera-device-monitor
 * @short_description: Simple object to enumerate v4l devices
 * @stability: Unstable
 * @include: cheese/cheese-camera-device-monitor.h
 *
 * #CheeseCameraDeviceMonitor provides a basic interface for video4linux device
 * enumeration and hotplugging.
 *
 * It uses either GUdev or some platform specific code to list video devices.
 * It is also capable (right now in Linux only, with the udev backend) to
 * monitor device plugging and emit a CheeseCameraDeviceMonitor::added or
 * CheeseCameraDeviceMonitor::removed signal when an event happens.
 */

struct _CheeseCameraDeviceMonitorPrivate
{
#ifdef HAVE_UDEV
  GUdevClient *client;
#else
  guint filler;
#endif /* HAVE_UDEV */
};

G_DEFINE_TYPE_WITH_PRIVATE (CheeseCameraDeviceMonitor, cheese_camera_device_monitor, G_TYPE_OBJECT)

#define CHEESE_CAMERA_DEVICE_MONITOR_ERROR cheese_camera_device_monitor_error_quark ()

GST_DEBUG_CATEGORY (cheese_device_monitor_cat);
#define GST_CAT_DEFAULT cheese_device_monitor_cat

enum
{
  ADDED,
  REMOVED,
  LAST_SIGNAL
};

static guint monitor_signals[LAST_SIGNAL];

GQuark cheese_camera_device_monitor_error_quark (void);

GQuark
cheese_camera_device_monitor_error_quark (void)
{
  return g_quark_from_static_string ("cheese-camera-error-quark");
}

#ifdef HAVE_UDEV

/*
 * cheese_camera_device_monitor_set_up_device:
 * @udevice: the device information from udev
 *
 * Creates a new #CheeseCameraDevice for the supplied @udevice.
 *
 * Returns: a new #CheeseCameraDevice, or %NULL if @udevice was not a V4L
 * capture device
 */
static CheeseCameraDevice*
cheese_camera_device_monitor_set_up_device (GUdevDevice *udevice)
{
  const char *device_file;
  const char *product_name;
  const char *vendor;
  const char *product;
  const char *bus;
  GError      *error = NULL;
  gint        vendor_id   = 0;
  gint        product_id  = 0;
  gint        v4l_version = 0;
  CheeseCameraDevice *device;

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
    return NULL;
  }

  /* vbi devices support capture capability too, but cannot be used,
   * so detect them by device name */
  if (strstr (device_file, "vbi"))
  {
    GST_INFO ("Skipping vbi device: %s", device_file);
    return NULL;
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
      return NULL;
    }
    product_name = g_udev_device_get_property (udevice, "ID_V4L_PRODUCT");
  }
  else if (v4l_version == 0)
  {
    GST_ERROR ("Fix your udev installation to include v4l_id, ignoring %s", device_file);
    return NULL;
  }
  else
  {
    g_assert_not_reached ();
  }

  device = cheese_camera_device_new (devpath,
                                     device_file,
                                     product_name,
                                     v4l_version,
                                     &error);

  if (device == NULL)
    GST_WARNING ("Device initialization for %s failed: %s ",
                 device_file,
                 (error != NULL) ? error->message : "Unknown reason");

  return device;
}

/*
 * cheese_camera_device_monitor_added:
 * @monitor: a #CheeseCameraDeviceMonitor
 * @udevice: the device information, from udev, for the device that was added
 *
 * Emits the ::added signal.
 */
static void
cheese_camera_device_monitor_added (CheeseCameraDeviceMonitor *monitor,
                                    GUdevDevice               *udevice)
{
  CheeseCameraDevice *device = cheese_camera_device_monitor_set_up_device (udevice);
  /* Ignore non-video devices, GNOME bug #677544. */
  if (device)
    g_signal_emit (monitor, monitor_signals[ADDED], 0, device);
}

/*
 * cheese_camera_device_monitor_removed:
 * @monitor: a #CheeseCameraDeviceMonitor
 * @udevice: the device information, from udev, for the device that was removed
 *
 * Emits the ::removed signal.
 */
static void
cheese_camera_device_monitor_removed (CheeseCameraDeviceMonitor *monitor,
                                      GUdevDevice               *udevice)
{
  g_signal_emit (monitor, monitor_signals[REMOVED], 0,
                 g_udev_device_get_property (udevice, "DEVPATH"));
}

/*
 * cheese_camera_device_monitor_uevent_cb:
 * @client: a #GUdevClient
 * @action: the string representing the action type of the uevent
 * @udevice: the #GUdevDevice to which the uevent refers
 * @monitor: a #CheeseCameraDeviceMonitor
 *
 * Check if the uevent corresponds to device addition or removal, and if so,
 * pass it on to cheese_camera_device_monitor_added() or
 * cheese_camera_device_monitor_removed() for emitting the ::added and
 * ::removed signals.
 */
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

/*
 * cheese_camera_device_monitor_add_devices:
 * @data: the #GUdevDevice to add
 * @user_data: the #CheeseCameraDeviceMonitor
 *
 * Add a #GUdevDevice representing a video capture device to the list. This
 * method is intended to be used as a #GFunc for g_list_foreach(), during
 * coldplug at application startup.
 */
static void
cheese_camera_device_monitor_add_devices (gpointer data, gpointer user_data)
{
  cheese_camera_device_monitor_added ((CheeseCameraDeviceMonitor *) user_data,
    (GUdevDevice *) data);
  g_object_unref (data);
}

/**
 * cheese_camera_device_monitor_coldplug:
 * @monitor: a #CheeseCameraDeviceMonitor
 *
 * Enumerate plugged in cameras and emit ::added for those which already exist.
 * This is only required when your program starts, so be sure to connect to
 * at least the ::added signal before calling this function.
 */
void
cheese_camera_device_monitor_coldplug (CheeseCameraDeviceMonitor *monitor)
{
  GList *devices;

  g_return_if_fail (CHEESE_IS_CAMERA_DEVICE_MONITOR (monitor)
    || monitor->priv->client != NULL);

  GST_INFO ("Probing devices with udev...");

  devices = g_udev_client_query_by_subsystem (monitor->priv->client, "video4linux");
  if (devices == NULL) GST_WARNING ("No device found");

  /* Initialize camera structures */
  g_list_foreach (devices, cheese_camera_device_monitor_add_devices, monitor);
  g_list_free (devices);
}

#else /* HAVE_UDEV */
void
cheese_camera_device_monitor_coldplug (CheeseCameraDeviceMonitor *monitor)
{
  #if 0
  CheeseCameraDeviceMonitorPrivate *priv = monitor->priv;
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
  CheeseCameraDeviceMonitorPrivate *priv = CHEESE_CAMERA_DEVICE_MONITOR (object)->priv;

  g_clear_object (&priv->client);
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
   * @monitor: the #CheeseCameraDeviceMonitor that emitted the signal
   * @device: a new #CheeseCameraDevice representing the video capture device
   *
   * The ::added signal is emitted when a camera is added, or on start-up
   * after cheese_camera_device_monitor_coldplug() is called.
   */
  monitor_signals[ADDED] = g_signal_new ("added", G_OBJECT_CLASS_TYPE (klass),
                                         G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                         G_STRUCT_OFFSET (CheeseCameraDeviceMonitorClass, added),
                                         NULL, NULL,
                                         g_cclosure_marshal_VOID__OBJECT,
                                         G_TYPE_NONE, 1, CHEESE_TYPE_CAMERA_DEVICE);

  /**
   * CheeseCameraDeviceMonitor::removed:
   * @monitor: the #CheeseCameraDeviceMonitor that emitted the signal
   * @uuid: UUID for the device on the system
   *
   * The ::removed signal is emitted when a camera is unplugged, or disabled on
   * the system.
   */
  monitor_signals[REMOVED] = g_signal_new ("removed", G_OBJECT_CLASS_TYPE (klass),
                                           G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                           G_STRUCT_OFFSET (CheeseCameraDeviceMonitorClass, removed),
                                           NULL, NULL,
                                           g_cclosure_marshal_VOID__STRING,
                                           G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void
cheese_camera_device_monitor_init (CheeseCameraDeviceMonitor *monitor)
{
#ifdef HAVE_UDEV
  CheeseCameraDeviceMonitorPrivate *priv = monitor->priv = cheese_camera_device_monitor_get_instance_private (monitor);
  const gchar *const subsystems[] = {"video4linux", NULL};

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
