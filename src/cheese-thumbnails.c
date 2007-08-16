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

#include <libgnomevfs/gnome-vfs.h>
#include <glib/gprintf.h>
#include <string.h>
#include <libgnomeui/libgnomeui.h>

#include "cheese.h"
#include "cheese-fileutil.h"
#include "cheese-thumbnails.h"
#include "cheese-window.h"
#include "eog-thumb-shadow.h"

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
cheese_thumbnails_append_item(gchar *filename) {
  GdkPixbuf *pixbuf = NULL;

  if (g_str_has_suffix(filename, PHOTO_NAME_SUFFIX_DEFAULT)) {
    pixbuf = gdk_pixbuf_new_from_file_at_size(filename, THUMB_WIDTH, THUMB_HEIGHT, NULL);

  } else if (g_str_has_suffix(filename, VIDEO_NAME_SUFFIX_DEFAULT)) {
    GnomeThumbnailFactory *factory;
    GnomeVFSFileInfo *file_info;
    gchar *uri;
    gchar *thumb_loc;

    file_info = gnome_vfs_file_info_new();
    uri = g_filename_to_uri(filename, NULL, NULL);
    if (!uri ||
        (gnome_vfs_get_file_info(uri, file_info,
            GNOME_VFS_FILE_INFO_DEFAULT) != GNOME_VFS_OK)) {
      g_printerr ("Invalid filename\n");
      return;
    }

    factory = gnome_thumbnail_factory_new(GNOME_THUMBNAIL_SIZE_NORMAL);

    thumb_loc = gnome_thumbnail_factory_lookup(factory, uri, file_info->mtime);
    //g_print("file: %s, time: %s, icon: %s\n", uri, ctime(&file_info->mtime), thumb_loc);

    if (!thumb_loc) {
      g_print("creating thumbnail for %s\n", filename);
      pixbuf = gnome_thumbnail_factory_generate_thumbnail(factory, uri, "video/x-theora+ogg");
      if (!pixbuf) {
        g_warning("could not load %s\n", filename);
        return;
      }
      gnome_thumbnail_factory_save_thumbnail(factory, pixbuf, uri, file_info->mtime);
    } else {
      pixbuf = gdk_pixbuf_new_from_file(thumb_loc, NULL);
    }
  }

  if (!pixbuf) {
    g_warning("could not load %s\n", filename);
    return;
  }

  eog_thumb_shadow_add_shadow(&pixbuf);
  eog_thumb_shadow_add_round_border(&pixbuf);

  g_print("appending %s to thumbnail row\n", filename);
  gtk_list_store_append(thumbnails.store, &thumbnails.iter);
  gtk_list_store_set(thumbnails.store, &thumbnails.iter, PIXBUF_COLUMN, pixbuf, URL_COLUMN, filename, -1);

  GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(thumbnails.store), &thumbnails.iter);
  gtk_icon_view_scroll_to_path(GTK_ICON_VIEW(thumbnails.iconview), path, TRUE, 1.0, 0.5);

  g_object_unref(pixbuf);
}

void
cheese_thumbnails_remove_item(gchar *filename) {

  gchar *path;
  GtkTreeIter i;

  gtk_tree_model_get_iter_first(GTK_TREE_MODEL(thumbnails.store), &i);
  while (gtk_tree_model_iter_next(GTK_TREE_MODEL(thumbnails.store), &i)) {
    gtk_tree_model_get(GTK_TREE_MODEL(thumbnails.store), &i, URL_COLUMN, &path, -1);
    if (!g_ascii_strcasecmp(path, filename))
      break;
  }
  gtk_list_store_remove(thumbnails.store, &i);
  g_print("removing %s from thumbnail row\n", filename);
}

gchar *
cheese_thumbnails_get_filename_from_path(GtkTreePath *path) {
  GtkTreeIter iter;
  gchar *file;
  gtk_tree_model_get_iter(GTK_TREE_MODEL(thumbnails.store), &iter, path);
  gtk_tree_model_get(GTK_TREE_MODEL(thumbnails.store), &iter, 1, &file, -1);
  return file;
}

void
cheese_thumbnails_fill_thumbs()
{
  GDir *dir;
  gchar *path;
  const gchar *name;
  gboolean is_dir;
  GList *filelist = NULL;
  
  gtk_list_store_clear(thumbnails.store);

  dir = g_dir_open(cheese_fileutil_get_photo_path(), 0, NULL);
  if (!dir)
    return;

  while ((name = g_dir_read_name(dir))) {
    if (name[0] != '.') {
      if (!g_str_has_suffix(name, PHOTO_NAME_SUFFIX_DEFAULT))
        continue;

      path = g_build_filename(cheese_fileutil_get_photo_path(), name, NULL);
      is_dir = g_file_test(path, G_FILE_TEST_IS_DIR);

      if (!is_dir)
        filelist = g_list_prepend(filelist, g_strdup(path));
      g_free(path);
    }
  }
  dir = g_dir_open(cheese_fileutil_get_video_path(), 0, NULL);
  if (!dir)
    return;

  while ((name = g_dir_read_name(dir))) {
    if (name[0] != '.') {
      if (!g_str_has_suffix(name, VIDEO_NAME_SUFFIX_DEFAULT))
        continue;

      path = g_build_filename(cheese_fileutil_get_video_path(), name, NULL);
      is_dir = g_file_test(path, G_FILE_TEST_IS_DIR);

      if (!is_dir)
        filelist = g_list_prepend(filelist, g_strdup(path));
      g_free(path);
    }
  }
  filelist = g_list_sort (filelist, (GCompareFunc)strcmp);
  g_list_foreach (filelist, (GFunc)cheese_thumbnails_append_item, NULL);

  g_free(dir);
}
