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

#ifndef __CHEESE_FILE_UTIL_H__
#define __CHEESE_FILE_UTIL_H__

#define PHOTO_NAME_SUFFIX ".jpg"
#define VIDEO_NAME_SUFFIX ".ogg"

typedef enum
{
  CHEESE_MEDIA_MODE_PHOTO,
  CHEESE_MEDIA_MODE_VIDEO
} CheeseMediaMode;

char *cheese_fileutil_get_path(void);
char *cheese_fileutil_get_media_path (void);
char *cheese_fileutil_get_new_media_filename (CheeseMediaMode mode);

#endif /* __CHEESE_FILE_UTIL_H__ */
