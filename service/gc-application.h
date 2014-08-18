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

#ifndef GC_APPLICATION_H_
#define GC_APPLICATION_H_

#include <gio/gio.h>

G_BEGIN_DECLS

typedef struct
{
    /*< private >*/
    GApplication parent_instance;
} GcApplication;

typedef struct
{
    GApplicationClass parent_class;
} GcApplicationClass;

#define GC_TYPE_APPLICATION (gc_application_get_type ())
#define GC_APPLICATION(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GC_TYPE_APPLICATION, GcApplication))

GType gc_application_get_type (void);
GcApplication * gc_application_new (void);

G_END_DECLS

#endif /* GC_APPLICATION_H_ */
