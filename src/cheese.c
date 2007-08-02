/*
 * Copyright (C) 2007 Copyright (C) 2007 daniel g. siegel <dgsiegel@gmail.com>
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

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <libgnomevfs/gnome-vfs.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "cheese-config.h"
#include "cheese.h"
#include "cheese-pipeline-photo.h"
#include "cheese-fileutil.h"
#include "cheese-thumbnails.h"
#include "cheese-window.h"
#include "cheese-effects-widget.h"

struct _cheese_window cheese_window;
struct _thumbnails thumbnails;

GnomeVFSMonitorHandle *monitor_handle = NULL;
Pipeline *PipelinePhoto;

void
on_cheese_window_close_cb(GtkWidget *widget, gpointer data)
{
  pipeline_set_stop(PipelinePhoto);
  g_object_unref(G_OBJECT(PipelinePhoto));
  gnome_vfs_monitor_cancel(monitor_handle);

  cheese_effects_widget_finalize();
  cheese_window_finalize();
  cheese_thumbnails_finalize();
  cheese_fileutil_finalize();

  gtk_main_quit();
}

int
main(int argc, char **argv)
{
  gchar *path = NULL;
  GnomeVFSURI *uri;

  g_thread_init (NULL);
  gtk_init(&argc, &argv);
  gst_init(&argc, &argv);
  gnome_vfs_init();
  g_type_init();

  bindtextdomain(CHEESE_PACKAGE_NAME, CHEESE_LOCALE_DIR);
  textdomain(CHEESE_PACKAGE_NAME);

  g_set_application_name (_("Cheese"));

  gtk_window_set_default_icon_name("cheese");

  path = cheese_fileutil_get_photo_path();
  uri = gnome_vfs_uri_new(path);

  if (!gnome_vfs_uri_exists(uri)) {
    gnome_vfs_make_directory_for_uri(uri, 0775);
    g_mkdir_with_parents(path, 0775);
    g_print("creating new directory: %s\n", path);
  }

  cheese_window_init();

  cheese_effects_widget_init();

  cheese_thumbnails_init();
  gtk_icon_view_set_model(GTK_ICON_VIEW(thumbnails.iconview),
      GTK_TREE_MODEL(thumbnails.store));
  cheese_thumbnails_fill_thumbs();

  gnome_vfs_monitor_add(&monitor_handle, cheese_fileutil_get_photo_path(),
      GNOME_VFS_MONITOR_DIRECTORY,
      (GnomeVFSMonitorCallback)cheese_fileutil_photos_monitor_cb, NULL);

  PipelinePhoto = PIPELINE(pipeline_new());
  pipeline_create(PipelinePhoto);

  gtk_widget_show_all(cheese_window.window);

  pipeline_set_play(PipelinePhoto);

  g_signal_connect(G_OBJECT(cheese_window.window), "destroy",
      G_CALLBACK(on_cheese_window_close_cb), NULL);
  g_signal_connect(G_OBJECT(cheese_window.widgets.take_picture), "clicked",
      G_CALLBACK(pipeline_button_clicked), PipelinePhoto);
  g_signal_connect(cheese_window.widgets.screen, "expose-event",
      G_CALLBACK(cheese_window_expose_cb), PipelinePhoto);
  g_signal_connect(G_OBJECT(cheese_window.widgets.button_effects), "clicked",
      G_CALLBACK(cheese_window_change_effect), PipelinePhoto);

  gtk_main();

  return EXIT_SUCCESS;
}
