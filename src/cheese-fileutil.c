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
#include <stdlib.h>

#include <libgnomevfs/gnome-vfs.h>

#include "cheese.h"
#include "cheese-thumbnails.h"

void
cheese_fileutil_init() {
}

void
cheese_fileutil_finalize() {
}

gchar *
cheese_fileutil_get_photo_path() {
  //FIXME: check for real path
  // maybe ~/cheese or on the desktop..
  //g_get_home_dir()
  //gchar *path = g_strdup_printf("%s/%s", getenv("PWD"), PHOTO_FOLDER_DEFAULT);
  gchar *path = g_strdup_printf("%s/.gnome2/cheese/%s", g_get_home_dir(), PHOTO_FOLDER_DEFAULT);
  return path;
}

gchar *
cheese_fileutil_get_photo_filename(int i) {
  gchar *filename;
  if (i < 0)
    i *= -1;
  if (i < 10)
    filename = g_strdup_printf("%s%s0%d%s", cheese_fileutil_get_photo_path(), PHOTO_NAME_DEFAULT, i, PHOTO_NAME_SUFFIX_DEFAULT);
  else
    filename = g_strdup_printf("%s%s%d%s", cheese_fileutil_get_photo_path(), PHOTO_NAME_DEFAULT, i, PHOTO_NAME_SUFFIX_DEFAULT);

  return filename;
}

gchar *
cheese_fileutil_get_video_path() {
  //FIXME: check for real path
  // maybe ~/cheese or on the desktop..
  //gchar *path = g_strdup_printf("%s/%s", getenv("PWD"), VIDEO_FOLDER_DEFAULT);
  gchar *path = g_strdup_printf("%s/.gnome2/cheese/%s", g_get_home_dir(), VIDEO_FOLDER_DEFAULT);
  return path;
}

gchar *
cheese_fileutil_get_video_filename() {
  gchar *filename;
  int i;
  GnomeVFSURI *uri;

  i = 1;
  filename = g_strdup_printf("%s%s0%d%s", cheese_fileutil_get_video_path(), VIDEO_NAME_DEFAULT, i, VIDEO_NAME_SUFFIX_DEFAULT);

  uri = gnome_vfs_uri_new(filename);

  while (gnome_vfs_uri_exists(uri)) {
    i++;
    if (i < 10)
      filename = g_strdup_printf("%s%s0%d%s", cheese_fileutil_get_video_path(), VIDEO_NAME_DEFAULT, i, VIDEO_NAME_SUFFIX_DEFAULT);
    else
      filename = g_strdup_printf("%s%s%d%s", cheese_fileutil_get_video_path(), VIDEO_NAME_DEFAULT, i, VIDEO_NAME_SUFFIX_DEFAULT);
    g_free(uri);
    uri = gnome_vfs_uri_new(filename);
  }
  g_free(uri);

  return filename;
}

void
cheese_fileutil_monitor_cb(GnomeVFSMonitorHandle *monitor_handle, const gchar *monitor_uri, const gchar *info_uri, GnomeVFSMonitorEventType event_type)
{
  gchar *filename = gnome_vfs_get_local_path_from_uri(info_uri);
  gboolean is_dir;

  is_dir = g_file_test(filename, G_FILE_TEST_IS_DIR);

  if (!is_dir) {
    switch (event_type) {
      case GNOME_VFS_MONITOR_EVENT_DELETED:
        cheese_thumbnails_remove_item(filename);
        break;
      // in case if we need to check if a file changed
      //case GNOME_VFS_MONITOR_EVENT_CHANGED:
      case GNOME_VFS_MONITOR_EVENT_CREATED:
        if (!g_str_has_suffix(filename, VIDEO_NAME_SUFFIX_DEFAULT)) {
          g_message("new file found: %s\n", filename);
          cheese_thumbnails_append_item(filename);
        }
        break;
      default:
        break;
    }
  }
  g_free(filename);
}
