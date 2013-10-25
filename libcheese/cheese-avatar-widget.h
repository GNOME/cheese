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

#ifndef _CHEESE_AVATAR_WIDGET_H_
#define _CHEESE_AVATAR_WIDGET_H_

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CHEESE_TYPE_AVATAR_WIDGET (cheese_avatar_widget_get_type ())
#define CHEESE_AVATAR_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHEESE_TYPE_AVATAR_WIDGET, \
                                                                           CheeseAvatarWidget))
#define CHEESE_AVATAR_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CHEESE_TYPE_AVATAR_WIDGET, \
                                                                        CheeseAvatarWidgetClass))
#define CHEESE_IS_AVATAR_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHEESE_TYPE_AVATAR_WIDGET))
#define CHEESE_IS_AVATAR_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CHEESE_TYPE_AVATAR_WIDGET))
#define CHEESE_AVATAR_WIDGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CHEESE_TYPE_AVATAR_WIDGET, \
                                                                          CheeseAvatarWidgetClass))

typedef struct _CheeseAvatarWidgetPrivate CheeseAvatarWidgetPrivate;
typedef struct _CheeseAvatarWidgetClass CheeseAvatarWidgetClass;
typedef struct _CheeseAvatarWidget CheeseAvatarWidget;

/**
 * CheeseAvatarWidgetClass:
 *
 * Use the accessor functions below.
 */
struct _CheeseAvatarWidgetClass
{
  /*< private >*/
  GtkBinClass parent_class;
};

/**
 * CheeseAvatarWidget:
 *
 * Use the accessor functions below.
 */
struct _CheeseAvatarWidget
{
  /*< private >*/
  GtkBin parent_instance;
  CheeseAvatarWidgetPrivate *priv;
};

GType cheese_avatar_widget_get_type (void) G_GNUC_CONST;

GtkWidget *cheese_avatar_widget_new (void);
GdkPixbuf *cheese_avatar_widget_get_picture (CheeseAvatarWidget *widget);

G_END_DECLS

#endif /* _CHEESE_AVATAR_WIDGET_H_ */
