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

G_DEFINE_TYPE(PipelinePhoto, cheese_pipeline_photo, G_TYPE_OBJECT)

static GObjectClass *parent_class = NULL;

typedef struct _PipelinePhotoPrivate PipelinePhotoPrivate;

struct _PipelinePhotoPrivate
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

#define PIPELINE_PHOTO_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), PIPELINE_PHOTO_TYPE, PipelinePhotoPrivate))

// private methods
static gboolean cheese_pipeline_photo_have_data_cb(GstElement *element, GstBuffer *buffer, GstPad *pad, gpointer user_data);
static void cheese_pipeline_photo_lens_open(PipelinePhoto *self);
static void cheese_pipeline_photo_create_photo(unsigned char *data, int width, int height);

void
cheese_pipeline_photo_finalize(GObject *object)
{
  PipelinePhotoPrivate *priv = PIPELINE_PHOTO_GET_PRIVATE(object);
  gst_caps_unref(priv->filter);

  (*parent_class->finalize) (object);
  return;
}

PipelinePhoto *
cheese_pipeline_photo_new(void)
{
  PipelinePhoto *self = g_object_new(PIPELINE_PHOTO_TYPE, NULL);

  return self;
}

void
cheese_pipeline_photo_class_init(PipelinePhotoClass *klass)
{
  GObjectClass *object_class;

  parent_class = g_type_class_peek_parent(klass);
  object_class = (GObjectClass*) klass;

  object_class->finalize = cheese_pipeline_photo_finalize;
  g_type_class_add_private(klass, sizeof(PipelinePhotoPrivate));

  G_OBJECT_CLASS(klass)->finalize = (GObjectFinalizeFunc) cheese_pipeline_photo_finalize;
}

void
cheese_pipeline_photo_init(PipelinePhoto *self)
{
}

void
cheese_pipeline_photo_set_play(PipelinePhoto *self)
{
  gst_element_set_state(self->pipeline, GST_STATE_PLAYING);
  return;
}

void
cheese_pipeline_photo_set_stop(PipelinePhoto *self)
{
  gst_element_set_state(self->pipeline, GST_STATE_NULL);
  return;
}

void
cheese_pipeline_photo_button_clicked(GtkWidget *widget, gpointer self)
{
  cheese_pipeline_photo_lens_open(self);
  return;
}

static void
cheese_pipeline_photo_lens_open(PipelinePhoto *self)
{
  PipelinePhotoPrivate *priv = PIPELINE_PHOTO_GET_PRIVATE(self);
  priv->picture_requested = TRUE;
  return;
}

GstElement *
cheese_pipeline_photo_get_ximagesink(PipelinePhoto *self)
{
  return self->ximagesink;
}

GstElement *
cheese_pipeline_photo_get_fakesink(PipelinePhoto *self)
{
  return self->fakesink;
}

GstElement *
cheese_pipeline_photo_get_pipeline(PipelinePhoto *self)
{
  return self->pipeline;
}

void
cheese_pipeline_photo_change_effect(gchar *effect, gpointer self)
{
  PipelinePhotoPrivate *priv = PIPELINE_PHOTO_GET_PRIVATE(self);

  if (effect != NULL) {
    cheese_pipeline_photo_set_stop(PIPELINE_PHOTO(self));

    gst_element_unlink(priv->ffmpeg1, priv->effect);
    gst_element_unlink(priv->effect, priv->ffmpeg2);
    gst_bin_remove(GST_BIN(PIPELINE_PHOTO(self)->pipeline), priv->effect);

    g_print("changing to effect: %s\n", effect);
    priv->effect = gst_parse_bin_from_description(effect, TRUE, NULL);
    gst_bin_add(GST_BIN(PIPELINE_PHOTO(self)->pipeline), priv->effect);

    gst_element_link(priv->ffmpeg1, priv->effect);
    gst_element_link(priv->effect, priv->ffmpeg2);

    cheese_pipeline_photo_set_play(self);
  }
}

void
cheese_pipeline_photo_create(gchar *source_pipeline, PipelinePhoto *self) {

  PipelinePhotoPrivate *priv = PIPELINE_PHOTO_GET_PRIVATE(self);
  gboolean link_ok; 

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
                   G_CALLBACK(cheese_pipeline_photo_have_data_cb), self);

}
static gboolean
cheese_pipeline_photo_have_data_cb(GstElement *element, GstBuffer *buffer, GstPad *pad, gpointer self)
{
  PipelinePhotoPrivate *priv = PIPELINE_PHOTO_GET_PRIVATE(self);

  unsigned char *data_photo = (unsigned char *)GST_BUFFER_DATA(buffer);
  if (priv->picture_requested) {
    GstCaps *caps;
    const GstStructure *structure;

    int width, height;

    caps = gst_buffer_get_caps(buffer);
    structure = gst_caps_get_structure(caps, 0);
    gst_structure_get_int(structure, "width", &width);
    gst_structure_get_int(structure, "height", &height);

    cheese_pipeline_photo_create_photo(data_photo, width, height);
    priv->picture_requested = FALSE;
  }
  return TRUE;
}

void
cheese_pipeline_photo_create_photo(unsigned char *data, int width, int height)
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
