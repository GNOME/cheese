/*
 * Copyright © 2007,2008 daniel g. siegel <dgsiegel@gnome.org>
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

#include "cheese-fileutil.h"
#include "cheese-window.h"
#include "cheese-dbus.h"

struct _CheeseOptions
{
  gboolean verbose;
  gboolean wide_mode;
  char *hal_device_id;
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

void
cheese_handle_files_from_before_224 (void)
{
  CheeseFileUtil *fileutil             = cheese_fileutil_new ();
  gchar          *photo_path           = cheese_fileutil_get_photo_path (fileutil);
  gchar          *video_path           = cheese_fileutil_get_video_path (fileutil);
  gchar          *path_from_before_224 = cheese_fileutil_get_path_before_224 (fileutil);
  GDir           *dir_from_before_224;
  gchar          *source_filename, *target_filename;
  const char     *name;
  GFile          *source, *target, *dir;

  if (g_file_test (path_from_before_224, G_FILE_TEST_IS_DIR) &&
      (g_strcmp0 (video_path, path_from_before_224) != 0) &&
      (g_strcmp0 (photo_path, path_from_before_224) != 0))
  {
    dir_from_before_224 = g_dir_open (path_from_before_224, 0, NULL);

    while ((name = g_dir_read_name (dir_from_before_224)))
    {
      /* the filenames from before 2.24 have different namings than the > 2.24
       * files (0042.jpg/ogg vs. 2008-09-09-015555.jpg/ogv) so we just copy and
       * rename the ogg files to ogv
       */
      if (g_str_has_suffix (name, VIDEO_NAME_SUFFIX))
      {
        target_filename = g_build_filename (video_path, name, NULL);
      }
      else if (g_str_has_suffix (name, ".ogg"))
      {
        gchar *filename  = g_strdup (name);
        gchar *extension = g_strrstr (filename, ".ogg");
        extension[3]    = 'v';
        target_filename = g_build_filename (video_path, filename, NULL);
        g_free (filename);
      }
      else if (g_str_has_suffix (name, PHOTO_NAME_SUFFIX))
      {
        target_filename = g_build_filename (photo_path, name, NULL);
      }
      else
      {
        continue;
      }

      source_filename = g_build_filename (path_from_before_224, name, NULL);
      source          = g_file_new_for_path (source_filename);
      target          = g_file_new_for_path (target_filename);
      g_print ("copying %s to %s\n", source_filename, target_filename);
      g_file_move (source, target, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, NULL);
      g_free (source_filename);
      g_free (target_filename);
      g_object_unref (source);
      g_object_unref (target);
    }

    g_dir_close (dir_from_before_224);

    dir = g_file_new_for_path (path_from_before_224);
    g_file_delete (dir, NULL, NULL);
    g_object_unref (dir);
  }
  g_free (path_from_before_224);
  g_object_unref (fileutil);
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
    {"hal-device", 'd', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &CheeseOptions.hal_device_id,
     NULL, NULL},
    {"version",    0,   0,                    G_OPTION_ARG_NONE,   &CheeseOptions.version,
     _("output version information and exit"), NULL},
    {NULL}
  };

  CheeseOptions.hal_device_id = NULL;

  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  gtk_rc_parse (APPNAME_DATA_DIR G_DIR_SEPARATOR_S "gtkrc");

  g_thread_init (NULL);
  gdk_threads_init ();

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

  cheese_handle_files_from_before_224 ();
  cheese_window_init (CheeseOptions.hal_device_id, dbus_server, CheeseOptions.wide_mode);

  gdk_threads_enter ();
  gtk_main ();
  gdk_threads_leave ();

  return 0;
}
