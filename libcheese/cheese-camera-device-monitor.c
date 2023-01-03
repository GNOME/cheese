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
  #include <config.h>
#endif

#include <glib-object.h>
#include <string.h>

#include "cheese-camera-device-monitor.h"

/**
 * SECTION:cheese-camera-device-monitor
 * @short_description: Simple object to enumerate video devices
 * @stability: Unstable
 * @include: cheese/cheese-camera-device-monitor.h
 *
 * #CheeseCameraDeviceMonitor provides a basic interface for video device
 * enumeration and hotplugging.
 *
 * It uses GstDeviceMonitor to list video devices. It is also capable to
 * monitor device plugging and emit a CheeseCameraDeviceMonitor::added or
 * CheeseCameraDeviceMonitor::removed signal when an event happens.
 */

struct _CheeseCameraDeviceMonitorPrivate
{
  GstDeviceMonitor *monitor;
};

static void initable_iface_init       (GInitableIface      *initable_iface);
static void async_initable_iface_init (GAsyncInitableIface *async_initable_iface);

G_DEFINE_TYPE_WITH_CODE (CheeseCameraDeviceMonitor, cheese_camera_device_monitor,
                         G_TYPE_OBJECT,
                         G_ADD_PRIVATE (CheeseCameraDeviceMonitor)
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE, async_initable_iface_init));

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

/*
 * cheese_camera_device_monitor_set_up_device:
 * @device: the device information from GStreamer
 *
 * Creates a new #CheeseCameraDevice for the supplied @device.
 *
 * Returns: a new #CheeseCameraDevice, or %NULL if @device was not an
 * acceptable capture device
 */
static CheeseCameraDevice*
cheese_camera_device_monitor_set_up_device (GstDevice *device)
{
  CheeseCameraDevice *newdev;
  GError *error = NULL;

  newdev = cheese_camera_device_new (device, &error);

  if (newdev == NULL)
    GST_WARNING ("Device initialization for %p failed: %s ",
                 device,
                 (error != NULL) ? error->message : "Unknown reason");

  return newdev;
}

/*
 * cheese_camera_device_monitor_added:
 * @monitor: a #CheeseCameraDeviceMonitor
 * @device: the device information, from GStreamer, for the device that was added
 *
 * Emits the ::added signal.
 */
static void
cheese_camera_device_monitor_added (CheeseCameraDeviceMonitor *monitor,
                                    GstDevice                 *device)
{
  CheeseCameraDevice *olddev;
  CheeseCameraDevice *newdev;

  olddev = g_object_get_data (G_OBJECT (device), "cheese-camera-device");
  if (olddev) {
      GST_DEBUG ("Ignoring duplicate device %" GST_PTR_FORMAT, device);
      return;
  }

  newdev = cheese_camera_device_monitor_set_up_device (device);
  /* Ignore non-video devices, GNOME bug #677544. */
  if (newdev) {
    g_object_set_data (G_OBJECT (device), "cheese-camera-device", newdev);
    g_signal_emit (monitor, monitor_signals[ADDED], 0, newdev);
  }
}

/*
 * cheese_camera_device_monitor_removed:
 * @monitor: a #CheeseCameraDeviceMonitor
 * @device: the device information, from GStreamer, for the device that was removed
 *
 * Emits the ::removed signal.
 */
static void
cheese_camera_device_monitor_removed (CheeseCameraDeviceMonitor *monitor,
                                      GstDevice                 *device)
{
  CheeseCameraDevice *olddev;

  olddev = g_object_get_data (G_OBJECT (device), "cheese-camera-device");
  if (olddev)
    g_signal_emit (monitor, monitor_signals[REMOVED], 0, olddev);
}

/*
 * cheese_camera_device_monitor_bus_func:
 * @bus: a #GstBus
 * @message: the message posted on the bus
 *
 * Check if the message corresponds to device addition or removal, and if so,
 * pass it on to cheese_camera_device_monitor_added() or
 * cheese_camera_device_monitor_removed() for emitting the ::added and
 * ::removed signals.
 */
static gboolean
cheese_camera_device_monitor_bus_func (GstBus     *bus,
                                       GstMessage *message,
                                       gpointer user_data)
{
  CheeseCameraDeviceMonitor *monitor = user_data;
  GstDevice *device;

  switch (GST_MESSAGE_TYPE (message))
  {
    case GST_MESSAGE_DEVICE_ADDED:
      gst_message_parse_device_added (message, &device);
      cheese_camera_device_monitor_added (monitor, device);
      break;
    case GST_MESSAGE_DEVICE_REMOVED:
      gst_message_parse_device_removed (message, &device);
      cheese_camera_device_monitor_removed (monitor, device);
      break;
    default:
      break;
  }
  return G_SOURCE_CONTINUE;
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
    (GstDevice *) data);
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
    CheeseCameraDeviceMonitorPrivate *priv;
  GList *devices;

    g_return_if_fail (CHEESE_IS_CAMERA_DEVICE_MONITOR (monitor));

    priv = cheese_camera_device_monitor_get_instance_private (monitor);

    g_return_if_fail (priv->monitor != NULL);

  GST_INFO ("Probing devices with GStreamer monitor...");

  devices = gst_device_monitor_get_devices (priv->monitor);

  if (devices == NULL) GST_WARNING ("No device found");

  /* Initialize camera structures */
  g_list_foreach (devices, cheese_camera_device_monitor_add_devices, monitor);
  g_list_free (devices);
}

