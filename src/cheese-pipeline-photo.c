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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "cheese-config.h"

#include <gst/interfaces/xoverlay.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs.h>

#include "cheese.h"
#include "cheese-window.h"
#include "cheese-pipeline-photo.h"
#include "cheese-effects-widget.h"
#include "cheese-fileutil.h"

G_DEFINE_TYPE (Pipeline, pipeline, G_TYPE_OBJECT)

static GObjectClass *parent_class = NULL;

typedef struct _PipelinePrivate PipelinePrivate;

struct _PipelinePrivate
{
  int picture_requested;

  GstElement *source;
  GstElement *ffmpeg1, *ffmpeg2, *ffmpeg3;
  GstElement *tee;
  GstElement *queuevid, *queueimg;
  GstElement *caps;
  GstElement *effect;
  GstCaps *filter;
  GstElement *gst_test_pipeline;
};

#define PIPELINE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), PIPELINE_TYPE, PipelinePrivate))

// private methods
static gboolean cb_have_data(GstElement *element, GstBuffer *buffer, GstPad *pad, gpointer user_data);
static void pipeline_lens_open(Pipeline *self);
static void create_photo(unsigned char *data, int width, int height);
static gboolean build_test_pipeline(const gchar *pipeline_desc, GError **p_err, Pipeline *self);
static gboolean test_pipeline(const gchar *pipeline_desc, Pipeline *self);
//static void pipeline_error_dlg(const gchar *pipeline_desc, const gchar *error_message, Pipeline *self);
static void pipeline_error_print(const gchar *pipeline_desc, const gchar *error_message, Pipeline *self);

void
pipeline_finalize(GObject *object)
{
  PipelinePrivate *priv = PIPELINE_GET_PRIVATE(object);
  gst_caps_unref(priv->filter);

  (*parent_class->finalize) (object);
  return;
}

Pipeline *
pipeline_new(void)
{
  Pipeline *self = g_object_new(PIPELINE_TYPE, NULL);

  return self;
}

void
pipeline_class_init(PipelineClass *klass)
{
  GObjectClass *object_class;

  parent_class = g_type_class_peek_parent(klass);
  object_class = (GObjectClass*) klass;

  object_class->finalize = pipeline_finalize;
  g_type_class_add_private(klass, sizeof(PipelinePrivate));

  G_OBJECT_CLASS(klass)->finalize = (GObjectFinalizeFunc) pipeline_finalize;
}

void
pipeline_init(Pipeline *self)
{
}

void
pipeline_set_play(Pipeline *self)
{
  gst_element_set_state(self->pipeline, GST_STATE_PLAYING);
  return;
}

void
pipeline_set_stop(Pipeline *self)
{
  gst_element_set_state(self->pipeline, GST_STATE_NULL);
  return;
}

void
pipeline_button_clicked(GtkWidget *widget, gpointer self)
{
  pipeline_lens_open(self);
  return;
}

static void
pipeline_lens_open(Pipeline *self)
{
  PipelinePrivate *priv = PIPELINE_GET_PRIVATE(self);
  priv->picture_requested = TRUE;
  return;
}

GstElement *
pipeline_get_ximagesink(Pipeline *self)
{
  return self->ximagesink;
}

GstElement *
pipeline_get_fakesink(Pipeline *self)
{
  return self->fakesink;
}

GstElement *
pipeline_get_pipeline(Pipeline *self)
{
  return self->pipeline;
}

void
pipeline_change_effect(gpointer self)
{
  PipelinePrivate *priv = PIPELINE_GET_PRIVATE(self);
  gchar *effect = cheese_effects_get_selection();

  if (effect != NULL) {
    pipeline_set_stop(PIPELINE(self));

    gst_element_unlink(priv->ffmpeg1, priv->effect);
    gst_element_unlink(priv->effect, priv->ffmpeg2);
    gst_bin_remove(GST_BIN(PIPELINE(self)->pipeline), priv->effect);

    g_print("changing to effect: %s\n", effect);
    priv->effect = gst_parse_bin_from_description(effect, TRUE, NULL);
    gst_bin_add(GST_BIN(PIPELINE(self)->pipeline), priv->effect);

    gst_element_link(priv->ffmpeg1, priv->effect);
    gst_element_link(priv->effect, priv->ffmpeg2);

    pipeline_set_play(self);
  }
}

