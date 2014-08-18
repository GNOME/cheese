/*
 *  GNOME Camera - Access camera devices on a system via D-Bus
 *  Copyright (C) 2014  Red Hat, Inc.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "camera.h"

/* FIXME: Error checking. Global variables. Memory leaks. */

GcManager *manager_proxy;
GcClient *client_proxy;

static void
on_acquire_client_clicked (GtkButton *acquire_client,
                           GtkButton *show_chooser)
{
    GError *error = NULL;
    gchar *client_path;

    if (!gc_manager_call_get_client_sync (manager_proxy, &client_path, NULL,
                                          &error))
    {
        g_error ("error getting client from manager: %s", error->message);
    }

    client_proxy = gc_client_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                     G_DBUS_PROXY_FLAGS_NONE,
                                                     "org.gnome.Camera",
                                                     client_path,
                                                     NULL,
                                                     NULL);

    gtk_widget_set_sensitive (GTK_WIDGET (show_chooser), TRUE);
}

static void
on_client_proxy_user_done (GcClient *client,
                           gboolean photo_taken,
                           const gchar *image_data,
                           GtkImage *image)
{
    if (photo_taken)
    {
        guchar *buffer;
        gsize length;
        GdkPixbufLoader *loader;
        GError *error = NULL;

        buffer = g_base64_decode (image_data, &length);

        loader = gdk_pixbuf_loader_new_with_type ("png", NULL);

        if (!gdk_pixbuf_loader_write (loader, buffer, length, &error))
        {
            g_error ("Error loading image data: %s", error->message);
        }

        g_free (buffer);

        if (gdk_pixbuf_loader_close (loader, NULL))
        {
            gtk_image_set_from_pixbuf (image,
                                       gdk_pixbuf_loader_get_pixbuf (loader));
        }
        else
        {
            g_error ("%s", "error deserializing image data");
        }

        g_object_unref (loader);
    }
    else
    {
        g_debug ("%s", "User did not accept the image");
    }
}

static void
on_show_chooser_clicked (GtkButton *show_chooser,
                         GtkImage *image)
{
    GError *error = NULL;

    g_signal_connect (client_proxy, "user-done",
                      G_CALLBACK (on_client_proxy_user_done), image);

    if (!gc_client_call_show_chooser_sync (client_proxy, NULL, &error))
    {
        g_error ("error showing chooser: %s", error->message);
    }
}

static GtkWidget *
create_window (void)
{
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *acquire_client;
    GtkWidget *show_chooser;
    GtkWidget *image;

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    grid = gtk_grid_new ();

    gtk_orientable_set_orientation (GTK_ORIENTABLE (grid),
                                    GTK_ORIENTATION_VERTICAL);

    acquire_client = gtk_button_new_with_label ("Acquire camera client");
    gtk_container_add (GTK_CONTAINER (grid), acquire_client);

    show_chooser = gtk_button_new_with_label ("Show chooser dialog");
    gtk_widget_set_sensitive (show_chooser, FALSE);
    gtk_container_add (GTK_CONTAINER (grid), show_chooser);

    image = gtk_image_new_from_icon_name ("insert-image",
                                          GTK_ICON_SIZE_DIALOG);
    gtk_container_add (GTK_CONTAINER (grid), image);

    g_signal_connect (acquire_client, "clicked",
                      G_CALLBACK (on_acquire_client_clicked), show_chooser);
    g_signal_connect (show_chooser, "clicked",
                      G_CALLBACK (on_show_chooser_clicked), image);

    gtk_container_add (GTK_CONTAINER (window), grid);

    return window;
}

static void
setup_manager_proxy (void)
{
    GError *error = NULL;

    manager_proxy = gc_manager_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                       G_DBUS_PROXY_FLAGS_NONE,
                                                       "org.gnome.Camera",
                                                       "/org/gnome/Camera/Manager",
                                                       NULL,
                                                       NULL);

    if (error)
    {
        g_error ("error creating manager proxy: %s", error->message);
    }
}

int
main (int argc,
      char *argv[])
{
    GtkWidget *window;

    gtk_init (&argc, &argv);

    window = create_window ();
    setup_manager_proxy ();

    gtk_widget_show_all (window);

    gtk_main ();

    return 0;
}
