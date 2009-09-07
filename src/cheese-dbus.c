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


#ifdef HAVE_CONFIG_H
  #include <cheese-config.h>
#endif

#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus.h>
#include <glib.h>
#include <unistd.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "cheese-dbus.h"
#include "cheese-window.h"
#include "cheese-dbus-infos.h"

gpointer window_pointer;

G_DEFINE_TYPE (CheeseDbus, cheese_dbus, G_TYPE_OBJECT);

void
cheese_dbus_set_window (gpointer data)
{
  if (data != NULL)
    window_pointer = data;
}

gboolean
cheese_dbus_notify ()
{
  cheese_window_bring_to_front (window_pointer);
  return TRUE;
}

void
cheese_dbus_class_init (CheeseDbusClass *klass)
{
  GError *error = NULL;

  /* Init the DBus connection, per-klass */
  klass->connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (klass->connection == NULL)
  {
    g_warning ("Unable to connect to dbus: %s", error->message);
    g_error_free (error);
    return;
  }

  dbus_g_object_type_install_info (CHEESE_TYPE_DBUS, &dbus_glib_cheese_dbus_object_info);
}

void
cheese_dbus_init (CheeseDbus *server)
{
  CheeseDbusClass *klass = CHEESE_DBUS_GET_CLASS (server);

  /* Register DBUS path */
  dbus_g_connection_register_g_object (klass->connection,
                                       "/org/gnome/cheese",
                                       G_OBJECT (server));
}

CheeseDbus *
cheese_dbus_new ()
{
  CheeseDbus      *server;
  GError          *error = NULL;
  DBusGProxy      *proxy;
  guint            request_ret;
  CheeseDbusClass *klass;

  server = g_object_new (CHEESE_TYPE_DBUS, NULL);

  klass = CHEESE_DBUS_GET_CLASS (server);

  /* Register the service name, the constant here are defined in dbus-glib-bindings.h */
  proxy = dbus_g_proxy_new_for_name (klass->connection,
                                     DBUS_SERVICE_DBUS,
                                     DBUS_PATH_DBUS,
                                     DBUS_INTERFACE_DBUS);

  if (!org_freedesktop_DBus_request_name (proxy,
                                          "org.gnome.Cheese",
                                          0, &request_ret,
                                          &error))
  {
    g_warning ("Unable to register service: %s", error->message);
    g_error_free (error);
  }

  /* check if there is already a instance running -> exit*/
  if (request_ret == DBUS_REQUEST_NAME_REPLY_EXISTS ||
      request_ret == DBUS_REQUEST_NAME_REPLY_IN_QUEUE)
  {
    g_warning ("Another instance of cheese is already running!");

    /* notify the other instance of cheese*/
    proxy = dbus_g_proxy_new_for_name (klass->connection,
                                       "org.gnome.Cheese",
                                       "/org/gnome/cheese",
                                       "org.gnome.Cheese");

    if (!dbus_g_proxy_call (proxy, "notify", &error, G_TYPE_INVALID, G_TYPE_INVALID))
    {
      /* Method failed, the GError is set, let's warn everyone */
      g_warning ("Notifying the other cheese instance failed: %s", error->message);
      g_error_free (error);
    }

    g_object_unref (server);
    server = NULL;
  }

  g_object_unref (proxy);

  return server;
}