void
pipeline_create(Pipeline *self) {

  gboolean link_ok; 
  gchar *source_pipeline;

  PipelinePrivate *priv = PIPELINE_GET_PRIVATE(self);

  if (test_pipeline("v4l2src ! fakesink", self)) {
    //source_pipeline = g_strdup_printf ("%s", v4l2src);
    source_pipeline = "v4l2src";
  } else if (test_pipeline("v4lsrc ! video/x-raw-rgb,width=640,height=480 ! fakesink", self)) {
    source_pipeline = "v4lsrc ! ffmpegcolorspace ! video/x-raw-rgb,width=640,height=480";
  } else if (test_pipeline("v4lsrc ! video/x-raw-rgb,width=320,height=240 ! fakesink", self)) {
    source_pipeline = "v4lsrc ! ffmpegcolorspace ! video/x-raw-rgb,width=320,height=240";
  } else if (test_pipeline("v4lsrc ! video/x-raw-rgb,width=1280,height=960 ! fakesink", self)) {
    source_pipeline = "v4lsrc ! ffmpegcolorspace ! video/x-raw-rgb,width=1280,height=960";
  } else if (test_pipeline("v4lsrc ! video/x-raw-rgb,width=160,height=120 ! fakesink", self)) {
    source_pipeline = "v4lsrc ! ffmpegcolorspace ! video/x-raw-rgb,width=160,height=120";
  } else if (test_pipeline("v4lsrc ! fakesink", self)) {
    source_pipeline = "v4lsrc";
  } else {
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new(GTK_WINDOW(cheese_window.window),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
        _("Unable to find a webcam, SORRY!"));

  gtk_dialog_run(GTK_DIALOG (dialog));
  gtk_widget_destroy(dialog);
    source_pipeline = "videotestsrc";
  }

  g_print("using source: %s\n", source_pipeline);

  self->pipeline = gst_pipeline_new("pipeline");
  priv->source = gst_parse_bin_from_description(source_pipeline, TRUE, NULL);
  //priv->source = gst_element_factory_make(source_pipeline, source_pipeline);
  // if you want to test without having a webcam
  // priv->source = gst_element_factory_make("videotestsrc", "v4l2src");
  gst_bin_add(GST_BIN(self->pipeline), priv->source);

  priv->ffmpeg1 = gst_element_factory_make("ffmpegcolorspace", "ffmpegcolorspace");
  gst_bin_add(GST_BIN(self->pipeline), priv->ffmpeg1);

  priv->effect = gst_element_factory_make("identity", "effect");
  gst_bin_add(GST_BIN(self->pipeline), priv->effect);

  priv->ffmpeg2 = gst_element_factory_make("ffmpegcolorspace", "ffmpegcolorspace2");
  gst_bin_add(GST_BIN(self->pipeline), priv->ffmpeg2);

  priv->ffmpeg3 = gst_element_factory_make("ffmpegcolorspace", "ffmpegcolorspace3");
  gst_bin_add(GST_BIN(self->pipeline), priv->ffmpeg3);

  priv->tee = gst_element_factory_make("tee", "tee");
  gst_bin_add(GST_BIN(self->pipeline), priv->tee);

  priv->queuevid = gst_element_factory_make("queue", "queuevid");
  gst_bin_add(GST_BIN(self->pipeline), priv->queuevid);

  priv->queueimg = gst_element_factory_make("queue", "queueimg");
  gst_bin_add(GST_BIN(self->pipeline), priv->queueimg);

  self->ximagesink = gst_element_factory_make("gconfvideosink", "gconfvideosink");
  gst_bin_add(GST_BIN(self->pipeline), self->ximagesink);

  self->fakesink = gst_element_factory_make("fakesink", "fakesink");
  gst_bin_add(GST_BIN(self->pipeline), self->fakesink);

  /*
   * the pipeline looks like this:
   * gconfvideosrc -> ffmpegcsp
   *                    '-> videoscale
   *                         '-> ffmpegcsp -> effects -> ffmpegcsp 
   *    -------------------------------------------------------'
   *    '--> tee (filtered) -> queue-> ffmpegcsp -> gconfvideosink
   *          |
   *       queueimg -> fakesink -> pixbuf (gets picture from data)
   */


  gst_element_link(priv->source, priv->ffmpeg1);

  gst_element_link(priv->ffmpeg1, priv->effect);
  gst_element_link(priv->effect, priv->ffmpeg2);

  priv->filter = gst_caps_new_simple("video/x-raw-rgb",
      "bpp", G_TYPE_INT, 24,
      "depth", G_TYPE_INT, 24, NULL);

  link_ok = gst_element_link_filtered(priv->ffmpeg2, priv->tee, priv->filter);
  if (!link_ok) {
    g_warning("Failed to link elements!");
  }

  gst_element_link(priv->tee, priv->queuevid);
  gst_element_link(priv->queuevid, priv->ffmpeg3);

  gst_element_link(priv->ffmpeg3, self->ximagesink);

  // setting back the format to get nice pictures
  priv->filter = gst_caps_new_simple("video/x-raw-rgb", NULL);
  link_ok = gst_element_link_filtered(priv->tee, priv->queueimg, priv->filter);
  if (!link_ok) {
    g_warning("Failed to link elements!");
  }
  //gst_element_link(priv->tee, priv->queueimg);

  gst_element_link(priv->queueimg, self->fakesink);
  g_object_set(G_OBJECT(self->fakesink), "signal-handoffs", TRUE, NULL);

  g_signal_connect(G_OBJECT(self->fakesink), "handoff", 
                   G_CALLBACK(cb_have_data), self);

}
static gboolean
cb_have_data(GstElement *element, GstBuffer *buffer, GstPad *pad, gpointer self)
{
  PipelinePrivate *priv = PIPELINE_GET_PRIVATE(self);

  unsigned char *data_photo = (unsigned char *)GST_BUFFER_DATA(buffer);
  if (priv->picture_requested) {
    GstCaps *caps;
    const GstStructure *structure;

    int width, height;

    caps = gst_buffer_get_caps(buffer);
    structure = gst_caps_get_structure(caps, 0);
    gst_structure_get_int(structure, "width", &width);
    gst_structure_get_int(structure, "height", &height);

    create_photo(data_photo, width, height);
    priv->picture_requested = FALSE;
  }
  return TRUE;
}

