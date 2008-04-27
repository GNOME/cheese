/*
 * Copyright (C) 2007,2008 daniel g. siegel <dgsiegel@gmail.com>
 * Copyright (C) 2007,2008 Jaap Haitsma <jaap@haitsma.org>
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

#ifdef HAVE_CONFIG_H
#include <cheese-config.h>
#endif

#include <glib.h>
#include <gio/gio.h>
#include <stdlib.h>

#include "cheese-fileutil.h"

char *
cheese_fileutil_get_path ()
{
  char *path;
#ifdef HILDON
  path = g_strjoin (G_DIR_SEPARATOR_S, g_get_home_dir(), "Mydocs", ".images", NULL);
#else
  path = g_strjoin (G_DIR_SEPARATOR_S, g_get_home_dir(), ".gnome2", "cheese", NULL);
#endif
  return path;
}

char *
cheese_fileutil_get_media_path ()
{
  char *path;
  char *cheese_path;

  cheese_path = cheese_fileutil_get_path ();
  path = g_strjoin (G_DIR_SEPARATOR_S, cheese_path, "media", NULL);
  g_free (cheese_path);

  return path;
}

char *
cheese_fileutil_get_new_media_filename (CheeseMediaMode mode)
{
  struct tm *ptr;
  time_t tm;
  char date[21];
  char *path;
  char *filename;
  GFile *file;
  int num;

  tm = time (NULL);
  ptr = localtime (&tm);
  strftime (date, 20, "%F-%H%M%S", ptr);

  path = cheese_fileutil_get_media_path ();

  if (mode == CHEESE_MEDIA_MODE_PHOTO)
    filename = g_strdup_printf ("%s%s%s%s", path, G_DIR_SEPARATOR_S, date, PHOTO_NAME_SUFFIX);
  else
    filename = g_strdup_printf ("%s%s%s%s", path, G_DIR_SEPARATOR_S, date, VIDEO_NAME_SUFFIX);

  file = g_file_new_for_path (filename);

  if (g_file_query_exists (file, NULL)) {
    num = 1;
    if (mode == CHEESE_MEDIA_MODE_PHOTO)
      filename = g_strdup_printf ("%s%s%s (%d)%s", path, G_DIR_SEPARATOR_S, date, num, PHOTO_NAME_SUFFIX);
    else
      filename = g_strdup_printf ("%s%s%s (%d)%s", path, G_DIR_SEPARATOR_S, date, num, VIDEO_NAME_SUFFIX);

    file = g_file_new_for_path (filename);

    while (g_file_query_exists (file, NULL)) {
      num++;
      if (mode == CHEESE_MEDIA_MODE_PHOTO)
        filename = g_strdup_printf ("%s%s%s (%d)%s", path, G_DIR_SEPARATOR_S, date, num, PHOTO_NAME_SUFFIX);
      else
        filename = g_strdup_printf ("%s%s%s (%d)%s", path, G_DIR_SEPARATOR_S, date, num, VIDEO_NAME_SUFFIX);

      file = g_file_new_for_path (filename);
    }
  }

  g_free (path);
  return filename;
}

