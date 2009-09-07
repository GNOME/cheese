/*
 * Copyright © 2007,2008 daniel g. siegel <dgsiegel@gnome.org>
 * Copyright © 2007,2008 Jaap Haitsma <jaap@haitsma.org>
 * Copyright © 2008 Felix Kaser <f.kaser@gmx.net>
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

#ifndef _CHEESE_DBUS_H_
#define _CHEESE_DBUS_H_

#include <dbus/dbus-glib.h>
#include <glib.h>

G_BEGIN_DECLS

typedef struct
{
  GObjectClass parent_class;
  DBusGConnection *connection;
} CheeseDbusClass;

typedef struct _CheeseDbus
{
  GObject parent;
} CheeseDbus;


#define CHEESE_TYPE_DBUS (cheese_dbus_get_type ())
#define CHEESE_DBUS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHEESE_TYPE_DBUS, CheeseDbus))
#define CHEESE_DBUS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CHEESE_TYPE_DBUS, CheeseDbusClass))
#define CHEESE_IS_DBUS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHEESE_TYPE_DBUS))
#define CHEESE_IS_DBUS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CHEESE_TYPE_DBUS))
#define CHEESE_DBUS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CHEESE_TYPE_DBUS, CheeseDbusClass))

GType       cheese_dbus_get_type (void);
CheeseDbus *cheese_dbus_new (void);

void     cheese_dbus_set_window (gpointer);
gboolean cheese_dbus_notify (void);

G_END_DECLS

#endif /* _CHEESE_DBUS_H_ */
