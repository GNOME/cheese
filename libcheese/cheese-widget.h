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

#ifndef _CHEESE_WIDGET_H_
#define _CHEESE_WIDGET_H_

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CHEESE_TYPE_WIDGET (cheese_widget_get_type ())
#define CHEESE_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHEESE_TYPE_WIDGET, \
                                                                   CheeseWidget))
#define CHEESE_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CHEESE_TYPE_WIDGET, \
                                                                CheeseWidgetClass))
#define CHEESE_IS_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHEESE_TYPE_WIDGET))
#define CHEESE_IS_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CHEESE_TYPE_WIDGET))
#define CHEESE_WIDGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CHEESE_TYPE_WIDGET, \
                                                                  CheeseWidgetClass))

typedef struct _CheeseWidgetClass CheeseWidgetClass;
typedef struct _CheeseWidget CheeseWidget;

struct _CheeseWidgetClass
{
  GtkNotebookClass parent_class;

  void (*ready)(CheeseWidget *widget, gboolean is_ready);
  void (*error)(CheeseWidget *widget, const char *error);
};

struct _CheeseWidget
{
  GtkNotebook parent_instance;
};

GType cheese_widget_get_type (void) G_GNUC_CONST;

GtkWidget *cheese_widget_new (void);

GObject *cheese_widget_get_camera (CheeseWidget *widget);

G_END_DECLS

#endif /* _CHEESE_WIDGET_H_ */