void
create_photo(unsigned char *data, int width, int height)
{
  int i;
  gchar *filename = NULL;
  GnomeVFSURI *uri;
  GdkPixbuf *pixbuf;

  i = 1;
  filename = cheese_fileutil_get_photo_filename(i);

  uri = gnome_vfs_uri_new(filename);

  while (gnome_vfs_uri_exists(uri)) {
    i++;
    filename = cheese_fileutil_get_photo_filename(i);
    g_free(uri);
    uri = gnome_vfs_uri_new(filename);
  }
  g_free(uri);

  pixbuf = gdk_pixbuf_new_from_data (data, GDK_COLORSPACE_RGB, FALSE, 8, 
                                     width, height, width * 3, NULL, NULL);

  gdk_pixbuf_save(pixbuf, filename, "jpeg", NULL, NULL);
  g_object_unref(G_OBJECT(pixbuf));

  g_print("Photo saved: %s (%dx%d)\n", filename, width, height);
}

/*
 * shamelessly Stolen from gnome-media:
 * pipeline-tests.c
 * Copyright (C) 2002 Jan Schmidt
 * Copyright (C) 2005 Tim-Philipp Müller <tim centricular net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

static gboolean
build_test_pipeline(const gchar *pipeline_desc, GError **p_err, Pipeline *self) {
  PipelinePrivate *priv = PIPELINE_GET_PRIVATE(self);
  gboolean return_val = FALSE;

  g_assert (p_err != NULL);

  if (pipeline_desc) {
    priv->gst_test_pipeline = gst_parse_launch(pipeline_desc, p_err);

    if (*p_err == NULL && priv->gst_test_pipeline != NULL) {
      return_val = TRUE;
    }
  }

  return return_val;
}

static gboolean
test_pipeline(const gchar *pipeline_desc, Pipeline *self) {
  PipelinePrivate *priv = PIPELINE_GET_PRIVATE(self);
  GstStateChangeReturn ret;
  GstMessage *msg;
  GError *err = NULL;
  GstBus *bus;


  /* Build the pipeline */
  if (!build_test_pipeline (pipeline_desc, &err, self)) {
    /* Show the error pipeline */
    //pipeline_error_dlg(pipeline_desc, (err) ? err->message : NULL, self);
    pipeline_error_print(pipeline_desc, (err) ? err->message : NULL, self);
    if (err)
      g_error_free(err);
    return FALSE;
  }

  /* Start the pipeline and wait for max. 3 seconds for it to start up */
  gst_element_set_state(priv->gst_test_pipeline, GST_STATE_PLAYING);
  ret = gst_element_get_state(priv->gst_test_pipeline, NULL, NULL, 3 * GST_SECOND);

  /* Check if any error messages were posted on the bus */
  bus = gst_element_get_bus(priv->gst_test_pipeline);
  msg = gst_bus_poll(bus, GST_MESSAGE_ERROR, 0);
  gst_object_unref(bus);

  if (priv->gst_test_pipeline) {
    gst_element_set_state (priv->gst_test_pipeline, GST_STATE_NULL);
    gst_object_unref (priv->gst_test_pipeline);
    priv->gst_test_pipeline = NULL;
  }

  if (msg != NULL) {
    gchar *dbg = NULL;

    gst_message_parse_error(msg, &err, &dbg);
    gst_message_unref(msg);

    g_message("Error running pipeline '%s': %s [%s]", pipeline_desc,
        (err) ? err->message : "(null error)",
        (dbg) ? dbg : "no additional debugging details");
    //pipeline_error_dlg(pipeline_desc, err->message, self);
    pipeline_error_print(pipeline_desc, err->message, self);
    g_error_free (err);
    g_free (dbg);
    return FALSE;
  } else if (ret != GST_STATE_CHANGE_SUCCESS) {
    //pipeline_error_dlg(pipeline_desc, NULL, self);
    pipeline_error_print(pipeline_desc, NULL, self);
    return FALSE;
  } else {
    //works
    return TRUE;
  }
  return FALSE;
}

