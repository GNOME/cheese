/*
 *  GNOME Camera - Access camera devices on a system via D-Bus
 *  Copyright (C) 2014  Red Hat, Inc.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GC_CLIENT_MONITOR_H_
#define GC_CLIENT_MONITOR_H_

#include <gio/gio.h>

G_BEGIN_DECLS

typedef struct
{
    /*< private >*/
    GObject parent_instance;
} GcClientMonitor;

typedef struct
{
    GObjectClass parent_class;

    void (* peer_vanished) (GcClientMonitor *self);
} GcClientMonitorClass;

#define GC_TYPE_CLIENT_MONITOR (gc_client_monitor_get_type ())
#define GC_CLIENT_MONITOR(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GC_TYPE_CLIENT_MONITOR, GcClientMonitor))

GType gc_client_monitor_get_type (void);
void gc_client_monitor_new_async (const gchar *bus_name, GDBusConnection *connection, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
GcClientMonitor * gc_client_monitor_new_finish (GAsyncResult *result, GError **error);

G_END_DECLS

#endif /* GC_CLIENT_MONITOR_H_ */
