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

#ifndef CHEESE_AVATAR_CHOOSER_H_
#define CHEESE_AVATAR_CHOOSER_H_

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

/**
 * CheeseAvatarChooser:
 *
 * Use the accessor functions below.
 */
struct _CheeseAvatarChooser
{
  /*< private >*/
  GtkDialog parent_instance;
  void *unused;
};

#define CHEESE_TYPE_AVATAR_CHOOSER (cheese_avatar_chooser_get_type ())
G_DECLARE_FINAL_TYPE (CheeseAvatarChooser, cheese_avatar_chooser, CHEESE, AVATAR_CHOOSER, GtkDialog)

GtkWidget *cheese_avatar_chooser_new (void);
GdkPixbuf *cheese_avatar_chooser_get_picture (CheeseAvatarChooser *chooser);

G_END_DECLS

#endif /* CHEESE_AVATAR_CHOOSER_H_ */
