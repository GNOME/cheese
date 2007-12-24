/*
 * Copyright (C) 2007 daniel g. siegel <dgsiegel@gmail.com>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <stdlib.h>
#include <gst/gst.h>

#include "cheese-audio-play.h"

#define SHUTTER_SOUNDS 4

G_DEFINE_TYPE (CheeseAudioPlay, cheese_audio_play, G_TYPE_OBJECT)

#define CHEESE_AUDIO_PLAY_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHEESE_TYPE_AUDIO_PLAY, CheeseAudioPlayPrivate))

typedef struct
{
  GRand *rand;
  int counter;

  GstElement *pipeline;
  GstElement *source;
  GstElement *parser;
  GstElement *decoder;
  GstElement *conv;
  GstElement *sink;
} CheeseAudioPlayPrivate;

static char *
cheese_audio_get_filename (CheeseAudioPlay *audio_play) 
{
  CheeseAudioPlayPrivate *priv = CHEESE_AUDIO_PLAY_GET_PRIVATE (audio_play);  
  char *filename;
  if (priv->counter > 7)
   filename = g_strdup_printf ("%ssounds/shutter%i.ogg", PACKAGE_DATADIR, g_rand_int_range (priv->rand, 1, SHUTTER_SOUNDS));
  else
   filename = g_strdup_printf ("%ssounds/shutter0.ogg", PACKAGE_DATADIR);

  return filename;
}

void
cheese_audio_play_file (CheeseAudioPlay *audio_play) {

  CheeseAudioPlayPrivate *priv = CHEESE_AUDIO_PLAY_GET_PRIVATE (audio_play);  
  priv->counter++;
  gst_element_set_state (priv->pipeline, GST_STATE_NULL);

  char *file = cheese_audio_get_filename (audio_play);
  g_object_set (G_OBJECT (priv->source), "location", file, NULL);
  g_free (file);

  gst_element_set_state (priv->pipeline, GST_STATE_PLAYING);

}

static void
cheese_audio_play_new_pad (GstElement *element,
	 GstPad     *pad,
	 gpointer    data)
{
  CheeseAudioPlayPrivate *priv = CHEESE_AUDIO_PLAY_GET_PRIVATE (data);  
  GstPad *sinkpad;

  sinkpad = gst_element_get_pad (priv->decoder, "sink");

  gst_pad_link (pad, sinkpad);
  gst_object_unref (sinkpad);
}

static void
cheese_audio_play_finalize (GObject *object)
{
  CheeseAudioPlay *audio_play;

  audio_play = CHEESE_AUDIO_PLAY (object);
  CheeseAudioPlayPrivate *priv = CHEESE_AUDIO_PLAY_GET_PRIVATE (audio_play);  
  g_rand_free (priv->rand);
  g_free (priv->pipeline);
  g_free (priv->source);
  g_free (priv->parser);
  g_free (priv->decoder);
  g_free (priv->conv);
  g_free (priv->sink);

  G_OBJECT_CLASS (cheese_audio_play_parent_class)->finalize (object);
}

static void
cheese_audio_play_class_init (CheeseAudioPlayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = cheese_audio_play_finalize;

  g_type_class_add_private (klass, sizeof (CheeseAudioPlayPrivate));
}

static void
cheese_audio_play_init (CheeseAudioPlay *audio_play)
{
  CheeseAudioPlayPrivate* priv = CHEESE_AUDIO_PLAY_GET_PRIVATE (audio_play);
  priv->rand = g_rand_new ();

  priv->pipeline = gst_pipeline_new ("audio-player");
  priv->source = gst_element_factory_make ("filesrc", "file-source");
  priv->parser = gst_element_factory_make ("oggdemux", "ogg-parser");
  priv->decoder = gst_element_factory_make ("vorbisdec", "vorbis-decoder");
  priv->conv = gst_element_factory_make ("audioconvert", "converter");
  priv->sink = gst_element_factory_make ("gconfaudiosink", "output");

  char *file = cheese_audio_get_filename (audio_play);
  g_object_set (G_OBJECT (priv->source), "location", file, NULL);
  g_free (file);

  gst_bin_add_many (GST_BIN (priv->pipeline),
		    priv->source, priv->parser, priv->decoder, priv->conv, priv->sink, NULL);

  /* link together - note that we cannot link the parser and
   * decoder yet, becuse the parser uses dynamic pads. For that,
   * we set a pad-added signal handler. */
  gst_element_link (priv->source, priv->parser);
  gst_element_link_many (priv->decoder, priv->conv, priv->sink, NULL);
  g_signal_connect (priv->parser, "pad-added", G_CALLBACK (cheese_audio_play_new_pad), audio_play);
}

CheeseAudioPlay * 
cheese_audio_play_new ()
{
  CheeseAudioPlay *audio_play;

  audio_play = g_object_new (CHEESE_TYPE_AUDIO_PLAY, NULL);  
  return audio_play;
}
