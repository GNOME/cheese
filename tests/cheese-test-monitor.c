#include <glib.h>

#include "cheese-camera-device-monitor.h"

static void
added_cb (CheeseCameraDeviceMonitor *monitor,
          CheeseCameraDevice        *device)
{
  gchar *id;

  g_object_get (device, "device-id", &id, NULL);
  g_message ("Added new device with ID '%s'", id);
  g_free (id);
}

static void
removed_cb (CheeseCameraDeviceMonitor *monitor,
            const char                *id)
{
  g_message ("Removed device with ID '%s'", id);
}

int
main (int argc, char **argv)
{
  CheeseCameraDeviceMonitor *monitor;

  gst_init (&argc, &argv);

  monitor = cheese_camera_device_monitor_new ();
  g_signal_connect (G_OBJECT (monitor), "added",
                    G_CALLBACK (added_cb), NULL);
  g_signal_connect (G_OBJECT (monitor), "removed",
                    G_CALLBACK (removed_cb), NULL);
  cheese_camera_device_monitor_coldplug (monitor);

  g_main_loop_run (g_main_loop_new (NULL, FALSE));

  return 0;
}
