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

#include "gc-camera-client.h"
#include "cheese-avatar-chooser.h"

typedef struct
{
    GDBusConnection *connection;
    gchar *path;
    GtkWidget *chooser;
    gchar *image_data;
} GcCameraClientPrivate;

static void gc_camera_client_client_iface_init (GcClientIface *iface);
static void gc_camera_client_initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (GcCameraClient, gc_camera_client,
                         GC_TYPE_CLIENT_SKELETON,
                         G_ADD_PRIVATE (GcCameraClient)
                         G_IMPLEMENT_INTERFACE (GC_TYPE_CLIENT, gc_camera_client_client_iface_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, gc_camera_client_initable_iface_init))

enum
{
    PROP_0,
    PROP_CONNECTION,
    PROP_PATH,
    PROP_IMAGE_DATA,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

static void
gc_camera_client_set_image_data_from_pixbuf (GcCameraClient *self,
                                             GdkPixbuf *pixbuf)
{
    GcCameraClientPrivate *priv;
    gchar *buffer;
    gsize length;

    priv = gc_camera_client_get_instance_private (self);

    /* FIXME: Check for errors. */
    if (!gdk_pixbuf_save_to_buffer (pixbuf, &buffer, &length, "png", NULL, NULL))
    {
        g_error ("%s", "GdkPixbuf could not be saved to PNG format");
    }

    g_free (priv->image_data);
    priv->image_data = g_base64_encode ((const guchar *)buffer, length);
    g_free (buffer);

    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_IMAGE_DATA]);
}

static void
on_chooser_response (CheeseAvatarChooser *chooser,
                     gint response,
                     GcCameraClient *self)
{
    GcCameraClientPrivate *priv;

    priv = gc_camera_client_get_instance_private (self);

    if (response == GTK_RESPONSE_ACCEPT)
    {
        gc_camera_client_set_image_data_from_pixbuf (self,
                                                     cheese_avatar_chooser_get_picture (chooser));
        gc_client_emit_user_done (GC_CLIENT (self), TRUE, priv->image_data);
    }
    else
    {
        gc_client_emit_user_done (GC_CLIENT (self), FALSE, priv->image_data);
    }

    /* FIXME: Destroy the chooser, as there is no way to stop the webcam
     * viewfinder. */
    gtk_widget_hide (GTK_WIDGET (chooser));
}

static gboolean
gc_camera_client_handle_show_chooser (GcClient *client,
                                      GDBusMethodInvocation *invocation)
{
    GcCameraClientPrivate *priv;

    priv = gc_camera_client_get_instance_private (GC_CAMERA_CLIENT (client));

    gtk_widget_show (priv->chooser);

    gc_client_complete_show_chooser (client, invocation);

    return TRUE;
}

static const gchar *
gc_camera_client_get_image_data (GcClient *client)
{
    GcCameraClient *self;
    GcCameraClientPrivate *priv;

    self = GC_CAMERA_CLIENT (client);
    priv = gc_camera_client_get_instance_private (self);

    return priv->image_data;
}

const gchar *
gc_camera_client_get_path (GcCameraClient *client)
{
    GcCameraClientPrivate *priv;

    g_return_val_if_fail (GC_CAMERA_CLIENT (client), NULL);

    priv = gc_camera_client_get_instance_private (client);

    return priv->path;
}

static void
gc_camera_client_finalize (GObject *object)
{
    GcCameraClientPrivate *priv;

    priv = gc_camera_client_get_instance_private (GC_CAMERA_CLIENT (object));

    g_clear_object (&priv->chooser);
    g_clear_pointer (&priv->image_data, g_free);

    g_free (priv->path);

    G_OBJECT_CLASS (gc_camera_client_parent_class)->finalize (object);
}

static void
gc_camera_client_get_property (GObject *object,
                               guint prop_id,
                               GValue *value,
                               GParamSpec *pspec)
{
    GcCameraClientPrivate *priv;

    priv = gc_camera_client_get_instance_private (GC_CAMERA_CLIENT (object));

    switch (prop_id)
    {
        case PROP_CONNECTION:
            g_value_set_object (value, priv->connection);
            break;
        case PROP_PATH:
            g_value_set_string (value, priv->path);
            break;
        case PROP_IMAGE_DATA:
            g_value_set_string (value, priv->image_data);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gc_camera_client_set_property (GObject *object,
                               guint prop_id,
                               const GValue *value,
                               GParamSpec *pspec)
{
    GcCameraClientPrivate *priv;

    priv = gc_camera_client_get_instance_private (GC_CAMERA_CLIENT (object));

    switch (prop_id)
    {
        case PROP_CONNECTION:
            priv->connection = g_value_dup_object (value);
            break;
        case PROP_PATH:
            g_free (priv->path);
            priv->path = g_value_dup_string (value);
            break;
        case PROP_IMAGE_DATA:
            g_free (priv->image_data);
            priv->image_data = g_value_dup_string (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gc_camera_client_class_init (GcCameraClientClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = gc_camera_client_finalize;
    object_class->get_property = gc_camera_client_get_property;
    object_class->set_property = gc_camera_client_set_property;

    properties[PROP_CONNECTION] = g_param_spec_object ("connection",
                                                       "Connection",
                                                       "DBus Connection",
                                                       G_TYPE_DBUS_CONNECTION,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_STRINGS);
    g_object_class_install_property (object_class, PROP_CONNECTION,
                                     properties[PROP_CONNECTION]);

    properties[PROP_PATH] = g_param_spec_string ("path",
                                                 "Path",
                                                 "D-Bus object path",
                                                 NULL,
                                                 G_PARAM_READWRITE |
                                                 G_PARAM_CONSTRUCT_ONLY |
                                                 G_PARAM_STATIC_STRINGS);
    g_object_class_install_property (object_class, PROP_PATH,
                                     properties[PROP_PATH]);

    properties[PROP_IMAGE_DATA] = g_param_spec_string ("image-data",
                                                       "Image data",
                                                       "PNG image data, Base64-encoded",
                                                       NULL,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_STATIC_STRINGS);
    g_object_class_install_property (object_class, PROP_IMAGE_DATA,
                                     properties[PROP_IMAGE_DATA]);
}

static void
gc_camera_client_init (GcCameraClient *self)
{
    GcCameraClientPrivate *priv;

    priv = gc_camera_client_get_instance_private (self);

    priv->chooser = cheese_avatar_chooser_new ();

    g_signal_connect (priv->chooser, "response",
                      G_CALLBACK (on_chooser_response), self);
}

static gboolean
gc_camera_client_initable_init (GInitable *initable,
                                GCancellable *cancellable,
                                GError **error)
{
    GcCameraClientPrivate *priv;

    priv = gc_camera_client_get_instance_private (GC_CAMERA_CLIENT (initable));

    return g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (initable),
                                             priv->connection,
                                             priv->path,
                                             error);
}

static void
gc_camera_client_client_iface_init (GcClientIface *iface)
{
    iface->handle_show_chooser = gc_camera_client_handle_show_chooser;
    iface->get_image_data = gc_camera_client_get_image_data;
}

static void
gc_camera_client_initable_iface_init (GInitableIface *iface)
{
    iface->init = gc_camera_client_initable_init;
}

GcCameraClient *
gc_camera_client_new (GDBusConnection *connection,
                      const gchar *path,
                      GError **error)
{
    return g_initable_new (GC_TYPE_CAMERA_CLIENT, NULL, error, "connection",
                           connection, "path", path, NULL);
}
