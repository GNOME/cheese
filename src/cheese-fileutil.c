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

#include <glib.h>
#include <stdlib.h>

#include <libgnomevfs/gnome-vfs.h>

#include "cheese.h"
#include "cheese-thumbnails.h"

gchar *
get_cheese_path() {
  //FIXME: check for real path
  // maybe ~/cheese or on the desktop..
  //g_get_home_dir()
  gchar *path = g_strdup_printf("%s/%s", getenv("PWD"), SAVE_FOLDER_DEFAULT);
  return path;
}

gchar *
get_cheese_filename(int i) {
  gchar *filename;
  if (i < 0)
    i *= -1;
  if (i < 10)
    filename = g_strdup_printf("%s%s0%d%s", get_cheese_path(), PHOTO_NAME_DEFAULT, i, PHOTO_NAME_SUFFIX_DEFAULT);
  else
    filename = g_strdup_printf("%s%s%d%s", get_cheese_path(), PHOTO_NAME_DEFAULT, i, PHOTO_NAME_SUFFIX_DEFAULT);

  return filename;
}

void
photos_monitor_cb(GnomeVFSMonitorHandle *monitor_handle, const gchar *monitor_uri, const gchar *info_uri, GnomeVFSMonitorEventType event_type)
{
  gchar *filename = gnome_vfs_get_local_path_from_uri(info_uri);
  gboolean is_dir;

  is_dir = g_file_test(filename, G_FILE_TEST_IS_DIR);

  if (!is_dir) {
    switch (event_type) {
      case GNOME_VFS_MONITOR_EVENT_DELETED:
        //FIXME: implement deleting photos
        //remove_photo(filename);
        break;
      //case GNOME_VFS_MONITOR_EVENT_CHANGED:
      case GNOME_VFS_MONITOR_EVENT_CREATED:
        g_message("new file found: %s\n", filename);
        append_photo(filename);
        break;
      default:
        break;
    }
  }
  g_free(filename);
}
