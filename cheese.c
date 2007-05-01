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
#include <gdk-pixbuf/gdk-pixbuf.h>

#define _(str) gettext(str)

#define GLADE_FILE  "cheese.glade"

#define SAVE_FOLDER_DEFAULT  	 "images/"
#define PHOTO_NAME_DEFAULT	 "Picture"
#define PHOTO_NAME_SUFFIX_DEFAULT ".jpg"
#define PHOTO_WIDTH 640
#define PHOTO_HEIGHT 480
#define PICTURE_QUALITY 80

void on_cheese_window_close_cb (GtkWidget *widget, gpointer data);
static gboolean expose_cb(GtkWidget * widget, GdkEventExpose * event, gpointer data);
static void take_photo(GtkWidget * widget, GdkEventExpose * event, gpointer data);
static void create_photo(unsigned char *data);
static gboolean cb_have_data(GstElement *element, GstBuffer * buffer, GstPad* pad, gpointer user_data);
gchar *get_cheese_path();
gchar *get_cheese_filename(int i);
static void gst_set_play();
static void gst_set_stop();
void show_thumbs();

GladeXML *gxml;     
static GtkWidget *app_window; 
GstElement *pipeline;
int picture_requested = 0;

int main(int argc, char **argv)
{
  GtkWidget *take_picture;
  GtkWidget *screen;
  GstElement *source;
  GstElement *ffmpeg1, *ffmpeg2, *ffmpeg3;
  GstElement *ximagesink;
  GstElement *tee, *fakesink;
  GstElement *queuevid, *queueimg;
  GstElement *caps;
  GstCaps *filter;
  gboolean link_ok;

  gtk_init(&argc, &argv);
  glade_init();
  gst_init(&argc, &argv);
  gnome_vfs_init();

  gxml = glade_xml_new(GLADE_FILE, NULL, NULL);
  app_window = glade_xml_get_widget(gxml, "cheese_window");
  take_picture = glade_xml_get_widget(gxml, "take_picture");
  screen = glade_xml_get_widget(gxml, "screen");

  gtk_window_set_title(GTK_WINDOW(app_window), _("Cheese"));

  g_signal_connect(G_OBJECT(app_window), "destroy", 
      G_CALLBACK(on_cheese_window_close_cb), NULL);
  g_signal_connect(G_OBJECT(take_picture), "clicked",
      G_CALLBACK(take_photo), NULL);

  gtk_widget_set_size_request(screen, PHOTO_WIDTH, PHOTO_HEIGHT);

  gtk_widget_show_all(GTK_WIDGET(app_window));

  pipeline = gst_pipeline_new ("pipeline");
  source = gst_element_factory_make ("gconfvideosrc", "v4l2src");
  //source = gst_element_factory_make ("videotestsrc", "v4l2src");
  gst_bin_add(GST_BIN(pipeline), source);
  ffmpeg1 = gst_element_factory_make ("ffmpegcolorspace", "ffmpegcolorspace");
  gst_bin_add(GST_BIN(pipeline), ffmpeg1);
  ffmpeg2 = gst_element_factory_make("ffmpegcolorspace", "ffmpegcolorspace2");
  gst_bin_add(GST_BIN(pipeline), ffmpeg2);
  ffmpeg3 = gst_element_factory_make("ffmpegcolorspace", "ffmpegcolorspace3");
  gst_bin_add(GST_BIN(pipeline), ffmpeg3);
  queuevid = gst_element_factory_make("queue", "queuevid");
  gst_bin_add(GST_BIN(pipeline), queuevid);
  queueimg = gst_element_factory_make("queue", "queueimg");
  gst_bin_add(GST_BIN(pipeline), queueimg);
  caps = gst_element_factory_make("capsfilter", "caps");
  gst_bin_add(GST_BIN(pipeline), caps);
  ximagesink = gst_element_factory_make ("gconfvideosink", "ximagesink");
  gst_bin_add(GST_BIN(pipeline), ximagesink);
  tee = gst_element_factory_make ("tee", "tee");
  gst_bin_add(GST_BIN(pipeline), tee);
  fakesink = gst_element_factory_make ("fakesink", "fakesink");
  gst_bin_add(GST_BIN(pipeline), fakesink);

  //gst_bin_add_many(GST_BIN (pipeline), source, ffmpeg, ffmpeg2, tee, queuevid, queueimg, ximagesink, fakesink, NULL);
  //gst_element_link_many (source, filter, sink , NULL);

  gst_element_link(source, ffmpeg1);
  //effect
  gst_element_link(ffmpeg1, ffmpeg2);

  filter = gst_caps_new_simple("video/x-raw-rgb",
      "width", G_TYPE_INT, PHOTO_WIDTH,
      "height", G_TYPE_INT, PHOTO_HEIGHT,
      "framerate", GST_TYPE_FRACTION, 30, 1,
      "bpp", G_TYPE_INT, 24,
      "depth", G_TYPE_INT, 24, NULL);

  link_ok = gst_element_link_filtered(ffmpeg2, tee, filter);
  if (!link_ok) {
    g_warning("Failed to link elements !");
  }
  filter = gst_caps_new_simple("video/x-raw-yuv", NULL);

  gst_element_link(tee, queuevid);
  gst_element_link(queuevid, ffmpeg3);

  gst_element_link(ffmpeg3, ximagesink);

  gst_element_link(tee, queueimg);
  gst_element_link(queueimg, fakesink);

  g_object_set (G_OBJECT (fakesink), "signal-handoffs", TRUE, NULL);

  g_signal_connect (fakesink, "handoff", G_CALLBACK (cb_have_data), NULL);
  g_signal_connect(screen, "expose-event", G_CALLBACK(expose_cb), ximagesink);

  gst_set_play();
  gtk_main();

  return EXIT_SUCCESS;
}

