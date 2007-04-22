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
static void take_photo(GtkWidget * widget, GdkEventExpose * event, gpointer data);
static void create_jpeg(unsigned char *data);
static gboolean cb_have_data(GstElement *element, GstBuffer * buffer, GstPad* pad, gpointer user_data);

GladeXML *gxml;     
static GtkWidget *app_window; 
GstElement *pipeline;
int picture_requested = 0;

int main(int argc, char **argv)
{
  GtkWidget *take_picture;
  GtkWidget *screen;
  GMainLoop *loop;
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

  loop = g_main_loop_new (NULL , FALSE);

  gxml = glade_xml_new(GLADE_FILE, NULL, NULL);
  app_window = glade_xml_get_widget(gxml, "cheese_window");
  take_picture = glade_xml_get_widget(gxml, "take_picture");
  screen = glade_xml_get_widget(gxml, "screen");

  gtk_window_set_title(GTK_WINDOW(app_window), _("Cheese"));

  g_signal_connect(G_OBJECT(app_window), "destroy", 
      G_CALLBACK(on_cheese_window_close_cb), NULL);
 	g_signal_connect(G_OBJECT(take_picture), "clicked",
			 G_CALLBACK(take_photo), NULL);

  gtk_widget_set_size_request(screen, 640, 480);

  gtk_widget_show_all(GTK_WIDGET(app_window));

  pipeline = gst_pipeline_new ("pipeline");
  source = gst_element_factory_make ("v4l2src", "v4l2src");
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
  ximagesink = gst_element_factory_make ("ximagesink", "ximagesink");
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
      "width", G_TYPE_INT, 640,
      "height", G_TYPE_INT, 480,
      "framerate", GST_TYPE_FRACTION, 30, 1,
      "bpp", G_TYPE_INT, 24,
      "depth", G_TYPE_INT, 24, NULL);

  link_ok = gst_element_link_filtered(ffmpeg2, tee, filter);
	if (!link_ok) {
		g_warning("Failed to link elements !");
	}
  //filter = gst_caps_new_simple("video/x-raw-yuv", NULL);

  gst_element_link(tee, queuevid);
	gst_element_link(queuevid, ffmpeg3);

  gst_element_link(ffmpeg3, ximagesink);

  gst_element_link(tee, queueimg);
  gst_element_link(queueimg, fakesink);

  gst_element_set_state (pipeline , GST_STATE_PLAYING);
  g_object_set (G_OBJECT (fakesink), "signal-handoffs", TRUE, NULL);

	g_signal_connect (fakesink, "handoff", G_CALLBACK (cb_have_data), NULL);

  g_signal_connect(screen, "expose-event", G_CALLBACK(expose_cb), ximagesink);

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

static gboolean expose_cb(GtkWidget * widget, GdkEventExpose * event, gpointer data)
{
  gst_x_overlay_set_xwindow_id(GST_X_OVERLAY(data),
      GDK_WINDOW_XWINDOW(widget->window));
  return FALSE;
}

static void take_photo(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  printf("clicked\n");
    picture_requested = 1;
}

static gboolean cb_have_data(GstElement *element, GstBuffer * buffer, GstPad* pad, gpointer user_data)
{
	unsigned char *data_photo = (unsigned char *) GST_BUFFER_DATA(buffer);
	if (picture_requested) {
		picture_requested = 0;
		create_jpeg(data_photo);
	}
	return TRUE;
}

static void create_jpeg(unsigned char *data)
{
	/* Boilerplate function that gets a framebuffer and writes a JPEG file */
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	int i, width, height;
	unsigned char *line;
	int byte_pixel;
	FILE *outfile;
	gchar *filename = NULL;
	char aux[256];
	GnomeVFSURI *test_uri = NULL;

	i = 1;
	filename =
	    g_build_filename(getenv("MYDOCSDIR"), SAVE_FOLDER_DEFAULT,
			     PHOTO_NAME_DEFAULT, NULL);

	sprintf(aux, "%s%d%s", PHOTO_NAME_DEFAULT, i, PHOTO_NAME_SUFFIX_DEFAULT);
	printf("%s%d%s", PHOTO_NAME_DEFAULT, i, PHOTO_NAME_SUFFIX_DEFAULT);
	test_uri = gnome_vfs_uri_new(aux);

	while (gnome_vfs_uri_exists(test_uri)) {
		i++;
		sprintf(aux, "%s%d%s", PHOTO_NAME_DEFAULT, i,
			PHOTO_NAME_SUFFIX_DEFAULT);
		g_free(test_uri);
		test_uri = gnome_vfs_uri_new(aux);
	}

	filename = aux;
	g_free(test_uri);

	width = 640;
	height = 480;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	if ((outfile = fopen(filename, "wb")) == NULL) {
		fprintf(stderr, "can't open %s\n", filename);
		exit(1);
	}

	jpeg_stdio_dest(&cinfo, outfile);

	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, 100, TRUE);

	jpeg_start_compress(&cinfo, TRUE);

	byte_pixel = 3;

	for (i = 0, line = data; i < height;
	     i++, line += width * byte_pixel) {
		jpeg_write_scanlines(&cinfo, &line, 1);
	}
	jpeg_finish_compress(&(cinfo));
	fclose(outfile);
	jpeg_destroy_compress(&(cinfo));
	printf("Photo Saved\n");
}
