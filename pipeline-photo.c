/*
 * Copyright (C) 2007 daniel g. siegel <dgsiegel@gmail.com>
 * All rights reserved. This software is released under the GPL2 licence.
 */

#include <gst/interfaces/xoverlay.h>

#include "cheese.h"
#include "pipeline-photo.h"
G_DEFINE_TYPE (Pipeline, pipeline, G_TYPE_OBJECT)


static GObjectClass *parent_class = NULL;

typedef struct _PipelinePrivate PipelinePrivate;

struct _PipelinePrivate {
  int picture_requested;
};

#define PIPELINE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), PIPELINE_TYPE, PipelinePrivate))

// private methods
static gboolean cb_have_data(GstElement *element, GstBuffer *buffer, GstPad *pad, gpointer user_data);
static void pipeline_lens_open(Pipeline *self);

void
pipeline_set_play(Pipeline *self)
{
  gst_element_set_state(PIPELINE(self)->pipeline ,GST_STATE_PLAYING);
  return;
}

void
pipeline_set_stop(Pipeline *self)
{
  gst_element_set_state(PIPELINE(self)->pipeline ,GST_STATE_NULL);
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
  return PIPELINE(self)->ximagesink;
}

GstElement *
pipeline_get_fakesink(Pipeline *self)
{
  return PIPELINE(self)->fakesink;
}

GstElement *
pipeline_get_pipeline(Pipeline *self)
{
  return PIPELINE(self)->pipeline;
}


void
pipeline_create(Pipeline *self) {

  GstElement *source;
  GstElement *ffmpeg1, *ffmpeg2, *ffmpeg3;
  GstElement *tee;
  GstElement *queuevid, *queueimg;
  GstElement *caps;
  GstCaps *filter;
  gboolean link_ok; 

  PIPELINE(self)->pipeline = gst_pipeline_new("pipeline");
  source = gst_element_factory_make("gconfvideosrc", "v4l2src");
  // if you want to test without having a webcam
  //source = gst_element_factory_make("videotestsrc", "v4l2src");
  gst_bin_add(GST_BIN(PIPELINE(self)->pipeline), source);
  ffmpeg1 = gst_element_factory_make("ffmpegcolorspace", "ffmpegcolorspace");
  gst_bin_add(GST_BIN(PIPELINE(self)->pipeline), ffmpeg1);
  ffmpeg2 = gst_element_factory_make("ffmpegcolorspace", "ffmpegcolorspace2");
  gst_bin_add(GST_BIN(PIPELINE(self)->pipeline), ffmpeg2);
  ffmpeg3 = gst_element_factory_make("ffmpegcolorspace", "ffmpegcolorspace3");
  gst_bin_add(GST_BIN(PIPELINE(self)->pipeline), ffmpeg3);
  queuevid = gst_element_factory_make("queue", "queuevid");
  gst_bin_add(GST_BIN(PIPELINE(self)->pipeline), queuevid);
  queueimg = gst_element_factory_make("queue", "queueimg");
  gst_bin_add(GST_BIN(PIPELINE(self)->pipeline), queueimg);
  caps = gst_element_factory_make("capsfilter", "capsfilter");
  gst_bin_add(GST_BIN(PIPELINE(self)->pipeline), caps);
  PIPELINE(self)->ximagesink = gst_element_factory_make("gconfvideosink", "gconfvideosink");
  gst_bin_add(GST_BIN(PIPELINE(self)->pipeline), PIPELINE(self)->ximagesink);
  tee = gst_element_factory_make("tee", "tee");
  gst_bin_add(GST_BIN(PIPELINE(self)->pipeline), tee);
  PIPELINE(self)->fakesink = gst_element_factory_make("fakesink", "fakesink");
  gst_bin_add(GST_BIN(PIPELINE(self)->pipeline), PIPELINE(self)->fakesink);

  /*
   * the pipeline looks like this:
   * gconfvideosrc -> ffmpegcsp -> ffmpegcsp -> tee (filtered) -> queue-> ffmpegcsp -> gconfvideosink
   *                                             |
   *                                         queueimg -> fakesink -> pixbuf (gets picture from data)
   */

  gst_element_link(source, ffmpeg1);
  //FIXME: provide effects here
  gst_element_link(ffmpeg1, ffmpeg2);

  filter = gst_caps_new_simple("video/x-raw-rgb",
      "width", G_TYPE_INT, PHOTO_WIDTH,
      "height", G_TYPE_INT, PHOTO_HEIGHT,
      "framerate", GST_TYPE_FRACTION, 30, 1,
      "bpp", G_TYPE_INT, 24,
      "depth", G_TYPE_INT, 24, NULL);

  link_ok = gst_element_link_filtered(ffmpeg2, tee, filter);
  if (!link_ok) {
    g_warning("Failed to link elements!");
  }
  //FIXME: do we need this?
  //filter = gst_caps_new_simple("video/x-raw-yuv", NULL);

  gst_element_link(tee, queuevid);
  gst_element_link(queuevid, ffmpeg3);

  gst_element_link(ffmpeg3, PIPELINE(self)->ximagesink);

  gst_element_link(tee, queueimg);
  gst_element_link(queueimg, PIPELINE(self)->fakesink);
  g_object_set(G_OBJECT(PIPELINE(self)->fakesink), "signal-handoffs", TRUE, NULL);

  g_signal_connect(G_OBJECT(PIPELINE(self)->fakesink), "handoff", 
      G_CALLBACK(cb_have_data), self);

}

static gboolean
cb_have_data(GstElement *element, GstBuffer *buffer, GstPad *pad, gpointer self)
{
  PipelinePrivate *priv = PIPELINE_GET_PRIVATE(self);

  unsigned char *data_photo = (unsigned char *)GST_BUFFER_DATA(buffer);
  if (priv->picture_requested) {
    priv->picture_requested = FALSE;
    create_photo(data_photo);
  }
  return TRUE;
}

void
pipeline_finalize(GObject *object)
{
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

