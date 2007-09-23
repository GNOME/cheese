/*
 * Copyright (C) 2007 Copyright (C) 2007 daniel g. siegel <dgsiegel@gmail.com>
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

#include "cheese-config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libebook/e-book.h>

#include "cheese.h"
#include "cheese-command-handler.h"
#include "cheese-thumbnails.h"
#include "cheese-window.h"

#define MAX_HEIGHT 150
#define MAX_WIDTH  150

void
cheese_command_handler_init ()
{
}

void
cheese_command_handler_finalize ()
{
}

void
cheese_command_handler_url_show (GtkIconView *widget, GtkTreePath *path)
{
  gchar *file = cheese_thumbnails_get_filename_from_path (path);
  g_print ("opening file %s\n", file);
  file = g_strconcat ("file://", file, NULL);
  gnome_vfs_url_show (file);
  g_free (file);
}

void
cheese_command_handler_run_command_from_string (GtkMenuItem *widget, gpointer data)
{
  GError *error;
  error = NULL;
  gchar* command = data;
  printf("%s\n", command);

  g_print ("running custom command line: %s\n", command);
  if (!gdk_spawn_command_line_on_screen
      (gtk_widget_get_screen (cheese_window.window), command, &error))
  {
    g_warning ("cannot launch command line: %s\n", error->message);
    g_error_free (error);
  }

}

void
cheese_command_handler_move_to_trash (GtkIconView *widget, GtkTreePath *path, gpointer data)
{
  GnomeVFSURI *uri;
  GnomeVFSURI *trash_dir;
  GnomeVFSURI *trash_uri;
  gint result;
  char *name;
  GError *error = NULL;
  gchar *file = cheese_thumbnails_get_filename_from_path (path);

  result = show_move_to_trash_confirm_dialog (file);

  if (result != GTK_RESPONSE_OK)
    return;

  uri = gnome_vfs_uri_new (g_filename_to_uri (file, NULL, NULL));
  result = gnome_vfs_find_directory (uri,
      GNOME_VFS_DIRECTORY_KIND_TRASH,
      &trash_dir, FALSE, FALSE, 0777);

  if (result != GNOME_VFS_OK)
  {
    gnome_vfs_uri_unref (uri);
    char *header;
    GtkWidget *dlg;

    header = g_strdup_printf (_("Could not find the Trash"));

    dlg = gtk_message_dialog_new (GTK_WINDOW (cheese_window.window),
        GTK_DIALOG_MODAL |
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
        header);

    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg),
        error->message);

    gtk_dialog_run (GTK_DIALOG (dlg));

    gtk_widget_destroy (dlg);

    g_free (header);
    g_free (file);

    return;
  }

  name = gnome_vfs_uri_extract_short_name (uri);
  trash_uri = gnome_vfs_uri_append_file_name (trash_dir, name);
  g_free (name);

  result = gnome_vfs_move_uri (uri, trash_uri, TRUE);

  gnome_vfs_uri_unref (uri);
  gnome_vfs_uri_unref (trash_uri);
  gnome_vfs_uri_unref (trash_dir);

  if (result != GNOME_VFS_OK)
  {
    char *header;
    GtkWidget *dlg;

    header = g_strdup_printf (_("Error on deleting %s"),
        g_basename (file));

    dlg = gtk_message_dialog_new (GTK_WINDOW (cheese_window.window),
        GTK_DIALOG_MODAL |
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
        header);

    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg),
        error->message);

    gtk_dialog_run (GTK_DIALOG (dlg));

    gtk_widget_destroy (dlg);

    g_free (header);
  }
}

void
cheese_command_handler_about_me_update_photo (GtkWidget *widget, gpointer data)
{

  EContact  *contact;
  EBook     *book;
  GError *error;
  GdkPixbuf *pixbuf;
  gchar *filename = data;

  if (e_book_get_self (&contact, &book, NULL)) {
    gchar *name = e_contact_get (contact, E_CONTACT_FULL_NAME);
    printf("Setting Account Photo for %s\n", name);

    if (filename) {
      pixbuf = gdk_pixbuf_new_from_file_at_scale (filename, MAX_HEIGHT, MAX_WIDTH, TRUE, NULL);

      if (contact) {
        EContactPhoto new_photo;
        guchar **data;
#if LIBEBOOK_VERSION_1_12
        gsize *length;
#else
        int *length;
#endif
        new_photo.type = E_CONTACT_PHOTO_TYPE_INLINED;
        new_photo.data.inlined.mime_type = "image/jpeg";

        data = &new_photo.data.inlined.data;
        length = &new_photo.data.inlined.length;

        if (gdk_pixbuf_save_to_buffer (pixbuf,
                                      (gchar **) data, (gsize *) length,
                                      "png", NULL,
                                      "compression", "9", NULL)) {

          e_contact_set (contact, E_CONTACT_PHOTO, &new_photo);

          if (!e_book_commit_contact(book, contact, &error)) {

            char *header;
            GtkWidget *dlg;

            header = g_strdup_printf (_("Could not set the Account Photo"));

            dlg = gtk_message_dialog_new (GTK_WINDOW (cheese_window.window),
                GTK_DIALOG_MODAL |
                GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                header);

            gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg),
                error->message);

            gtk_dialog_run (GTK_DIALOG (dlg));

            gtk_widget_destroy (dlg);

            g_free (header);
          }

          g_free (*data);

        }
      }
    }
  }
}

