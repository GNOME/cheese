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

#include "gc-application.h"

#include "gc-camera-manager.h"

typedef struct
{
    GcCameraManager *manager;
} GcApplicationPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GcApplication, gc_application, G_TYPE_APPLICATION)

static gboolean
gc_application_dbus_register (GApplication *application,
                              GDBusConnection *connection,
                              const gchar *object_path,
                              GError **error)
{
    GcApplicationPrivate *priv;

    priv = gc_application_get_instance_private (GC_APPLICATION (application));

    if (!G_APPLICATION_CLASS (gc_application_parent_class)->dbus_register (application,
                                                                           connection,
                                                                           object_path,
                                                                           error))
    {
        return FALSE;
    }

    priv->manager = gc_camera_manager_new (connection, error);

    if (priv->manager == NULL)
    {
        g_warning ("Error exporting camera interface skeleton: %s",
                   (*error)->message);
        return FALSE;
    }

    return TRUE;
}

static void
gc_application_dbus_unregister (GApplication *application,
                                GDBusConnection *connection,
                                const gchar *object_path)
{
    GcApplicationPrivate *priv;

    priv = gc_application_get_instance_private (GC_APPLICATION (application));

    /* TODO: Not necessary? */
    g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (priv->manager));

    G_APPLICATION_CLASS (gc_application_parent_class)->dbus_unregister (application,
                                                                        connection,
                                                                        object_path);
}

static void
gc_application_finalize (GObject *object)
{
    GcApplication *application;
    GcApplicationPrivate *priv;

    application = GC_APPLICATION (object);
    priv = gc_application_get_instance_private (application);

    g_clear_object (&priv->manager);
}

static void
gc_application_init (GcApplication *application)
{
}

static void
gc_application_class_init (GcApplicationClass *klass)
{
    GObjectClass *gobject_class;
    GApplicationClass *app_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = gc_application_finalize;

    app_class = G_APPLICATION_CLASS (klass);
    app_class->dbus_register = gc_application_dbus_register;
    app_class->dbus_unregister = gc_application_dbus_unregister;
}

GcApplication *
gc_application_new (void)
{
    return g_object_new (GC_TYPE_APPLICATION, "application-id",
                         "org.gnome.Camera", "flags", G_APPLICATION_IS_SERVICE,
                         NULL);
}
