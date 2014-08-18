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

#include <gio/gio.h>
#include <clutter-gtk/clutter-gtk.h>

#include "cheese-gtk.h"
#include "gc-application.h"

int
main (int argc,
      char *argv[])
{
    GApplication *application;
    int status;

    if (!cheese_gtk_init (&argc, &argv))
    {
        return 1;
    }

    g_set_prgname (PACKAGE_TARNAME);
    application = G_APPLICATION (gc_application_new ());
    status = g_application_run (application, argc, argv);
    g_object_unref (application);

    return status;
}
