#include <glib.h>
#include <stdlib.h>

#include "cheese-camera-device-monitor.h"
#include "cheese-camera-device.h"
#include "cheese.h"

static void
added_cb (CheeseCameraDeviceMonitor *monitor,
	  CheeseCameraDevice        *device,
	  gpointer                   user_data)
{
  g_message ("Added new device with ID '%s'", cheese_camera_device_get_uuid (device));
  g_object_unref (device);
}

static void
removed_cb (CheeseCameraDeviceMonitor *monitor,
            const gchar               *uuid,
            gpointer                   user_data)
{
  g_message ("Removed device with ID '%s'", uuid);
}

int
main (int argc, char **argv)
{
  CheeseCameraDeviceMonitor *monitor;
  GMainLoop *mainloop;

  if (!cheese_init (&argc, &argv))
    return EXIT_FAILURE;

  monitor = cheese_camera_device_monitor_new ();
  g_signal_connect (G_OBJECT (monitor), "added",
                    G_CALLBACK (added_cb), NULL);
  g_signal_connect (G_OBJECT (monitor), "removed",
                    G_CALLBACK (removed_cb), NULL);
  cheese_camera_device_monitor_coldplug (monitor);

  mainloop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (mainloop);
  g_main_loop_unref (mainloop);

  return EXIT_SUCCESS;
}
