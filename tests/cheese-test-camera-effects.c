#include "config.h"

#include <stdlib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include "cheese.h"
#include "cheese-camera.h"
#include "cheese-effect.h"

static gboolean
delete_callback (GtkWidget *window,
                 GdkEvent  *event,
                 gpointer   data)
{
  gtk_widget_destroy (window);
  gtk_main_quit ();
  return FALSE;
}

int
main (int argc, char **argv)
{
  GtkWidget *window;
  CheeseCamera *camera;
  GtkWidget *box, *video_box;
  GtkWidget *widget;
  CheeseEffect *effect;
  g_autoptr(GError) error = NULL;

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

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_add (GTK_CONTAINER (window), box);

  camera = cheese_camera_new ("/dev/video0", 640, 480);
  if (!cheese_camera_setup (camera, NULL, &error)) {
    g_warning ("Could not setup camera: %s", error->message);
    return EXIT_FAILURE;
  }
  g_object_get (G_OBJECT (camera), "widget", &widget, NULL);
  gtk_box_pack_start (GTK_BOX (box), widget, TRUE, TRUE, 12);
#if 0
  video_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (box), video_box, TRUE, TRUE, 12);
  effect = cheese_effect_new ("Kaleidoscope Test", "kaleidoscope");
  cheese_camera_connect_effect_texture (camera, effect, GTK_CONTAINER (video_box));
#endif
  cheese_camera_play (camera);
  gtk_widget_show_all (window);

  gtk_main ();

  return EXIT_SUCCESS;
}
