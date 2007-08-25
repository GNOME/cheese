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

#include <glib.h>
#include <glib/gi18n.h>
#include <gst/interfaces/xoverlay.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs.h>

#include "cheese.h"
#include "cheese-effects-widget.h"
#include "cheese-fileutil.h"
#include "cheese-pipeline-photo.h"
#include "cheese-window.h"

G_DEFINE_TYPE (PipelinePhoto, cheese_pipeline_photo, G_TYPE_OBJECT)

static GObjectClass *parent_class = NULL;

typedef struct _PipelinePhotoPrivate PipelinePhotoPrivate;

struct _PipelinePhotoPrivate
{
  int picture_requested;
  int timeout;
  gboolean countdown;
  gboolean countdown_is_active;

  GstElement *source;
  GstElement *ffmpeg1, *ffmpeg2, *ffmpeg3, *ffmpeg4;
  GstElement *tee;
  GstElement *queuevid, *queueimg;
  GstElement *caps;
  GstElement *effect;
  GstElement *textoverlay;
  GstCaps *filter;
};

#define PIPELINE_PHOTO_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), PIPELINE_PHOTO_TYPE, PipelinePhotoPrivate))

// private methods
static gboolean cheese_pipeline_photo_have_data_cb (GstElement *,
                                                    GstBuffer *,
                                                    GstPad *,
                                                    gpointer);
static gboolean cheese_pipeline_photo_lens_open (PipelinePhoto *);
static void cheese_pipeline_photo_create_photo (unsigned char *, int, int);
static gboolean cheese_pipeline_set_textoverlay (gpointer);

void
cheese_pipeline_photo_finalize (GObject *object)
{
  PipelinePhotoPrivate *priv = PIPELINE_PHOTO_GET_PRIVATE (object);
  gst_caps_unref (priv->filter);

  (*parent_class->finalize) (object);
  return;
}

PipelinePhoto *
cheese_pipeline_photo_new (void)
{
  PipelinePhoto *self = g_object_new (PIPELINE_PHOTO_TYPE, NULL);

  return self;
}

void
cheese_pipeline_photo_class_init (PipelinePhotoClass *klass)
{
  GObjectClass *object_class;

  parent_class = g_type_class_peek_parent (klass);
  object_class = (GObjectClass *) klass;

  object_class->finalize = cheese_pipeline_photo_finalize;
  g_type_class_add_private (klass, sizeof (PipelinePhotoPrivate));

  G_OBJECT_CLASS (klass)->finalize =
    (GObjectFinalizeFunc) cheese_pipeline_photo_finalize;
}

void
cheese_pipeline_photo_init (PipelinePhoto *self)
{
  cheese_pipeline_photo_set_countdown (TRUE, self);
}

void
cheese_pipeline_photo_set_play (PipelinePhoto *self)
{
  gst_element_set_state (self->pipeline, GST_STATE_PLAYING);
  return;
}

void
cheese_pipeline_photo_set_stop (PipelinePhoto *self)
{
  gst_element_set_state (self->pipeline, GST_STATE_NULL);
  return;
}

void
cheese_pipeline_photo_button_clicked (GtkWidget *widget, gpointer self)
{
  PipelinePhotoPrivate *priv = PIPELINE_PHOTO_GET_PRIVATE (self);
  if (priv->countdown) {
    // "3"
    g_timeout_add (0, (GSourceFunc) cheese_pipeline_set_textoverlay, self);
    // "2"
    g_timeout_add (1000, (GSourceFunc) cheese_pipeline_set_textoverlay, self);
    // "1"
    g_timeout_add (2000, (GSourceFunc) cheese_pipeline_set_textoverlay, self);
    // "Cheese!"
    g_timeout_add (3000, (GSourceFunc) cheese_pipeline_set_textoverlay, self);
    g_timeout_add (3000, (GSourceFunc) cheese_pipeline_photo_lens_open, self);
    // ""
    g_timeout_add (4000, (GSourceFunc) cheese_pipeline_set_textoverlay, self);
  } else {
    cheese_pipeline_photo_lens_open (self);
  }
  return;
}

static gboolean
cheese_pipeline_photo_lens_open (PipelinePhoto *self)
{
  PipelinePhotoPrivate *priv = PIPELINE_PHOTO_GET_PRIVATE (self);
  priv->picture_requested = TRUE;
  if (priv->countdown)
    return FALSE;
  else
    return TRUE;
}

GstElement *
cheese_pipeline_photo_get_ximagesink (PipelinePhoto *self)
{
  return self->ximagesink;
}

GstElement *
cheese_pipeline_photo_get_fakesink (PipelinePhoto *self)
{
  return self->fakesink;
}

