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
  #include <config.h>
#endif

#include <glib.h>
#include <gio/gio.h>
#include <string.h>
#include <glib/gi18n-lib.h>

#include "cheese-fileutil.h"

/**
 * SECTION:cheese-file-util
 * @short_description: File utility functions for Cheese
 * @stability: Unstable
 * @include: cheese/cheese-fileutil.h
 *
 * #CheeseFileUtil provides some helpful utility functions for looking up paths
 * for photos and videos.
 */

typedef struct
{
  gchar *video_path;
  gchar *photo_path;
  guint  burst_count;
  gchar *burst_raw_name;
} CheeseFileUtilPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (CheeseFileUtil, cheese_fileutil, G_TYPE_OBJECT)

static gchar *
cheese_fileutil_get_path_before_224 (CheeseFileUtil *fileutil);

/**
 * cheese_fileutil_get_video_path:
 * @fileutil: a #CheeseFileUtil
 *
 * Get the path where Cheese video files are stored.
 *
 * Returns: (transfer none) (type filename): the Cheese video path
 */
const gchar *
cheese_fileutil_get_video_path (CheeseFileUtil *fileutil)
{
    CheeseFileUtilPrivate *priv;

  g_return_val_if_fail (CHEESE_IS_FILEUTIL (fileutil), NULL);

    priv = cheese_fileutil_get_instance_private (fileutil);

    return priv->video_path;
}

/**
 * cheese_fileutil_get_photo_path:
 * @fileutil: a #CheeseFileUtil
 *
 * Get the path where Cheese photo files are stored.
 *
 * Returns: (transfer none) (type filename): the Cheese photo path
 */
const gchar *
cheese_fileutil_get_photo_path (CheeseFileUtil *fileutil)
{
    CheeseFileUtilPrivate *priv;

  g_return_val_if_fail (CHEESE_IS_FILEUTIL (fileutil), NULL);

    priv = cheese_fileutil_get_instance_private (fileutil);

    return priv->photo_path;
}

/*
 * cheese_fileutil_get_path_before_224:
 * @fileutil: a #CheeseFileUtil
 *
 * Returns: (transfer full) (type filename): the photo path for Cheese versions before 2.24
 */
static gchar *
cheese_fileutil_get_path_before_224 (CheeseFileUtil *fileutil)
{
  return g_build_filename (g_get_home_dir (), ".gnome2", "cheese", "media", NULL);
}

/**
 * cheese_fileutil_get_new_media_filename:
 * @fileutil: a #CheeseFileUtil
 * @mode: the type of media to create a filename for
 *
 * Creates a filename for one of the three media types: photo, photo burst or
 * video. If a filename for a photo burst image was previously created, this
 * function increments the burst count automatically. To start a new burst,
 * first call cheese_fileutil_reset_burst().
 *
 * Returns: (transfer full) (type filename): a new filename
 */
gchar *
cheese_fileutil_get_new_media_filename (CheeseFileUtil *fileutil, CheeseMediaMode mode)
{
  GDateTime *datetime;
  gchar       *time_string;
  const gchar *path;
  gchar       *filename;
  GFile       *file;
  guint        num;
  CheeseFileUtilPrivate *priv;

  g_return_val_if_fail (CHEESE_IS_FILEUTIL (fileutil), NULL);

    priv = cheese_fileutil_get_instance_private (fileutil);

  datetime = g_date_time_new_now_local ();

  g_assert (datetime != NULL);

  time_string = g_date_time_format (datetime, "%F-%H%M%S");
  g_date_time_unref (datetime);

  g_assert (time_string != NULL);

  switch (mode)
  {
    case CHEESE_MEDIA_MODE_PHOTO:
    case CHEESE_MEDIA_MODE_BURST:
      path = cheese_fileutil_get_photo_path (fileutil);
      break;
    case CHEESE_MEDIA_MODE_VIDEO:
      path = cheese_fileutil_get_video_path (fileutil);
      break;
    default:
      g_assert_not_reached ();
  }

  g_mkdir_with_parents (path, 0775);

  switch (mode)
  {
    case CHEESE_MEDIA_MODE_PHOTO:
      filename = g_strdup_printf ("%s%s%s%s", path, G_DIR_SEPARATOR_S, time_string, CHEESE_PHOTO_NAME_SUFFIX);
      break;
    case CHEESE_MEDIA_MODE_BURST:
      priv->burst_count++;
      if (strlen (priv->burst_raw_name) == 0)
      {
        g_free (priv->burst_raw_name);
        priv->burst_raw_name = g_strdup_printf ("%s%s%s", path, G_DIR_SEPARATOR_S, time_string);
      }

      filename = g_strdup_printf ("%s_%d%s", priv->burst_raw_name, priv->burst_count, CHEESE_PHOTO_NAME_SUFFIX);
      break;
    case CHEESE_MEDIA_MODE_VIDEO:
      filename = g_strdup_printf ("%s%s%s%s", path, G_DIR_SEPARATOR_S, time_string, CHEESE_VIDEO_NAME_SUFFIX);
      break;
    default:
      g_assert_not_reached ();
  }

  file = g_file_new_for_path (filename);
  num = 0;

  while (g_file_query_exists (file, NULL))
  {
    num++;

    g_object_unref (file);
    g_free (filename);

    switch (mode)
    {
      case CHEESE_MEDIA_MODE_PHOTO:
        filename = g_strdup_printf ("%s%s%s (%d)%s", path, G_DIR_SEPARATOR_S, time_string, num, CHEESE_PHOTO_NAME_SUFFIX);
        break;
      case CHEESE_MEDIA_MODE_BURST:
        filename = g_strdup_printf ("%s_%d (%d)%s", priv->burst_raw_name, priv->burst_count, num, CHEESE_PHOTO_NAME_SUFFIX);
        break;
      case CHEESE_MEDIA_MODE_VIDEO:
        filename = g_strdup_printf ("%s%s%s (%d)%s", path, G_DIR_SEPARATOR_S, time_string, num, CHEESE_VIDEO_NAME_SUFFIX);
        break;
      default:
        g_assert_not_reached ();
    }

    file = g_file_new_for_path (filename);
  }

  g_free (time_string);
  g_object_unref (file);

  return filename;
}

