
#include "cheese-config.h"

#include <stdlib.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include <clutter-gtk/clutter-gtk.h>
#include "cheese-avatar-chooser.h"
#include "cheese-gtk.h"

static void
response_cb (GtkDialog           *dialog,
             int                  response,
             CheeseAvatarChooser *chooser)
{
  if (response == GTK_RESPONSE_ACCEPT)
  {
    GdkPixbuf *pixbuf;
    g_message ("got pixbuf captured");
    g_object_get (G_OBJECT (chooser), "pixbuf", &pixbuf, NULL);
  }

  gtk_main_quit ();
}

int
main (int argc, char **argv)
{
  GtkWidget *window;

  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  if (!cheese_gtk_init (&argc, &argv))
    return EXIT_FAILURE;

  window = cheese_avatar_chooser_new ();
  g_signal_connect (G_OBJECT (window), "response",
                    G_CALLBACK (response_cb), window);

  gtk_widget_show_all (window);

  gtk_main ();

  gtk_widget_destroy (window);

  return EXIT_SUCCESS;
}
