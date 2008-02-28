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

#ifndef __GST_AUDIO_PLAY_H__
#define __GST_AUDIO_PLAY_H__

#include <glib-object.h>

#define GST_TYPE_AUDIO_PLAY           (gst_audio_play_get_type())
#define GST_AUDIO_PLAY(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_AUDIO_PLAY, GstAudioPlay))
#define GST_AUDIO_PLAY_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST((cls), GST_TYPE_AUDIO_PLAY, GstAudioPlayClass))
#define GST_IS_AUDIO_PLAY(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_AUDIO_PLAY))
#define GST_IS_AUDIO_PLAY_CLASS(cls)  (G_TYPE_CHECK_CLASS_TYPE((cls), GST_TYPE_AUDIO_PLAY))
#define GST_AUDIO_PLAY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_AUDIO_PLAY, GstAudioPlayClass))

G_BEGIN_DECLS

typedef struct
{
  GObject object;
} GstAudioPlay;

typedef struct
{
  GObjectClass parent_class;

  /* signals */
  void (*done) (GstAudioPlay *audio_play, GError *error);
} GstAudioPlayClass;

GType         gst_audio_play_get_type 	(void) G_GNUC_CONST;
GstAudioPlay *gst_audio_play_new      	(const gchar *uri);

void          gst_audio_play_start    	(GstAudioPlay *audio_play);
void          gst_audio_play_stop     	(GstAudioPlay *audio_play);

GstAudioPlay *gst_audio_play_file      	(const gchar *filename,
                                         GError **error);

G_END_DECLS

#endif /* __GST_AUDIO_PLAY_H__ */
