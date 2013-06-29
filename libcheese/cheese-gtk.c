/*
 * Copyright © 2011 David King <amigadave@amigadave.com>
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

#include <gtk/gtk.h>
#ifdef GDK_WINDOWING_X11
  #include <X11/Xlib.h>
#endif
#include <clutter-gtk/clutter-gtk.h>

#include "cheese-gtk.h"
#include "cheese.h"

/**
 * SECTION:cheese-gtk-init
 * @short_description: Initialize libcheese-gtk
 * @stability: Unstable
 * @include: cheese/cheese-gtk.h
 *
 * Call cheese_gtk_init() to initialize libcheese-gtk.
 */


/**
 * cheese_gtk_init:
 * @argc: (allow-none): pointer to the argument list count
 * @argv: (allow-none): pointer to the argument list vector
 *
 * Initialize libcheese-gtk, by initializing Clutter, GStreamer and GTK+. This
 * automatically calls cheese_init(), initializing libcheese.
 *
 * Returns: %TRUE if the initialization was successful, %FALSE otherwise
 */
gboolean
cheese_gtk_init (int *argc, char ***argv)
{
    ClutterInitError error;

#ifdef GDK_WINDOWING_X11
    XInitThreads ();
#endif

    error = gtk_clutter_init (argc, argv);

    if (error != CLUTTER_INIT_SUCCESS)
        return FALSE;

    if (!cheese_init (argc, argv))
        return FALSE;

    return TRUE;
}
