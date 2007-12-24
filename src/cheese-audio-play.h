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

#ifndef __CHEESE_AUDIO_PLAY_H__
#define __CHEESE_AUDIO_PLAY_H__

G_BEGIN_DECLS

#define CHEESE_TYPE_AUDIO_PLAY            (cheese_audio_play_get_type ())
#define CHEESE_AUDIO_PLAY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHEESE_TYPE_AUDIO_PLAY, CheeseAudioPlay))
#define CHEESE_AUDIO_PLAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CHEESE_TYPE_AUDIO_PLAY, CheeseAudioPlayClass))
#define CHEESE_IS_AUDIO_PLAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHEESE_TYPE_AUDIO_PLAY))
#define CHEESE_IS_AUDIO_PLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CHEESE_TYPE_AUDIO_PLAY))
#define CHEESE_AUDIO_PLAY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CHEESE_TYPE_AUDIO_PLAY, CheeseAudioPlayClass))

typedef struct 
{
  GObject parent;
} CheeseAudioPlay;

typedef struct
{
  GObjectClass parent_class;
} CheeseAudioPlayClass;

GType             cheese_audio_play_get_type(void);
CheeseAudioPlay  *cheese_audio_play_new ();
void              cheese_audio_play_file (CheeseAudioPlay *);

G_END_DECLS

#endif /* __CHEESE_AUDIO_PLAY_H__ */

