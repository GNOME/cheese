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

#include "gc-camera-manager.h"

#include "gc-camera-client.h"
#include "gc-client-monitor.h"

typedef struct
{
    GDBusConnection *connection;
    GStrv cameras;
    GHashTable *clients;
    guint client_ids;
} GcCameraManagerPrivate;

static void gc_camera_manager_manager_iface_init (GcManagerIface *iface);
static void gc_camera_manager_initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (GcCameraManager, gc_camera_manager,
                         GC_TYPE_MANAGER_SKELETON,
                         G_ADD_PRIVATE (GcCameraManager)
                         G_IMPLEMENT_INTERFACE (GC_TYPE_MANAGER, gc_camera_manager_manager_iface_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, gc_camera_manager_initable_iface_init))

typedef struct
{
    GcCameraManager *manager;
    GDBusMethodInvocation *invocation;
    GcClientMonitor *monitor;
} OnClientMonitorNewReadyData;

enum
{
    PROP_0,
    PROP_CONNECTION,
    PROP_CAMERAS,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

static void
on_client_monitor_new_ready (GObject *source_object,
                             GAsyncResult *result,
                             gpointer user_data)
{
    OnClientMonitorNewReadyData *data;
    GcClientMonitor *monitor;
    GError *error = NULL;
    GcCameraManager *self;
    GcCameraManagerPrivate *priv;
    gchar *path;
    GcCameraClient *client;

    data = (OnClientMonitorNewReadyData *)user_data;
    monitor = gc_client_monitor_new_finish (result, &error);

    if (monitor == NULL)
    {
        g_dbus_method_invocation_return_error (data->invocation,
                                               G_DBUS_ERROR,
                                               G_DBUS_ERROR_FAILED,
                                               "%s",
                                               error->message);
        g_error_free (error);
        g_slice_free (OnClientMonitorNewReadyData, data);

        return;
    }

    self = data->manager;
    priv = gc_camera_manager_get_instance_private (self);

    path = g_strdup_printf ("/org/gnome/Camera/Client/%d", ++priv->client_ids);

    /* FIXME: Do this properly when adding multiple client handling. */
    g_application_hold (g_application_get_default ());

    client = gc_camera_client_new (priv->connection, path, &error);

    if (client == NULL)
    {
        goto error_out;
    }

    g_hash_table_insert (priv->clients, path, client);

    gc_manager_complete_get_client (GC_MANAGER (self), data->invocation, path);

    goto out;

error_out:
    g_dbus_method_invocation_return_error (data->invocation,
                                           G_DBUS_ERROR,
                                           G_DBUS_ERROR_FAILED,
                                           "%s",
                                           error->message);
    g_error_free (error);

out:
    g_clear_object (&monitor);
    g_slice_free (OnClientMonitorNewReadyData, data);
    g_free (path);
}

static gboolean
gc_camera_manager_handle_get_client (GcManager *manager,
                                     GDBusMethodInvocation *invocation)
{
    GcCameraManager *self;
    GcCameraManagerPrivate *priv;
    const gchar *peer;
    GcCameraClient *client;
    OnClientMonitorNewReadyData *data;

    self = GC_CAMERA_MANAGER (manager);
    priv = gc_camera_manager_get_instance_private (self);

    peer = g_dbus_method_invocation_get_sender (invocation);
    client = g_hash_table_lookup (priv->clients, peer);

    if (client != NULL)
    {
        const gchar *path;

        path = gc_camera_client_get_path (client);
        gc_manager_complete_get_client (manager, invocation, path);
        return TRUE;
    }

    /* FIXME: Monitor when the associated peer disappears from the bus. */
    data = g_slice_new (OnClientMonitorNewReadyData);
    data->manager = self;
    data->invocation = invocation;
    gc_client_monitor_new_async (peer,
                                 priv->connection,
                                 NULL,
                                 on_client_monitor_new_ready,
                                 data);

    return TRUE;
}

static void
gc_camera_manager_finalize (GObject *object)
{
    GcCameraManagerPrivate *priv;

    priv = gc_camera_manager_get_instance_private (GC_CAMERA_MANAGER (object));

    g_clear_object (&priv->connection);
    g_clear_pointer (&priv->clients, g_hash_table_unref);

    G_OBJECT_CLASS (gc_camera_manager_parent_class)->finalize (object);
}

static void
gc_camera_manager_get_property (GObject *object,
                                guint prop_id,
                                GValue *value,
                                GParamSpec *pspec)
{
    GcCameraManagerPrivate *priv;

    priv = gc_camera_manager_get_instance_private (GC_CAMERA_MANAGER (object));

    switch (prop_id)
    {
        case PROP_CONNECTION:
            g_value_set_object (value, priv->connection);
            break;
        case PROP_CAMERAS:
            g_value_set_boxed (value, priv->cameras);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gc_camera_manager_set_property (GObject *object,
                                guint prop_id,
                                const GValue *value,
                                GParamSpec *pspec)
{
    GcCameraManagerPrivate *priv;

    priv = gc_camera_manager_get_instance_private (GC_CAMERA_MANAGER (object));

    switch (prop_id)
    {
        case PROP_CONNECTION:
            priv->connection = g_value_dup_object (value);
            break;
        case PROP_CAMERAS:
            g_strfreev (priv->cameras);
            priv->cameras = g_value_dup_boxed (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gc_camera_manager_class_init (GcCameraManagerClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = gc_camera_manager_finalize;
    object_class->get_property = gc_camera_manager_get_property;
    object_class->set_property = gc_camera_manager_set_property;

    properties[PROP_CONNECTION] = g_param_spec_object ("connection",
                                                       "Connection",
                                                       "DBus Connection",
                                                       G_TYPE_DBUS_CONNECTION,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_STRINGS);
    g_object_class_install_property (object_class, PROP_CONNECTION,
                                     properties[PROP_CONNECTION]);

    properties[PROP_CAMERAS] = g_param_spec_boxed ("cameras",
                                                   "Cameras",
                                                   "List of connected cameras",
                                                   G_TYPE_STRV,
                                                   G_PARAM_READWRITE |
                                                   G_PARAM_STATIC_STRINGS);
    g_object_class_install_property (object_class, PROP_CAMERAS,
                                     properties[PROP_CAMERAS]);
}

static void
gc_camera_manager_init (GcCameraManager *self)
{
}

static gboolean
gc_camera_manager_initable_init (GInitable *initable,
                                 GCancellable *cancellable,
                                 GError **error)
{
    GcCameraManagerPrivate *priv;

    priv = gc_camera_manager_get_instance_private (GC_CAMERA_MANAGER (initable));

    priv->clients = g_hash_table_new_full (g_str_hash,
                                           g_str_equal,
                                           g_free,
                                           g_object_unref);

    return g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (initable),
                                             priv->connection,
                                             "/org/gnome/Camera/Manager",
                                             error);
}

static void
gc_camera_manager_manager_iface_init (GcManagerIface *iface)
{
    iface->handle_get_client = gc_camera_manager_handle_get_client;
}

static void
gc_camera_manager_initable_iface_init (GInitableIface *iface)
{
    iface->init = gc_camera_manager_initable_init;
}

GcCameraManager *
gc_camera_manager_new (GDBusConnection *connection, GError **error)
{
    return g_initable_new (GC_TYPE_CAMERA_MANAGER, NULL, error, "connection",
                           connection, NULL);
}
