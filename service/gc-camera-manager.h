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

#ifndef GC_CAMERA_MANAGER_H_
#define GC_CAMERA_MANAGER_H_

#include "camera.h"

G_BEGIN_DECLS

typedef struct
{
    /*< private >*/
    GcManagerSkeleton parent_instance;
} GcCameraManager;

typedef struct
{
    GcManagerSkeletonClass parent_class;
} GcCameraManagerClass;

#define GC_TYPE_CAMERA_MANAGER (gc_camera_manager_get_type ())
#define GC_CAMERA_MANAGER(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GC_TYPE_CAMERA_MANAGER, GcCameraManager))

GType gc_camera_manager_get_type (void);
GcCameraManager * gc_camera_manager_new (GDBusConnection *connection, GError **error);

G_END_DECLS

#endif /* GC_CAMERA_MANAGER_H_ */
