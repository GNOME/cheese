/*
 * Copyright (C) 2007 daniel g. siegel <dgsiegel@gmail.com>
 * Copyright (C) 2007 Jaap Haitsma <jaap@haitsma.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include <libgnomevfs/gnome-vfs.h>

#include "cheese-window.h"

const int verbose = TRUE;

void cheese_printerr_handler(char *string)
{
  if (verbose) 
    fprintf (stdout, "%s\n", string);
}

int
main (int argc, char **argv)
{
  g_thread_init (NULL);
  gtk_init (&argc, &argv);
  gst_init (&argc, &argv);
  gnome_vfs_init ();

  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  g_set_application_name (_("Cheese"));

  gtk_window_set_default_icon_name ("cheese");

  g_set_print_handler ((GPrintFunc) cheese_printerr_handler);

  cheese_window_init ();
  
  gtk_main ();

  return 0;
}
