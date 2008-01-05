/* GStreamer
 * Copyright (C) 2007 Mathias Hasselmann <mathias.hasselmann@gmx.de>
 *
 * gst-audio-play.h: A simple audio file player
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "gst-audio-play.h"
#include <gst/gst.h>

#define GST_AUDIO_PLAY_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GST_TYPE_AUDIO_PLAY, GstAudioPlayPrivate))

enum {
  PROPID_URI = 1
};

enum {
  SIGNAL_DONE,
  LAST_SIGNAL
};

typedef struct {
  GstElement *pipeline;
  GstElement *playbin;
} GstAudioPlayPrivate;

static guint signals[LAST_SIGNAL];

G_DEFINE_TYPE (GstAudioPlay, gst_audio_play, G_TYPE_OBJECT)

static gboolean
gst_audio_play_bus_message (GstBus     *bus G_GNUC_UNUSED,
                        GstMessage *message,
                        gpointer    data)
{
  GstAudioPlay *self = GST_AUDIO_PLAY (data);
  GError *error = NULL;

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_EOS:
      g_signal_emit (self, signals[SIGNAL_DONE], 0, NULL);

      gst_audio_play_stop (self);
      g_object_unref (self);

      break;

    case GST_MESSAGE_ERROR: 
      gst_message_parse_error (message, &error, NULL);
      g_signal_emit (self, signals[SIGNAL_DONE], 0, error);
      g_clear_error (&error);

      gst_audio_play_stop (self);
      g_object_unref (self);

      break;

    default:
      break;
  }
  return TRUE;
}

static void
gst_audio_play_init (GstAudioPlay *self)
{
  GstAudioPlayPrivate* priv = GST_AUDIO_PLAY_GET_PRIVATE (self);
  GstBus *bus;

  priv->pipeline = gst_pipeline_new (NULL);

  priv->playbin = gst_element_factory_make ("playbin", NULL);
  g_return_if_fail (NULL != priv->playbin);

  gst_bin_add (GST_BIN (priv->pipeline), priv->playbin);

  bus = gst_pipeline_get_bus (GST_PIPELINE (priv->pipeline));
  gst_bus_add_watch (bus, gst_audio_play_bus_message, self);
  g_object_unref (bus);

  g_object_ref (self);
}

static void
gst_audio_play_set_property (GObject      *object,
                             guint         propid,
                             const GValue *value,
                             GParamSpec   *spec)
{
  GstAudioPlay *self = GST_AUDIO_PLAY (object);
  GstAudioPlayPrivate* priv = GST_AUDIO_PLAY_GET_PRIVATE (self);

  g_return_if_fail (self && priv);

  switch (propid) {
    case PROPID_URI:
      g_object_set_property (G_OBJECT (priv->playbin), 
                             "uri", value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, propid, spec);
      break;
  }
}

static void
gst_audio_play_dispose (GObject *object)
{
  GstAudioPlay *self = GST_AUDIO_PLAY (object);
  GstAudioPlayPrivate* priv = GST_AUDIO_PLAY_GET_PRIVATE (self);

  if (self && priv) {
    if (priv->pipeline) {
      g_object_unref (priv->pipeline);
      priv->pipeline = NULL;
    }
  }

  if (G_OBJECT_CLASS (gst_audio_play_parent_class)->dispose)
    G_OBJECT_CLASS (gst_audio_play_parent_class)->dispose (object);
}

static void
gst_audio_play_class_init (GstAudioPlayClass *audio_play_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (audio_play_class);

  object_class->dispose = gst_audio_play_dispose;
  object_class->set_property = gst_audio_play_set_property;

  g_object_class_install_property (object_class, PROPID_URI, g_param_spec_string (
                                   "uri", NULL, NULL, NULL, G_PARAM_STATIC_NAME |
                                   G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  signals[SIGNAL_DONE] = g_signal_new ("done", GST_TYPE_AUDIO_PLAY, G_SIGNAL_RUN_FIRST, 
                                       G_STRUCT_OFFSET (GstAudioPlayClass, done), NULL, NULL,
                                       g_cclosure_marshal_VOID__POINTER, 
                                       G_TYPE_NONE, 1, G_TYPE_POINTER);

  g_type_class_add_private (audio_play_class, sizeof (GstAudioPlayPrivate));
}

GstAudioPlay* 
gst_audio_play_new (const gchar *uri)
{
  return g_object_new (GST_TYPE_AUDIO_PLAY, "uri", uri, NULL);
}

void
gst_audio_play_start (GstAudioPlay *audio_play)
{
  g_return_if_fail (GST_IS_AUDIO_PLAY (audio_play));
  GstAudioPlayPrivate* priv = GST_AUDIO_PLAY_GET_PRIVATE (audio_play);

  gst_element_set_state (priv->pipeline, GST_STATE_PLAYING);
}

void
gst_audio_play_stop (GstAudioPlay *audio_play)
{
  g_return_if_fail (GST_IS_AUDIO_PLAY (audio_play));
  GstAudioPlayPrivate* priv = GST_AUDIO_PLAY_GET_PRIVATE (audio_play);

  gst_element_set_state (priv->pipeline, GST_STATE_NULL);
}

GstAudioPlay*
gst_audio_play_file (const gchar *filename,
                     GError     **error)
{
  GstAudioPlay *audio_play;
  gchar *uri;

  g_return_val_if_fail (NULL != filename, NULL);
  uri = g_filename_to_uri (filename, NULL, error);

  if (NULL == uri)
    return NULL;

  audio_play = gst_audio_play_new (uri);
  gst_audio_play_start (audio_play);
  g_free (uri);

  return audio_play;
}
