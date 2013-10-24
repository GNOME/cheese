
#include "cheese-config.h"

#include <stdlib.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <clutter-gtk/clutter-gtk.h>
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
  ClutterActor *stage;
  ClutterActor *texture;

  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  if (gtk_clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
    return EXIT_FAILURE;

  if (!cheese_init (&argc, &argv))
    return EXIT_FAILURE;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (window), 400, 300);
  g_signal_connect (G_OBJECT (window), "delete-event",
                    G_CALLBACK (delete_callback), NULL);

  screen = gtk_clutter_embed_new ();
  stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (screen));
  texture = clutter_texture_new ();

  clutter_actor_set_size (texture, 400, 300);
  clutter_actor_add_child (stage, texture);

  gtk_widget_show (screen);
  clutter_actor_show (texture);

  camera = cheese_camera_new (CLUTTER_TEXTURE (texture), NULL, 640, 480);

  cheese_camera_setup (camera, NULL, NULL);

  gtk_container_add (GTK_CONTAINER (window), screen);

  gtk_widget_show_all (window);

  cheese_camera_play (camera);

  g_timeout_add_seconds (5, (GSourceFunc) (time_cb), camera);

  gtk_main ();

  return EXIT_SUCCESS;
}
