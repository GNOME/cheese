/*
 * Copyright © 2009 Filippo Argiolas <filippo.argiolas@gmail.com>
 * Copyright © 2007,2008 daniel g. siegel <dgsiegel@gnome.org>
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

#ifndef __CHEESE_WINDOW_H__
#define __CHEESE_WINDOW_H__

#include <gtk/gtk.h>
#include "cheese-dbus.h"

G_BEGIN_DECLS

#define CHEESE_TYPE_WINDOW (cheese_window_get_type ())
#define CHEESE_WINDOW(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), CHEESE_TYPE_WINDOW, CheeseWindow))
#define CHEESE_WINDOW_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((klass), CHEESE_TYPE_WINDOW, CheeseWindowClass))
#define CHEESE_IS_WINDOW(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), CHEESE_TYPE_WINDOW))
#define CHEESE_IS_WINDOW_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), CHEESE_TYPE_WINDOW))
#define CHEESE_WINDOW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CHEESE_TYPE_WINDOW, CheeseWindowClass))

typedef struct
{
  GtkWindow parent;
} CheeseWindow;

typedef struct
{
  GtkWindowClass parent_class;
} CheeseWindowClass;

GType cheese_window_get_type (void) G_GNUC_CONST;

/* public methods */
CheeseWindow *cheese_window_new (void);

#if 0
void cheese_window_init (char *hal_dev_udi, CheeseDbus *dbus_server, gboolean startup_in_wide_mode);
void cheese_window_bring_to_front (gpointer data);
#endif

#endif /* __CHEESE_WINDOW_H__ */