/**
 * cheese_fileutil_reset_burst:
 * @fileutil: a #CheeseFileUtil
 *
 * Resets the burst counter, so that calling
 * cheese_fileutil_get_new_media_filename() with a photo burst starts a new
 * burst of photos.
 */
void
cheese_fileutil_reset_burst (CheeseFileUtil *fileutil)
{
  CheeseFileUtilPrivate *priv;

  g_return_if_fail (CHEESE_IS_FILEUTIL (fileutil));

    priv = cheese_fileutil_get_instance_private (fileutil);

  priv->burst_count    = 0;
  g_free (priv->burst_raw_name);
  priv->burst_raw_name = g_strdup ("");
}

static void
cheese_fileutil_finalize (GObject *object)
{
  CheeseFileUtil *fileutil = CHEESE_FILEUTIL (object);
    CheeseFileUtilPrivate *priv = cheese_fileutil_get_instance_private (fileutil);;

  g_free (priv->video_path);
  g_free (priv->photo_path);
  g_free (priv->burst_raw_name);
  G_OBJECT_CLASS (cheese_fileutil_parent_class)->finalize (object);
}

static void
cheese_fileutil_class_init (CheeseFileUtilClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = cheese_fileutil_finalize;
}

static void
cheese_fileutil_init (CheeseFileUtil *fileutil)
{
    CheeseFileUtilPrivate *priv = cheese_fileutil_get_instance_private (fileutil);

    GSettings *settings;

    priv->burst_count = 0;
    priv->burst_raw_name = g_strdup ("");

    settings = g_settings_new ("org.gnome.Cheese");

    priv->video_path = g_settings_get_string (settings, "video-path");
    priv->photo_path = g_settings_get_string (settings, "photo-path");

    /* Get the video path from GSettings, XDG or hardcoded. */
    if (!priv->video_path || !*priv->video_path)
    {
        const gchar *video_path;

        video_path = g_get_user_special_dir (G_USER_DIRECTORY_VIDEOS);
        g_free (priv->video_path);

        /* Get XDG. */
        if (video_path)
        {
            priv->video_path = g_build_filename (video_path, _("Webcam"), NULL);
        }
        else
        {
            /* Get "~/.gnome2/cheese/media". */
            priv->video_path = cheese_fileutil_get_path_before_224 (fileutil);
        }
    }

    /* Get the photo path from GSettings, XDG or hardcoded. */
    if (!priv->photo_path || !*priv->photo_path)
    {
        const gchar *photo_path;

        photo_path = g_get_user_special_dir (G_USER_DIRECTORY_PICTURES);
        g_free (priv->photo_path);

        /* Get XDG. */
        if (photo_path)
        {
            priv->photo_path = g_build_filename (photo_path, _("Webcam"), NULL);
        }
        else
        {
            /* Get "~/.gnome2/cheese/media". */
            priv->photo_path = cheese_fileutil_get_path_before_224 (fileutil);
        }
    }

    g_object_unref (settings);
}

/**
 * cheese_fileutil_new:
 *
 * Create a new #CheeseFileUtil object.
 *
 * Returns: a new #CheeseFileUtil
 */
CheeseFileUtil *
cheese_fileutil_new (void)
{
  static CheeseFileUtil *fileutil = NULL;

  if (fileutil != NULL)
    return g_object_ref (fileutil);

  fileutil = g_object_new (CHEESE_TYPE_FILEUTIL, NULL);
  g_object_add_weak_pointer (G_OBJECT (fileutil),
                             (gpointer) & fileutil);
  return fileutil;
}
