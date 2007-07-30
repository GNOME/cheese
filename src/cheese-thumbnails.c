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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <libgnomevfs/gnome-vfs.h>
#include <glib/gprintf.h>

#include "cheese.h"
#include "cheese-fileutil.h"
#include "cheese-thumbnails.h"
#include "cheese-window.h"

enum {
  PIXBUF_COLUMN,
  URL_COLUMN
};

struct _thumbnails thumbnails;

void
cheese_thumbnails_init() {
  thumbnails.store = gtk_list_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
}

void
cheese_thumbnails_finalize() {
}

void
cheese_thumbnails_append_photo(gchar *filename) {
  g_printf("appending %s to thumbnail row\n", filename);
  gtk_list_store_append(thumbnails.store, &thumbnails.iter);
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_size(filename, THUMB_WIDTH, THUMB_HEIGHT, NULL);
  if (!pixbuf) {
    g_warning("could not load %s\n", filename);
    return;
  }
  gtk_list_store_set(thumbnails.store, &thumbnails.iter, PIXBUF_COLUMN, pixbuf, URL_COLUMN, filename, -1);

  GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(thumbnails.store),&thumbnails.iter);
  gtk_icon_view_scroll_to_path(GTK_ICON_VIEW(thumbnails.iconview), path,
                                  TRUE, 1.0, 0.5);

  g_object_unref(pixbuf);
}

void
cheese_thumbnails_remove_photo(gchar *filename) {

  //gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, gtk_tree_path_new_from_string(path));
  //gtk_list_store_remove(store, &iter);
  //if (is_file_in_list_store(store, info_uri, &iter)) {
  //    gtk_list_store_remove(GTK_LIST_STORE(store), &iter);
}

void
cheese_thumbnails_fill_thumbs()
{
  GDir *dir;
  gchar *path;
  const gchar *name;
  gboolean is_dir;
  
  gtk_list_store_clear(thumbnails.store);

  dir = g_dir_open(get_cheese_path(), 0, NULL);
  if (!dir)
    return;

  name = g_dir_read_name(dir);
  while (name != NULL) {
    if (name[0] != '.') {
      if (!g_str_has_suffix (name, PHOTO_NAME_SUFFIX_DEFAULT))
        continue;
      path = g_build_filename(cheese_fileutil_get_photo_path(), name, NULL);

      is_dir = g_file_test(path, G_FILE_TEST_IS_DIR);

      if (!is_dir)
        cheese_thumbnails_append_photo(path);
      g_free(path);
    }
    name = g_dir_read_name(dir);      
  }
  g_free(dir);
}
