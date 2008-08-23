/*
 * Copyright © 2008 James Liggett <jrliggett@cox.net>
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

#ifndef _CHEESE_PREFS_WIDGET_H_
#define _CHEESE_PREFS_WIDGET_H_

#include <glib-object.h>
#include <gtk/gtk.h>
#include "cheese-gconf.h"

G_BEGIN_DECLS

#define CHEESE_TYPE_PREFS_WIDGET (cheese_prefs_widget_get_type ())
#define CHEESE_PREFS_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHEESE_TYPE_PREFS_WIDGET, \
                                                                         CheesePrefsWidget))
#define CHEESE_PREFS_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CHEESE_TYPE_PREFS_WIDGET, \
                                                                      CheesePrefsWidgetClass))
#define CHEESE_IS_PREFS_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHEESE_TYPE_PREFS_WIDGET))
#define CHEESE_IS_PREFS_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CHEESE_TYPE_PREFS_WIDGET))
#define CHEESE_PREFS_WIDGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CHEESE_TYPE_PREFS_WIDGET, \
                                                                        CheesePrefsWidgetClass))

typedef struct _CheesePrefsWidgetClass CheesePrefsWidgetClass;
typedef struct _CheesePrefsWidget CheesePrefsWidget;

struct _CheesePrefsWidgetClass
{
  GObjectClass parent_class;

  /* Signals */
  void (*changed) (CheesePrefsWidget *self);

  /* Virtual methods */
  void (*synchronize) (CheesePrefsWidget *self);
};

struct _CheesePrefsWidget
{
  GObject parent_instance;

  CheeseGConf *gconf;
};

GType cheese_prefs_widget_get_type (void) G_GNUC_CONST;

void cheese_prefs_widget_synchronize (CheesePrefsWidget *prefs_widget);
void cheese_prefs_widget_notify_changed (CheesePrefsWidget *prefs_widget);

G_END_DECLS

#endif /* _CHEESE_PREFS_WIDGET_H_ */
