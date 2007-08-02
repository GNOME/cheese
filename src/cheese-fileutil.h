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

#ifndef __CHEESE_FILE_UTIL_H__
#define __CHEESE_FILE_UTIL_H__

void cheese_fileutil_init();
void cheese_fileutil_finalize();
gchar *cheese_fileutil_get_photo_path(void);
gchar *cheese_fileutil_get_photo_filename(int);
void cheese_fileutil_photos_monitor_cb(GnomeVFSMonitorHandle *, const gchar *, const gchar *, GnomeVFSMonitorEventType);

#endif /* __CHEESE_FILE_UTIL_H__ */
