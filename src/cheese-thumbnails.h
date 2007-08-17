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

#ifndef __CHEESE_THUMBNAILS_H__
#define __CHEESE_THUMBNAILS_H__

#define THUMB_WIDTH (128)
#define THUMB_HEIGHT (96)

void cheese_thumbnails_init ();
void cheese_thumbnails_finalize ();
gchar *cheese_thumbnails_get_filename_from_path (GtkTreePath * path);
void cheese_thumbnails_append_item (gchar *);
void cheese_thumbnails_fill_thumbs ();
void cheese_thumbnails_remove_item (gchar *);

struct _thumbnails
{
  GtkListStore *store;
  GtkTreeIter iter;
  GtkWidget *iconview;
};

extern struct _thumbnails thumbnails;

#endif /* __CHEESE_THUMBNAILS_H__ */
