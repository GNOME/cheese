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
#include <cheese-config.h>
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

  GstElement *play;
  GstBus *bus;
} CheeseAudioPlayPrivate;

static char *
cheese_audio_get_filename (CheeseAudioPlay *audio_play) 
{
  CheeseAudioPlayPrivate *priv = CHEESE_AUDIO_PLAY_GET_PRIVATE (audio_play);  
  char *filename;
  if (priv->counter > 7)
   filename = g_strdup_printf ("file://%s/sounds/shutter%i.ogg", PACKAGE_DATADIR, g_rand_int_range (priv->rand, 1, SHUTTER_SOUNDS));
  else
   filename = g_strdup_printf ("file://%s/sounds/shutter0.ogg", PACKAGE_DATADIR);

  return filename;
}

void
cheese_audio_play_file (CheeseAudioPlay *audio_play) {

  CheeseAudioPlayPrivate *priv = CHEESE_AUDIO_PLAY_GET_PRIVATE (audio_play);  
  priv->counter++;
  gst_element_set_state (priv->play, GST_STATE_NULL);

  char *file = cheese_audio_get_filename (audio_play);
  g_object_set (G_OBJECT (priv->play), "uri", file, NULL);
  g_free (file);

  gst_element_set_state (priv->play, GST_STATE_PLAYING);

}

static void
cheese_audio_play_finalize (GObject *object)
{
  CheeseAudioPlay *audio_play;

  audio_play = CHEESE_AUDIO_PLAY (object);
  CheeseAudioPlayPrivate *priv = CHEESE_AUDIO_PLAY_GET_PRIVATE (audio_play);  

  g_rand_free (priv->rand);
  gst_object_unref (GST_OBJECT (priv->play));

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

  priv->play = gst_element_factory_make ("playbin", "play");

  char *file = cheese_audio_get_filename (audio_play);
  g_object_set (G_OBJECT (priv->play), "uri", file, NULL);
  g_free (file);
}

CheeseAudioPlay * 
cheese_audio_play_new ()
{
  CheeseAudioPlay *audio_play;

  audio_play = g_object_new (CHEESE_TYPE_AUDIO_PLAY, NULL);  
  return audio_play;
}
