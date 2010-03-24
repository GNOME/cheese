/*
 * Copyright © 2007-2009 daniel g. siegel <dgsiegel@gnome.org>
 * Copyright © 2007,2008 Jaap Haitsma <jaap@haitsma.org>
 * Copyright © 2008 Felix Kaser <f.kaser@gmx.net>
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
  #include <cheese-config.h>
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include <librsvg/rsvg.h>

#include "cheese-fileutil.h"
#include "cheese-window.h"
#include "cheese-dbus.h"

struct _CheeseOptions
{
  gboolean verbose;
  gboolean wide_mode;
  gboolean version;
} CheeseOptions;

void
cheese_print_handler (char *string)
{
  static FILE *fp = NULL;
  GDir        *dir;
  char        *filename, *path;

  CheeseFileUtil *fileutil = cheese_fileutil_new ();

  if (CheeseOptions.verbose)
    fprintf (stdout, "%s", string);

  if (fp == NULL)
  {
    path = cheese_fileutil_get_log_path (fileutil);

    dir = g_dir_open (path, 0, NULL);
    if (!dir)
    {
      return;
    }

    /* remove the old logfile if it exists */
    filename = g_build_filename (path, "log", NULL);
    if (g_file_test (filename, G_FILE_TEST_EXISTS))
    {
      GFile *old = g_file_new_for_path (filename);
      g_file_delete (old, NULL, NULL);
      g_object_unref (old);
    }
    g_free (filename);

    filename = g_build_filename (path, "log.txt", NULL);
    fp       = fopen (filename, "w");
    fputs ("Cheese " VERSION "\n\n", fp);

    g_object_unref (fileutil);
    g_free (filename);
  }

  if (fp)
    fputs (string, fp);
}

int
main (int argc, char **argv)
{
  GOptionContext *context;
  CheeseDbus     *dbus_server;
  GError         *error = NULL;

  GOptionEntry options[] = {
    {"verbose",    'v', 0,                    G_OPTION_ARG_NONE,   &CheeseOptions.verbose,
     _("Be verbose"), NULL},
    {"wide",       'w', 0,                    G_OPTION_ARG_NONE,   &CheeseOptions.wide_mode,
     _("Enable wide mode"), NULL},
    {"version",    0,   0,                    G_OPTION_ARG_NONE,   &CheeseOptions.version,
     _("output version information and exit"), NULL},
    {NULL}
  };

  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  gtk_rc_parse (APPNAME_DATA_DIR G_DIR_SEPARATOR_S "gtkrc");

  g_thread_init (NULL);
  gdk_threads_init ();

  /* initialize rsvg */
  /* needed to load the camera icon for the countdown widget */
  rsvg_init ();

  g_set_application_name (_("Cheese"));

  context = g_option_context_new (N_("- Take photos and videos with your webcam, with fun graphical effects"));
  g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));
  g_option_context_add_group (context, gst_init_get_option_group ());
  if (g_option_context_parse (context, &argc, &argv, &error) == FALSE)
  {
    gchar *help_text = g_option_context_get_help (context, TRUE, NULL);
    g_print ("%s\n\n%s", error->message, help_text);
    g_free (help_text);
    g_error_free (error);
    g_option_context_free (context);
    return -1;
  }
  g_option_context_free (context);

  if (CheeseOptions.version)
  {
    g_print ("Cheese " VERSION " \n");
    return 0;
  }

  dbus_server = cheese_dbus_new ();
  if (dbus_server == NULL)
  {
    gdk_notify_startup_complete ();
    return -1;
  }

  g_set_print_handler ((GPrintFunc) cheese_print_handler);
  g_print ("Cheese " VERSION " \n");

  gtk_window_set_default_icon_name ("cheese");
  gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (),
                                     APPNAME_DATA_DIR G_DIR_SEPARATOR_S "icons");

  CheeseWindow *window = g_object_new (CHEESE_TYPE_WINDOW,
                                       "startup-wide", CheeseOptions.wide_mode,
                                       NULL);

  cheese_dbus_set_window (window);

  gtk_widget_show (GTK_WIDGET (window));

  gdk_threads_enter ();
  gtk_main ();
  gdk_threads_leave ();

  /* cleanup rsvg */
  /* Note: this function is bad with multithread applications as it
   * calls xmlCleanupParser() and should be only called right before
   * exit */
  rsvg_term ();

  return 0;
}
