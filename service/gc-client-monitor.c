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

#include "config.h"

#include "gc-client-monitor.h"

typedef struct
{
    gchar *peer;
    GDBusConnection *connection;
    GDBusProxy *dbus_proxy;
    guint watch_id;
    guint32 user_id;
} GcClientMonitorPrivate;

static void gc_client_monitor_async_initable_iface_init (GAsyncInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (GcClientMonitor,
                         gc_client_monitor,
                         G_TYPE_OBJECT,
                         G_ADD_PRIVATE (GcClientMonitor)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE, gc_client_monitor_async_initable_iface_init))

enum
{
    PROP_0,
    PROP_CONNECTION,
    PROP_PEER,
    N_PROPERTIES
};

enum
{
    PEER_VANISHED,
    N_SIGNALS
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };
static guint signals[N_SIGNALS];

static void
on_name_vanished (GDBusConnection *connection,
                  const gchar *name,
                  gpointer user_data)
{
    g_signal_emit (GC_CLIENT_MONITOR (user_data), signals[PEER_VANISHED], 0);
}

static void
on_get_user_id_ready (GObject *source_object,
                      GAsyncResult *result,
                      gpointer user_data)
{
    GTask *task;
    GcClientMonitor *self;
    GcClientMonitorPrivate *priv;
    GError *error = NULL;
    GVariant *results = NULL;

    task = G_TASK (user_data);
    self = GC_CLIENT_MONITOR (g_task_get_source_object (task));
    priv = gc_client_monitor_get_instance_private (self);

    results = g_dbus_proxy_call_finish (G_DBUS_PROXY (source_object),
                                        result,
                                        &error);
    if (results == NULL)
    {
        g_task_return_error (task, error);
        g_object_unref (task);

        return;
    }

    g_assert (g_variant_n_children (results) > 0);
    g_variant_get_child (results, 0, "u", &priv->user_id);
    g_variant_unref (results);

    priv->watch_id = g_bus_watch_name_on_connection (priv->connection,
                                                     priv->peer,
                                                     G_BUS_NAME_WATCHER_FLAGS_NONE,
                                                     NULL,
                                                     on_name_vanished,
                                                     self,
                                                     NULL);

    g_task_return_boolean (task, TRUE);

    g_object_unref (task);
}

static void
on_dbus_proxy_ready (GObject *source_object,
                     GAsyncResult *result,
                     gpointer user_data)
{
    GTask *task;
    GcClientMonitor *self;
    GcClientMonitorPrivate *priv;
    GError *error = NULL;

    task = G_TASK (user_data);
    self = GC_CLIENT_MONITOR (g_task_get_source_object (task));
    priv = gc_client_monitor_get_instance_private (self);

    priv->dbus_proxy = g_dbus_proxy_new_for_bus_finish (result, &error);

    if (priv->dbus_proxy == NULL)
    {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    g_dbus_proxy_call (priv->dbus_proxy,
                       "GetConnectionUnixUser",
                       g_variant_new ("(s)", priv->peer),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       g_task_get_cancellable (task),
                       on_get_user_id_ready,
                       task);
}

static void
gc_client_monitor_finalize (GObject *object)
{
    GcClientMonitorPrivate *priv;

    priv = gc_client_monitor_get_instance_private (GC_CLIENT_MONITOR (object));

    g_free (priv->peer);

    G_OBJECT_CLASS (gc_client_monitor_parent_class)->finalize (object);
}

static void
gc_client_monitor_get_property (GObject *object,
                                guint prop_id,
                                GValue *value,
                                GParamSpec *pspec)
{
    GcClientMonitorPrivate *priv;

    priv = gc_client_monitor_get_instance_private (GC_CLIENT_MONITOR (object));

    switch (prop_id)
    {
        case PROP_CONNECTION:
            g_value_set_object (value, priv->connection);
            break;
        case PROP_PEER:
            g_value_set_string (value, priv->peer);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gc_client_monitor_set_property (GObject *object,
                                guint prop_id,
                                const GValue *value,
                                GParamSpec *pspec)
{
    GcClientMonitorPrivate *priv;

    priv = gc_client_monitor_get_instance_private (GC_CLIENT_MONITOR (object));

    switch (prop_id)
    {
        case PROP_CONNECTION:
            priv->connection = g_value_dup_object (value);
            break;
        case PROP_PEER:
            g_free (priv->peer);
            priv->peer = g_value_dup_string (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gc_client_monitor_class_init (GcClientMonitorClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = gc_client_monitor_finalize;
    object_class->get_property = gc_client_monitor_get_property;
    object_class->set_property = gc_client_monitor_set_property;

    properties[PROP_CONNECTION] = g_param_spec_object ("connection",
                                                       "Connection",
                                                       "DBus Connection",
                                                       G_TYPE_DBUS_CONNECTION,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_STRINGS);

    properties[PROP_PEER] = g_param_spec_string ("peer",
                                                 "Peer",
                                                 "D-Bus object path of peer to monitor",
                                                 NULL,
                                                 G_PARAM_READWRITE |
                                                 G_PARAM_CONSTRUCT_ONLY |
                                                 G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
gc_client_monitor_init (GcClientMonitor *self)
{
}

static void
gc_client_monitor_async_initable_init_async (GAsyncInitable *initable,
                                             gint io_priority,
                                             GCancellable *cancellable,
                                             GAsyncReadyCallback callback,
                                             gpointer user_data)
{
    GTask *task;

    task = g_task_new (initable, cancellable, callback, user_data);

    g_dbus_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                              G_DBUS_PROXY_FLAGS_NONE,
                              NULL,
                              "org.freedesktop.DBus",
                              "/org/freedesktop/DBus",
                              "org.freedesktop.DBus",
                              cancellable,
                              on_dbus_proxy_ready,
                              task);
}

static gboolean
gc_client_monitor_async_initable_init_finish (GAsyncInitable *initable,
                                              GAsyncResult *result,
                                              GError **error)
{
    return g_task_propagate_boolean (G_TASK (result), error);
}

static void
gc_client_monitor_async_initable_iface_init (GAsyncInitableIface *iface)
{
    iface->init_async = gc_client_monitor_async_initable_init_async;
    iface->init_finish = gc_client_monitor_async_initable_init_finish;
}

void
gc_client_monitor_new_async (const gchar *peer,
                             GDBusConnection *connection,
                             GCancellable *cancellable,
                             GAsyncReadyCallback callback,
                             gpointer user_data)
{
    g_async_initable_new_async (GC_TYPE_CLIENT_MONITOR,
                                G_PRIORITY_DEFAULT,
                                cancellable,
                                callback,
                                user_data,
                                "connection",
                                connection,
                                "peer",
                                peer,
                                NULL);
}

GcClientMonitor *
gc_client_monitor_new_finish (GAsyncResult *result, GError **error)
{
    GObject *object;
    GObject *source_object;

    source_object = g_async_result_get_source_object (result);
    object = g_async_initable_new_finish (G_ASYNC_INITABLE (source_object),
                                          result,
                                          error);
    g_object_unref (source_object);

    if (object != NULL)
    {
        return GC_CLIENT_MONITOR (object);
    }
    else
    {
        return NULL;
    }
}