GstElement *
cheese_pipeline_photo_get_pipeline (PipelinePhoto *self)
{
  return self->pipeline;
}

void
cheese_pipeline_photo_set_countdown (gboolean state, PipelinePhoto *self) {

  PipelinePhotoPrivate *priv = PIPELINE_PHOTO_GET_PRIVATE (self);
  priv->countdown = state;
}

gboolean
cheese_pipeline_photo_countdown_is_active (PipelinePhoto *self) {

  PipelinePhotoPrivate *priv = PIPELINE_PHOTO_GET_PRIVATE (self);
  return priv->countdown_is_active;
}

void
cheese_pipeline_photo_change_effect (gchar *effect, gpointer self)
{
  PipelinePhotoPrivate *priv = PIPELINE_PHOTO_GET_PRIVATE (self);

  if (effect != NULL)
  {
    cheese_pipeline_photo_set_stop (PIPELINE_PHOTO (self));

    gst_element_unlink (priv->ffmpeg1, priv->effect);
    gst_element_unlink (priv->effect, priv->ffmpeg2);
    gst_bin_remove (GST_BIN (PIPELINE_PHOTO (self)->pipeline), priv->effect);

    g_print ("changing to effect: %s\n", effect);
    priv->effect = gst_parse_bin_from_description (effect, TRUE, NULL);
    gst_bin_add (GST_BIN (PIPELINE_PHOTO (self)->pipeline), priv->effect);

    gst_element_link (priv->ffmpeg1, priv->effect);
    gst_element_link (priv->effect, priv->ffmpeg2);

    cheese_pipeline_photo_set_play (self);
  }
}

/* 
  this is an easter egg for raphael ;)

              .-"-.                ,      ,_
            .'=^=^='.              )\      \".
           /=^=^=^=^=\             / )      ) \
          :^= HAPPY =^;           / /       / /
          |^ EASTER! ^|           | |      / / 
          :^=^=^=^=^=^:           \ |     / /
           \=^=^=^=^=/             \|-""-//`
            `.=^=^=.'              / _  _ \
              `~~~`              _| / \/ \ |_
                                / `-\0||0/-' \
                               =\.: ,_()_  :./=
                                 `-._\II/_.-'
                                     `""`      
  it was nice to work with you the whole summer!
*/

void
cheese_pipeline_photo_create (gchar *source_pipeline, PipelinePhoto *self)
{

  PipelinePhotoPrivate *priv = PIPELINE_PHOTO_GET_PRIVATE (self);
  gboolean link_ok;

  self->pipeline = gst_pipeline_new ("pipeline");
  priv->source = gst_parse_bin_from_description (source_pipeline, TRUE, NULL);
  gst_bin_add (GST_BIN (self->pipeline), priv->source);

  priv->ffmpeg1 = gst_element_factory_make ("ffmpegcolorspace", "ffmpegcolorspace");
  gst_bin_add (GST_BIN (self->pipeline), priv->ffmpeg1);

  priv->effect = gst_element_factory_make ("identity", "effect");
  gst_bin_add (GST_BIN (self->pipeline), priv->effect);

  priv->ffmpeg2 = gst_element_factory_make ("ffmpegcolorspace", "ffmpegcolorspace2");
  gst_bin_add (GST_BIN (self->pipeline), priv->ffmpeg2);

  priv->ffmpeg3 = gst_element_factory_make ("ffmpegcolorspace", "ffmpegcolorspace3");
  gst_bin_add (GST_BIN (self->pipeline), priv->ffmpeg3);

  priv->ffmpeg4 = gst_element_factory_make ("ffmpegcolorspace", "ffmpegcolorspace4");
  gst_bin_add (GST_BIN (self->pipeline), priv->ffmpeg4);

  priv->tee = gst_element_factory_make ("tee", "tee");
  gst_bin_add (GST_BIN (self->pipeline), priv->tee);

  priv->queuevid = gst_element_factory_make ("queue", "queuevid");
  gst_bin_add (GST_BIN (self->pipeline), priv->queuevid);

  priv->queueimg = gst_element_factory_make ("queue", "queueimg");
  gst_bin_add (GST_BIN (self->pipeline), priv->queueimg);

  priv->textoverlay = gst_element_factory_make ("textoverlay", "textoverlay");
  gst_bin_add (GST_BIN (self->pipeline), priv->textoverlay);
  g_object_set (priv->textoverlay, "font-desc", "mono 60", NULL);
  priv->timeout = 3;

  self->ximagesink = gst_element_factory_make ("gconfvideosink", "gconfvideosink");
  gst_bin_add (GST_BIN (self->pipeline), self->ximagesink);

  self->fakesink = gst_element_factory_make ("fakesink", "fakesink");
  gst_bin_add (GST_BIN (self->pipeline), self->fakesink);

  /*
   * the pipeline looks like this:
   * v4l(2)src ->
   *      '-> ffmpegcsp -> effects -> ffmpegcsp 
   *    ------------------------------------'
   *    '--> tee (filtered) -> queue-> ffmpegcsp -> gconfvideosink
   *          |
   *       queueimg -> fakesink -> pixbuf (gets picture from data)
   */


  gst_element_link_many (priv->source,
                         priv->ffmpeg1,
                         priv->effect,
                         priv->ffmpeg2,
                         priv->tee, NULL);

  // here we split: first path
  gst_element_link_many (priv->tee,
                         priv->queuevid,
                         priv->ffmpeg3,
                         priv->textoverlay,
                         priv->ffmpeg4,
                         self->ximagesink, NULL);

  // second path
  gst_element_link (priv->tee, priv->queueimg);

  priv->filter = gst_caps_new_simple ("video/x-raw-rgb",
      "bpp", G_TYPE_INT, 24,
      "depth", G_TYPE_INT, 24,
      "red_mask",   G_TYPE_INT, 0xff0000, // enforce rgb
      "green_mask", G_TYPE_INT, 0x00ff00,
      "blue_mask",  G_TYPE_INT, 0x0000ff, NULL);
  link_ok = gst_element_link_filtered (priv->queueimg, self->fakesink, priv->filter);
  if (!link_ok)
  {
    g_warning ("Failed to link elements!");
  }

  g_object_set (G_OBJECT (self->fakesink), "signal-handoffs", TRUE, NULL);

  g_signal_connect (G_OBJECT (self->fakesink), "handoff",
      G_CALLBACK (cheese_pipeline_photo_have_data_cb), self);

}

