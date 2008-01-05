/* GStreamer
 * Copyright (C) 2007 Mathias Hasselmann <mathias.hasselmann@gmx.de>
 *
 * gst-play.h: A simple (audio) file player
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

#include "gst-player.h"
#include <gst/gst.h>

enum {
  PROPID_URI = 1
};

enum {
  SIGNAL_DONE,
  LAST_SIGNAL
};

struct _GstPlayerPrivate {
  GstElement *pipeline;
  GstElement *playbin;
};

static guint signals[LAST_SIGNAL];

G_DEFINE_TYPE (GstPlayer, gst_player, G_TYPE_OBJECT)

static gboolean
gst_player_bus_message (GstBus     *bus G_GNUC_UNUSED,
                        GstMessage *message,
                        gpointer    data)
{
  GstPlayer *self = GST_PLAYER (data);
  GError *error = NULL;

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_EOS:
      g_signal_emit (self, signals[SIGNAL_DONE], 0, NULL);

      gst_player_stop (self);
      g_object_unref (self);

      break;

    case GST_MESSAGE_ERROR: 
      gst_message_parse_error (message, &error, NULL);
      g_signal_emit (self, signals[SIGNAL_DONE], 0, error);
      g_clear_error (&error);

      gst_player_stop (self);
      g_object_unref (self);

      break;

    default:
      break;
  }
  return TRUE;
}

static void
gst_player_init (GstPlayer *self)
{
  GstBus *bus;

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, GST_TYPE_PLAYER, GstPlayerPrivate);
  self->priv->pipeline = gst_pipeline_new (NULL);

  self->priv->playbin = gst_element_factory_make ("playbin", NULL);
  g_return_if_fail (NULL != self->priv->playbin);

  gst_bin_add (GST_BIN (self->priv->pipeline), self->priv->playbin);

  bus = gst_pipeline_get_bus (GST_PIPELINE (self->priv->pipeline));
  gst_bus_add_watch (bus, gst_player_bus_message, self);
  g_object_unref (bus);

  g_object_ref (self);
}

static void
gst_player_set_property (GObject      *object,
                         guint         propid,
                         const GValue *value,
                         GParamSpec   *spec)
{
  GstPlayer *self = GST_PLAYER (object);
  g_return_if_fail (self && self->priv);

  switch (propid) {
    case PROPID_URI:
      g_object_set_property (G_OBJECT (self->priv->playbin), 
                             "uri", value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, propid, spec);
      break;
  }
}

static void
gst_player_dispose (GObject *object)
{
  GstPlayer *self = GST_PLAYER (object);

  g_message ("disposing %s", G_OBJECT_TYPE_NAME (object));

  if (self && self->priv) {
    if (self->priv->pipeline) {
      g_object_unref (self->priv->pipeline);
      self->priv->pipeline = NULL;
    }
  }

  if (G_OBJECT_CLASS (gst_player_parent_class)->dispose)
    G_OBJECT_CLASS (gst_player_parent_class)->dispose (object);
}

static void
gst_player_class_init (GstPlayerClass *player_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (player_class);

  object_class->dispose = gst_player_dispose;
  object_class->set_property = gst_player_set_property;

  g_object_class_install_property (object_class, PROPID_URI, g_param_spec_string (
                                   "uri", NULL, NULL, NULL, G_PARAM_STATIC_NAME |
                                   G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  signals[SIGNAL_DONE] = g_signal_new ("done", GST_TYPE_PLAYER, G_SIGNAL_RUN_FIRST, 
                                       G_STRUCT_OFFSET (GstPlayerClass, done), NULL, NULL,
                                       g_cclosure_marshal_VOID__POINTER, 
                                       G_TYPE_NONE, 1, G_TYPE_POINTER);

  g_type_class_add_private (player_class, sizeof (GstPlayerPrivate));
}

GstPlayer* 
gst_player_new (const gchar *uri)
{
  return g_object_new (GST_TYPE_PLAYER, "uri", uri, NULL);
}

void
gst_player_start (GstPlayer *player)
{
  g_return_if_fail (GST_IS_PLAYER (player));
  gst_element_set_state (player->priv->pipeline, GST_STATE_PLAYING);
}

void
gst_player_stop (GstPlayer *player)
{
  g_return_if_fail (GST_IS_PLAYER (player));
  gst_element_set_state (player->priv->pipeline, GST_STATE_NULL);
}

GstPlayer*
gst_play_file (const gchar *filename,
               GError     **error)
{
  GstPlayer *player;
  gchar *uri;

  g_return_val_if_fail (NULL != filename, NULL);
  uri = g_filename_to_uri (filename, NULL, error);

  if (NULL == uri)
    return NULL;

  player = gst_player_new (uri);
  gst_player_start (player);
  g_free (uri);

  return player;
}
