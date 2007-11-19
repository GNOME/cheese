/*
 * Copyright (C) 2007 daniel g. siegel <dgsiegel@gmail.com>
 * Copyright (C) 2007 Jaap Haitsma <jaap@haitsma.org>
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
#include <config.h>
#endif

#include <glib.h>
#include <stdlib.h>

#include "cheese-fileutil.h"

char *
cheese_fileutil_get_media_path ()
{
  const char *home_dir;
  char *path;

  home_dir = g_get_home_dir ();
  path = g_strdup_printf ("%s/.gnome2/cheese/media", home_dir);

  return path;
}

char *
cheese_fileutil_get_new_media_filename (CheeseMediaMode mode)
{
  static int filename_num = 0;
  char *filename;
  GDir *dir;
  char *path;
  const char *name;
  int num;
  
  path = cheese_fileutil_get_media_path ();
  if (filename_num == 0)
  { 
    dir = g_dir_open (path, 0, NULL);
    while ((name = g_dir_read_name (dir)) != NULL)
    {
      if (g_str_has_suffix (name, PHOTO_NAME_SUFFIX) || 
          g_str_has_suffix (name, VIDEO_NAME_SUFFIX))
      {
        num = atoi (name);
        if (num > filename_num)
          filename_num = num;
      }
    }
  }
  filename_num++;

  if (mode == CHEESE_MEDIA_MODE_PHOTO)
    filename = g_strdup_printf ("%s/%04d%s", path, filename_num, PHOTO_NAME_SUFFIX);
  else
    filename = g_strdup_printf ("%s/%04d%s", path, filename_num, VIDEO_NAME_SUFFIX);

  g_free (path);
  return filename;
}