static gboolean
cheese_pipeline_photo_have_data_cb (GstElement *element, GstBuffer *buffer,
                                    GstPad *pad, gpointer self)
{
  PipelinePhotoPrivate *priv = PIPELINE_PHOTO_GET_PRIVATE (self);

  unsigned char *data_photo = (unsigned char *) GST_BUFFER_DATA (buffer);
  if (priv->picture_requested)
  {
    GstCaps *caps;
    const GstStructure *structure;

    int width, height;

    caps = gst_buffer_get_caps (buffer);
    structure = gst_caps_get_structure (caps, 0);
    gst_structure_get_int (structure, "width", &width);
    gst_structure_get_int (structure, "height", &height);

    cheese_pipeline_photo_create_photo (data_photo, width, height);
    priv->picture_requested = FALSE;
  }
  return TRUE;
}

void
cheese_pipeline_photo_create_photo (unsigned char *data, int width, int height)
{
  int i;
  gchar *filename = NULL;
  GnomeVFSURI *uri;
  GdkPixbuf *pixbuf;

  i = 1;
  filename = cheese_fileutil_get_photo_filename (i);

  uri = gnome_vfs_uri_new (filename);

  while (gnome_vfs_uri_exists (uri))
  {
    i++;
    filename = cheese_fileutil_get_photo_filename (i);
    g_free (uri);
    uri = gnome_vfs_uri_new (filename);
  }
  g_free (uri);

  pixbuf = gdk_pixbuf_new_from_data(data,
      GDK_COLORSPACE_RGB, // RGB-colorspace
      FALSE,              // No alpha-channel
      8,                  // Bits per RGB-component
      width, height,      // Dimensions
      3 * width,          // Number of bytes between lines (ie stride)
      NULL, NULL);

  gdk_pixbuf_save (pixbuf, filename, "jpeg", NULL, NULL);
  g_object_unref (G_OBJECT (pixbuf));

  g_print ("Photo saved: %s (%dx%d)\n", filename, width, height);
}

static gboolean
cheese_pipeline_set_textoverlay (gpointer self) {
  PipelinePhotoPrivate *priv = PIPELINE_PHOTO_GET_PRIVATE (self);

  priv->countdown_is_active = TRUE;
  if (priv->timeout <= 0) {
    g_object_set (priv->textoverlay, "text", _("Cheese!"), NULL);
    priv->timeout = 4;
  } else if (priv->timeout == 4) {
    g_object_set (priv->textoverlay, "text", "", NULL);
    priv->timeout = 3;
    priv->countdown_is_active = FALSE;
  } else {
    g_object_set (priv->textoverlay, "text", g_strdup_printf("%d", priv->timeout), NULL);
    priv->timeout--;
  }
  return FALSE;
}
