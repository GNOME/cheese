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

#ifndef __CHEESE_FILEUTIL_H__
#define __CHEESE_FILEUTIL_H__

#define PHOTO_NAME_SUFFIX ".jpg"
#define VIDEO_NAME_SUFFIX ".ogv"

G_BEGIN_DECLS

#include <glib-object.h>

#define CHEESE_TYPE_FILEUTIL (cheese_fileutil_get_type ())
#define CHEESE_FILEUTIL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHEESE_TYPE_FILEUTIL, CheeseFileUtil))
#define CHEESE_FILEUTIL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CHEESE_TYPE_FILEUTIL, CheeseFileUtilClass))
#define CHEESE_IS_FILEUTIL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHEESE_TYPE_FILEUTIL))
#define CHEESE_IS_FILEUTIL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CHEESE_TYPE_FILEUTIL))
#define CHEESE_FILEUTIL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CHEESE_TYPE_FILEUTIL, CheeseFileUtilClass))

typedef struct
{
  GObject parent;
} CheeseFileUtil;

typedef struct
{
  GObjectClass parent_class;
} CheeseFileUtilClass;

typedef enum
{
  CHEESE_MEDIA_MODE_PHOTO,
  CHEESE_MEDIA_MODE_VIDEO
} CheeseMediaMode;


GType           cheese_fileutil_get_type (void);
CheeseFileUtil *cheese_fileutil_new (void);

gchar *cheese_fileutil_get_video_path (CheeseFileUtil *fileutil);
gchar *cheese_fileutil_get_photo_path (CheeseFileUtil *fileutil);
gchar *cheese_fileutil_get_path_before_224 (CheeseFileUtil *fileutil);
gchar *cheese_fileutil_get_log_path (CheeseFileUtil *fileutil);
gchar *cheese_fileutil_get_new_media_filename (CheeseFileUtil *fileutil, CheeseMediaMode mode);

G_END_DECLS

#endif /* __CHEESE_FILEUTIL_H__ */