static void
cheese_camera_device_monitor_finalize (GObject *object)
{
    CheeseCameraDeviceMonitorPrivate *priv;

    priv = cheese_camera_device_monitor_get_instance_private (CHEESE_CAMERA_DEVICE_MONITOR (object));

  gst_device_monitor_stop (priv->monitor);
  g_clear_object (&priv->monitor);

  G_OBJECT_CLASS (cheese_camera_device_monitor_parent_class)->finalize (object);
}

static void
cheese_camera_device_monitor_class_init (CheeseCameraDeviceMonitorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

#ifndef GST_DISABLE_GST_DEBUG
  if (cheese_device_monitor_cat == NULL)
    GST_DEBUG_CATEGORY_INIT (cheese_device_monitor_cat,
                             "cheese-device-monitor",
                             0, "Cheese Camera Device Monitor");
#endif

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
   * @device: the #CheeseCameraDevice that was removed
   *
   * The ::removed signal is emitted when a camera is unplugged, or disabled on
   * the system.
   */
  monitor_signals[REMOVED] = g_signal_new ("removed", G_OBJECT_CLASS_TYPE (klass),
                                           G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                           G_STRUCT_OFFSET (CheeseCameraDeviceMonitorClass, removed),
                                           NULL, NULL,
                                           g_cclosure_marshal_VOID__STRING,
                                           G_TYPE_NONE, 1, CHEESE_TYPE_CAMERA_DEVICE);
}

static gboolean
initable_init (GInitable     *initable,
               GCancellable  *cancellable,
               GError       **error)
{
  CheeseCameraDeviceMonitor *monitor = CHEESE_CAMERA_DEVICE_MONITOR (initable);
  CheeseCameraDeviceMonitorPrivate *priv = cheese_camera_device_monitor_get_instance_private (monitor);
  GstBus *bus;
  GstCaps *caps;

  priv->monitor = gst_device_monitor_new ();

  bus = gst_device_monitor_get_bus (priv->monitor);
  gst_bus_add_watch (bus, cheese_camera_device_monitor_bus_func, monitor);
  gst_object_unref (bus);

  caps = cheese_camera_device_supported_format_caps ();
  gst_device_monitor_add_filter (priv->monitor, "Video/Source", caps);
  gst_caps_unref (caps);

  gst_device_monitor_start (priv->monitor);

  return TRUE;
}

static void
initable_iface_init (GInitableIface *initable_iface)
{
  initable_iface->init = initable_init;
}

static void
async_initable_iface_init (GAsyncInitableIface *async_initable_iface)
{
  /* Run GInitable code in thread. */
}

static void
cheese_camera_device_monitor_init (CheeseCameraDeviceMonitor *monitor)
{
  /* Let GInitable initialize it. */
}

/**
 * cheese_camera_device_monitor_new:
 *
 * Returns a new #CheeseCameraDeviceMonitor object. The initialization may block.
 * See cheese_camera_device_monitor_new_async() for the asynchronous version.
 *
 * Return value: a new #CheeseCameraDeviceMonitor object.
 **/
CheeseCameraDeviceMonitor *
cheese_camera_device_monitor_new (void)
{
  return g_initable_new (CHEESE_TYPE_CAMERA_DEVICE_MONITOR, NULL, NULL, NULL);
}

/**
 * cheese_camera_device_monitor_new_async:
 * @cancellable: a #GCancellable or NULL
 * @callback: a GAsyncReadyCallback to call when the initialization is finished
 * @user_data: user data to pass to callback
 *
 * Creates a new #CheeseCameraDeviceMonitor object asynchronously. Callback
 * will be called when it is done. Use cheese_camera_device_monitor_new_finish()
 * to get the result.
 *
 * See cheese_camera_device_monitor_new() for the synchronous version.
 **/
void
cheese_camera_device_monitor_new_async (GCancellable *cancellable,
                                        GAsyncReadyCallback callback,
                                        gpointer user_data)
{
  g_async_initable_new_async (CHEESE_TYPE_CAMERA_DEVICE_MONITOR, 0, cancellable, callback, user_data, NULL);
}

/**
 * cheese_camera_device_monitor_new_finish:
 * @result: the GAsyncResult from the callback
 * @error: return location for errors, or NULL to ignore
 *
 * Finishes creating a new #CheeseCameraDeviceMonitor object.
 *
 * Return value: a new #CheeseCameraDeviceMonitor object or NULL if error is set.
 **/
CheeseCameraDeviceMonitor *
cheese_camera_device_monitor_new_finish (GAsyncResult *result,
                                         GError      **error)
{
  GObject *ret;
  GObject *source;

  source = g_async_result_get_source_object (result);
  ret = g_async_initable_new_finish (G_ASYNC_INITABLE (source), result, error);
  g_object_unref (source);

  if (ret != NULL)
    return CHEESE_CAMERA_DEVICE_MONITOR (ret);
  else
    return NULL;
}
