/* Stolen from Eye Of Gnome
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Taken from gnome-utils/gnome-screensaver/screensaver-shadow.h
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

#ifndef EOG_THUMB_SHADOW_H
#define EOG_THUMB_SHADOW_H

#include <gtk/gtk.h>

void eog_thumb_shadow_add_border (GdkPixbuf **);
void eog_thumb_shadow_add_frame (GdkPixbuf **);
void eog_thumb_shadow_add_rectangle (GdkPixbuf **);
void eog_thumb_shadow_add_round_border (GdkPixbuf **);
void eog_thumb_shadow_add_shadow (GdkPixbuf **);

#endif /* EOG_THUMB_SHADOW_H */