void on_cheese_window_close_cb (GtkWidget *widget, gpointer data)
{
  gst_set_stop();
  gst_object_unref(pipeline);
  gtk_main_quit();
}

static gboolean expose_cb(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  GstElement *tmp = gst_bin_get_by_interface(GST_BIN(pipeline), GST_TYPE_X_OVERLAY);
  gst_x_overlay_set_xwindow_id(GST_X_OVERLAY(tmp),
  //gst_x_overlay_set_xwindow_id(GST_X_OVERLAY(data),
      GDK_WINDOW_XWINDOW(widget->window));
  return FALSE;
}

static void take_photo(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  picture_requested = 1;
}

static gboolean cb_have_data(GstElement *element, GstBuffer *buffer, GstPad *pad, gpointer user_data)
{
  unsigned char *data_photo = (unsigned char *) GST_BUFFER_DATA(buffer);
  if (picture_requested) {
    picture_requested = 0;
    create_photo(data_photo);
  }
  return TRUE;
}

static void create_photo(unsigned char *data)
{
  int i;
  gchar *filename = NULL;
  gchar *path = NULL;
  GnomeVFSURI *uri;
  GdkPixbuf *pixbuf;

  path = get_cheese_path();
  uri = gnome_vfs_uri_new(path); 

  if (!gnome_vfs_uri_exists(uri)) {
    gnome_vfs_make_directory_for_uri(uri, 0775);
    printf("creating directory: %s\n", path);
  }

  i = 1;
  filename = get_cheese_filename(i);

  uri = gnome_vfs_uri_new(filename);

  while (gnome_vfs_uri_exists(uri)) {
    i++;
    filename = get_cheese_filename(i);
    g_free(uri);
    uri = gnome_vfs_uri_new(filename);
  }
  g_free(uri);

  pixbuf = gdk_pixbuf_new_from_data (data, GDK_COLORSPACE_RGB, FALSE, 8, PHOTO_WIDTH, PHOTO_HEIGHT, PHOTO_WIDTH * 3, NULL, NULL);
  gdk_pixbuf_save (pixbuf, filename, "jpeg", NULL, NULL);
  //gdk_pixbuf_save (pixbuf, "ff.png", "png", NULL, NULL);
  g_object_unref(G_OBJECT(pixbuf));

  printf("Photo saved: %s\n", filename);
}

gchar *get_cheese_path() {
  //FIXME: check for real path
  // maybe ~/cheese or on the desktop..
  //g_get_home_dir ()
  gchar *path = g_strdup_printf("%s/%s", getenv("PWD"), SAVE_FOLDER_DEFAULT);
  return path;
}

gchar *get_cheese_filename(int i) {
  gchar *filename = g_strdup_printf("%s%s%d%s", get_cheese_path(), PHOTO_NAME_DEFAULT, i, PHOTO_NAME_SUFFIX_DEFAULT);
  return filename;
}

void gst_set_play() {
  gst_element_set_state(pipeline , GST_STATE_PLAYING);
}

void gst_set_stop() {
  gst_element_set_state(pipeline , GST_STATE_NULL);
}
