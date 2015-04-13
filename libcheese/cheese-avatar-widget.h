/*
 * Copyright © 2009 Bastien Nocera <hadess@hadess.net>
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

#ifndef CHEESE_AVATAR_WIDGET_H_
#define CHEESE_AVATAR_WIDGET_H_

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

/**
 * CheeseAvatarWidget:
 *
 * Use the accessor functions below.
 */
struct _CheeseAvatarWidget
{
  /*< private >*/
  GtkBin parent_instance;
  void *unused;
};

#define CHEESE_TYPE_AVATAR_WIDGET (cheese_avatar_widget_get_type ())
G_DECLARE_FINAL_TYPE (CheeseAvatarWidget, cheese_avatar_widget, CHEESE, AVATAR_WIDGET, GtkBin)

GtkWidget *cheese_avatar_widget_new (void);
GdkPixbuf *cheese_avatar_widget_get_picture (CheeseAvatarWidget *widget);

G_END_DECLS

#endif /* CHEESE_AVATAR_WIDGET_H_ */
