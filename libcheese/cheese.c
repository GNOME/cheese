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

#include <clutter-gst/clutter-gst.h>

#include "cheese.h"

/**
 * SECTION:cheese-init
 * @short_description: Initialize libcheese
 * @stability: Unstable
 * @include: cheese/cheese.h
 *
 * Call cheese_init() to initialize libcheese.
 */


/**
 * cheese_init:
 * @argc: (allow-none): pointer to the argument list count
 * @argv: (allow-none): pointer to the argument list vector
 *
 * Initialize libcheese, by initializing Clutter and GStreamer.
 *
 * Returns: %TRUE if the initialization was successful, %FALSE otherwise
 */
gboolean
cheese_init (int *argc, char ***argv)
{
    ClutterInitError error;

    error = clutter_gst_init (argc, argv);

    if (error != CLUTTER_INIT_SUCCESS)
        return FALSE;

    return TRUE;
}
