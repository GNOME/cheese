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

#ifndef GC_CAMERA_CLIENT_H_
#define GC_CAMERA_CLIENT_H_

#include "camera.h"

G_BEGIN_DECLS

typedef struct
{
    /*< private >*/
    GcClientSkeleton parent_instance;
} GcCameraClient;

typedef struct
{
    GcClientSkeletonClass parent_class;
} GcCameraClientClass;

#define GC_TYPE_CAMERA_CLIENT (gc_camera_client_get_type ())
#define GC_CAMERA_CLIENT(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GC_TYPE_CAMERA_CLIENT, GcCameraClient))

GType gc_camera_client_get_type (void);
GcCameraClient * gc_camera_client_new (GDBusConnection *connection, const gchar *path, GError **error);
const gchar * gc_camera_client_get_path (GcCameraClient *client);

G_END_DECLS

#endif /* GC_CAMERA_CLIENT_H_ */
