/*
 * Copyright (C) 2007 daniel g. siegel <dgsiegel@gmail.com>
 * All rights reserved. This software is released under the GPL2 licence.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <pwd.h>
#include <dirent.h>
#include <libintl.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <glade/glade.h>
#include <libgnomevfs/gnome-vfs.h>

#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>
#include <jpeglib.h>

#define _(str) gettext(str)

#define GLADE_FILE  "cheese.glade"

#define SAVE_FOLDER_DEFAULT  	 ".images/"
#define PHOTO_NAME_DEFAULT	 "Picture"
#define PHOTO_NAME_SUFFIX_DEFAULT ".jpg"

void on_cheese_window_close_cb (GtkWidget *widget, gpointer data);
static gboolean expose_cb(GtkWidget * widget, GdkEventExpose * event, gpointer data);

GladeXML *gxml;     
static GtkWidget *app_window; 
GstElement *pipeline;

int main(int argc, char **argv)
{
  GtkWidget *take_picture;
  GtkWidget *screen;
  GMainLoop *loop;
  GstElement *source, *filter, *sink;

  gtk_init(&argc, &argv);
  glade_init();
  gst_init(&argc, &argv);
  gnome_vfs_init();

  loop = g_main_loop_new (NULL , FALSE);

  gxml = glade_xml_new(GLADE_FILE, NULL, NULL);
  app_window = glade_xml_get_widget(gxml, "cheese_window");
  take_picture = glade_xml_get_widget(gxml, "take_picture");
  screen = glade_xml_get_widget(gxml, "screen");

  gtk_window_set_title(GTK_WINDOW(app_window), _("Cheese"));

  g_signal_connect(G_OBJECT(app_window), "destroy", 
      G_CALLBACK(on_cheese_window_close_cb), NULL);

  gtk_widget_set_size_request(screen, 500, 380);

  gtk_widget_show_all(GTK_WIDGET(app_window));

  pipeline = gst_pipeline_new ("mypipeline");
  source = gst_element_factory_make ("v4l2src", "mysrc");
  filter = gst_element_factory_make ("ffmpegcolorspace", "filter");
  sink = gst_element_factory_make ("ximagesink", "mysink");
  gst_bin_add_many(GST_BIN (pipeline), source, filter, sink , NULL);
  gst_element_link_many (source, filter, sink , NULL);
  gst_element_set_state (pipeline , GST_STATE_PLAYING);

  g_signal_connect(screen, "expose-event", G_CALLBACK(expose_cb), sink);


  gst_element_set_state(pipeline, GST_STATE_PLAYING);
  gtk_main();

  return EXIT_SUCCESS;
}

void on_cheese_window_close_cb (GtkWidget *widget, gpointer data)
{
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);
  gtk_main_quit();
}

  static gboolean
expose_cb(GtkWidget * widget, GdkEventExpose * event, gpointer data)
{

  gst_x_overlay_set_xwindow_id(GST_X_OVERLAY(data),
      GDK_WINDOW_XWINDOW(widget->window));
  return FALSE;
}

