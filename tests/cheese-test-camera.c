
#include "config.h"

#include <stdlib.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include "cheese-camera.h"
#include "cheese.h"

static gboolean
delete_callback (GtkWidget *window,
                 GdkEvent  *event,
                 gpointer   data)
{
  gtk_widget_destroy (window);
  gtk_main_quit ();
  return FALSE;
}

static gboolean
time_cb (CheeseCamera *camera)
{
  g_print ("Cheese! filename: testcamera.jpg\n");
  cheese_camera_take_photo (camera, "testcamera.jpg");
  return FALSE;
}

int
main (int argc, char **argv)
{
  GtkWidget *window;
  GtkWidget *screen;
  CheeseCamera *camera;

  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  if (!cheese_init (&argc, &argv))
    return EXIT_FAILURE;
  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (window), 400, 300);
  g_signal_connect (G_OBJECT (window), "delete-event",
                    G_CALLBACK (delete_callback), NULL);

  camera = cheese_camera_new (NULL, 640, 480);
  cheese_camera_setup (camera, NULL, NULL);

  g_object_get (G_OBJECT (camera), "widget", &screen, NULL);
  gtk_widget_show (screen);
  gtk_container_add (GTK_CONTAINER (window), screen);

  gtk_widget_show_all (window);

  cheese_camera_play (camera);

  g_timeout_add_seconds (5, (GSourceFunc) (time_cb), camera);

  gtk_main ();

  return EXIT_SUCCESS;
}
