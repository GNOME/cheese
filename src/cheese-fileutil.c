/*
 * Copyright © 2007,2008 daniel g. siegel <dgsiegel@gnome.org>
 * Copyright © 2007,2008 Jaap Haitsma <jaap@haitsma.org>
 * Copyright © 2008 Felix Kaser <f.kaser@gmx.net>
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
#include <string.h>

#include "cheese-fileutil.h"
#include "cheese-gconf.h"

G_DEFINE_TYPE (CheeseFileUtil, cheese_fileutil, G_TYPE_OBJECT)

#define CHEESE_FILEUTIL_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHEESE_TYPE_FILEUTIL, CheeseFileUtilPrivate))

typedef struct
{
  gchar *video_path;
  gchar *photo_path;
  gchar *log_path;
} CheeseFileUtilPrivate;

gchar *
cheese_fileutil_get_video_path (CheeseFileUtil *fileutil)
{
  CheeseFileUtilPrivate *priv = CHEESE_FILEUTIL_GET_PRIVATE (fileutil);

  gchar *path;

  path = priv->video_path;

  return path;
}

gchar *
cheese_fileutil_get_photo_path (CheeseFileUtil *fileutil)
{
  CheeseFileUtilPrivate *priv = CHEESE_FILEUTIL_GET_PRIVATE (fileutil);

  gchar *path;

  path = priv->photo_path;

  return path;
}

gchar *
cheese_fileutil_get_path_before_224 (CheeseFileUtil *fileutil)
{
  return g_strjoin (G_DIR_SEPARATOR_S, g_get_home_dir (), ".gnome2", "cheese", "media", NULL);
}

gchar *
cheese_fileutil_get_log_path (CheeseFileUtil *fileutil)
{
  CheeseFileUtilPrivate *priv = CHEESE_FILEUTIL_GET_PRIVATE (fileutil);

  gchar *path;

  path = priv->log_path;

  return path;
}

gchar *
cheese_fileutil_get_new_media_filename (CheeseFileUtil *fileutil, CheeseMediaMode mode)
{
  struct tm *ptr;
  time_t     tm;
  char       date[21];
  gchar     *path;
  char      *filename;
  GFile     *file;
  int        num;

  tm  = time (NULL);
  ptr = localtime (&tm);
  strftime (date, 20, "%F-%H%M%S", ptr);

  if (mode == CHEESE_MEDIA_MODE_PHOTO)
    path = cheese_fileutil_get_photo_path (fileutil);
  else
    path = cheese_fileutil_get_video_path (fileutil);

  if (mode == CHEESE_MEDIA_MODE_PHOTO)
    filename = g_strdup_printf ("%s%s%s%s", path, G_DIR_SEPARATOR_S, date, PHOTO_NAME_SUFFIX);
  else
    filename = g_strdup_printf ("%s%s%s%s", path, G_DIR_SEPARATOR_S, date, VIDEO_NAME_SUFFIX);

  file = g_file_new_for_path (filename);

  if (g_file_query_exists (file, NULL))
  {
    num = 1;
    if (mode == CHEESE_MEDIA_MODE_PHOTO)
      filename = g_strdup_printf ("%s%s%s (%d)%s", path, G_DIR_SEPARATOR_S, date, num, PHOTO_NAME_SUFFIX);
    else
      filename = g_strdup_printf ("%s%s%s (%d)%s", path, G_DIR_SEPARATOR_S, date, num, VIDEO_NAME_SUFFIX);

    file = g_file_new_for_path (filename);

    while (g_file_query_exists (file, NULL))
    {
      num++;
      if (mode == CHEESE_MEDIA_MODE_PHOTO)
        filename = g_strdup_printf ("%s%s%s (%d)%s", path, G_DIR_SEPARATOR_S, date, num, PHOTO_NAME_SUFFIX);
      else
        filename = g_strdup_printf ("%s%s%s (%d)%s", path, G_DIR_SEPARATOR_S, date, num, VIDEO_NAME_SUFFIX);

      file = g_file_new_for_path (filename);
    }
  }

  return filename;
}

static void
cheese_fileutil_finalize (GObject *object)
{
  CheeseFileUtil *fileutil;

  fileutil = CHEESE_FILEUTIL (object);
  CheeseFileUtilPrivate *priv = CHEESE_FILEUTIL_GET_PRIVATE (fileutil);

  g_free (priv->video_path);
  g_free (priv->photo_path);
  g_free (priv->log_path);
  G_OBJECT_CLASS (cheese_fileutil_parent_class)->finalize (object);
}

static void
cheese_fileutil_class_init (CheeseFileUtilClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = cheese_fileutil_finalize;

  g_type_class_add_private (klass, sizeof (CheeseFileUtilPrivate));
}

static void
cheese_fileutil_init (CheeseFileUtil *fileutil)
{
  CheeseFileUtilPrivate *priv = CHEESE_FILEUTIL_GET_PRIVATE (fileutil);

  CheeseGConf *gconf;

  gconf = cheese_gconf_new ();

  g_object_get (gconf, "gconf_prop_video_path", &priv->video_path, NULL);
  g_object_get (gconf, "gconf_prop_photo_path", &priv->photo_path, NULL);

  /* get the video path from gconf, xdg or hardcoded */
  if (!priv->video_path || strcmp (priv->video_path, "") == 0)
  {
    /* get xdg */
    priv->video_path = g_strjoin (G_DIR_SEPARATOR_S, g_get_user_special_dir (G_USER_DIRECTORY_VIDEOS), "Webcam", NULL);
    if (strcmp (priv->video_path, "") == 0)
    {
      /* get "~/.gnome2/cheese/media" */
      priv->video_path = cheese_fileutil_get_path_before_224 (fileutil);
    }
  }

  /* get the photo path from gconf, xdg or hardcoded */
  if (!priv->photo_path || strcmp (priv->photo_path, "") == 0)
  {
    /* get xdg */
    priv->photo_path = g_strjoin (G_DIR_SEPARATOR_S, g_get_user_special_dir (G_USER_DIRECTORY_PICTURES), "Webcam", NULL);
    if (strcmp (priv->photo_path, "") == 0)
    {
      /* get "~/.gnome2/cheese/media" */
      priv->photo_path = cheese_fileutil_get_path_before_224 (fileutil);
    }
  }

  /* FIXME: use the xdg log path */
  priv->log_path = g_strjoin (G_DIR_SEPARATOR_S, g_get_home_dir (), ".gnome2", "cheese", NULL);

  g_object_unref (gconf);
}

CheeseFileUtil *
cheese_fileutil_new ()
{
  CheeseFileUtil *fileutil;

  fileutil = g_object_new (CHEESE_TYPE_FILEUTIL, NULL);
  return fileutil;
}
