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

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs.h>

#include "cheese.h"
#include "cheese-command-handler.h"
#include "cheese-thumbnails.h"
#include "cheese-window.h"

void
cheese_command_handler_init() {
}

void
cheese_command_handler_finalize() {
}

void
cheese_command_handler_url_show(GtkWidget *widget, GtkTreePath *path) {
  gchar *file = cheese_thumbnails_get_filename_from_path(path);
  g_print("opening file %s\n", file);
  file = g_strconcat ("file://", file, NULL);
  gnome_vfs_url_show(file);
}

void
cheese_command_handler_run_command_from_string(GtkWidget *widget, gchar *data) {
	GError       *error;
	error = NULL;

  g_print("running custom command line: %s\n", data);
	if (!gdk_spawn_command_line_on_screen(gtk_widget_get_screen(cheese_window.window),
					       data, &error)) {
		g_warning("cannot launch command line: %s\n", error->message);
		g_error_free (error);
	}

}

void
cheese_command_handler_move_to_trash(GtkWidget *widget, gchar *file)
{
  GnomeVFSURI *uri;
  GnomeVFSURI *trash_dir;
  GnomeVFSURI *trash_uri;
  gint result;
  char *name;
  GError *error = NULL;

  result = show_move_to_trash_confirm_dialog(file);

  if (result != GTK_RESPONSE_OK)
    return;

  printf("DELETIN\n");

  uri = gnome_vfs_uri_new(g_filename_to_uri(file, NULL, NULL));
  result = gnome_vfs_find_directory(uri,
      GNOME_VFS_DIRECTORY_KIND_TRASH,
      &trash_dir,
      FALSE,
      FALSE,
      0777);

  if (result != GNOME_VFS_OK) {
    gnome_vfs_uri_unref (uri);
    char *header;
    GtkWidget *dlg;

    header = g_strdup_printf (_("Could not find the Trash"));

    dlg = gtk_message_dialog_new (GTK_WINDOW (cheese_window.window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK,
        header);

    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg),
        error->message);

    gtk_dialog_run (GTK_DIALOG (dlg));

    gtk_widget_destroy (dlg);

    g_free (header);

    return;
  }

  name = gnome_vfs_uri_extract_short_name(uri);
  trash_uri = gnome_vfs_uri_append_file_name(trash_dir, name);
  g_free(name);

  result = gnome_vfs_move_uri(uri, trash_uri, TRUE);

  gnome_vfs_uri_unref (uri);
  gnome_vfs_uri_unref (trash_uri);
  gnome_vfs_uri_unref (trash_dir);

  if (result != GNOME_VFS_OK) {
    char *header;
    GtkWidget *dlg;

    header = g_strdup_printf (_("Error on deleting image %s"), 
        g_basename (file));

    dlg = gtk_message_dialog_new (GTK_WINDOW (cheese_window.window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK,
        header);

    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg),
        error->message);

    gtk_dialog_run (GTK_DIALOG (dlg));

    gtk_widget_destroy (dlg);

    g_free (header);
  }

}

