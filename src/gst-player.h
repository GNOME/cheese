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

#ifndef __GST_PLAYER_H__
#define __GST_PLAYER_H__

#include <glib-object.h>

#define GST_TYPE_PLAYER           (gst_player_get_type())
#define GST_PLAYER(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_PLAYER, GstPlayer))
#define GST_PLAYER_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST((cls), GST_TYPE_PLAYER, GstPlayerClass))
#define GST_IS_PLAYER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_PLAYER))
#define GST_IS_PLAYER_CLASS(cls)  (G_TYPE_CHECK_CLASS_TYPE((cls), GST_TYPE_PLAYER))
#define GST_PLAYER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_PLAYER, GstPlayerClass))

G_BEGIN_DECLS

typedef struct _GstPlayer         GstPlayer;
typedef struct _GstPlayerClass    GstPlayerClass;
typedef struct _GstPlayerPrivate  GstPlayerPrivate;

struct _GstPlayer
{
  GObject object;

  /*< private >*/
  GstPlayerPrivate *priv;
};

struct _GstPlayerClass
{
  GObjectClass parent_class;

  /* signals */
  void (*done) (GstPlayer *player, GError *error);
};

GType      gst_player_get_type (void) G_GNUC_CONST;
GstPlayer* gst_player_new      (const gchar *uri);

void       gst_player_start    (GstPlayer   *player);
void       gst_player_stop     (GstPlayer   *player);

GstPlayer* gst_play_file       (const gchar *filename,
                                GError     **error);

G_END_DECLS

#endif /* __GST_PLAY_H__ */
