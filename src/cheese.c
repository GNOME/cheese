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

#include "cheese-config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include <libgnomevfs/gnome-vfs.h>

#include "cheese-window.h"
#include "cheese-gconf.h"

int
main (int argc, char **argv)
{
  g_thread_init (NULL);
  gtk_init (&argc, &argv);
  gst_init (&argc, &argv);
  gnome_vfs_init ();

  bindtextdomain (CHEESE_PACKAGE_NAME, CHEESE_LOCALE_DIR);
  textdomain (CHEESE_PACKAGE_NAME);

  g_set_application_name (_("Cheese"));

  gtk_window_set_default_icon_name ("cheese");

  CheeseGConf *gconf;
  gconf = cheese_gconf_new ();
  cheese_window_init ();
  
  gtk_main ();

  return 0;
}