/*
static void
pipeline_error_dlg(const gchar *pipeline_desc, const gchar *error_message, Pipeline *self) {
  gchar *errstr;

  if (error_message) {
    errstr = g_strdup_printf("[%s]: %s", pipeline_desc, error_message);
  } else {
    errstr = g_strdup_printf(("Failed to construct test pipeline for '%s'"),
        pipeline_desc);
  }

  GtkWidget *dialog;

  dialog = gtk_message_dialog_new(GTK_WINDOW(cheese_window.window),
      GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
      "SORRY, but Cheese has failed to set up you webcam with %s:\n\n%s",
      g_strsplit(pipeline_desc, " !", 0)[0], errstr);

  gtk_dialog_run(GTK_DIALOG (dialog));
  gtk_widget_destroy(dialog);

  g_free(errstr);
}
*/

static void
pipeline_error_print(const gchar *pipeline_desc, const gchar *error_message, Pipeline *self) {
  gchar *errstr;

  if (error_message) {
    errstr = g_strdup_printf("[%s]: %s", pipeline_desc, error_message);
  } else {
    errstr = g_strdup_printf(("Failed to construct test pipeline for '%s'"),
        pipeline_desc);
  }

  g_message("test pipeline for %s failed:\n%s",
      g_strsplit(pipeline_desc, " !", 0)[0], errstr);

  g_free(errstr);
}
