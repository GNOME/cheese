
/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * mx-aspect-frame.h: A container that respect the aspect ratio of its child
 *
 * Copyright 2010, 2011 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef __CHEESE_ASPECT_FRAME_H__
#define __CHEESE_ASPECT_FRAME_H__

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define CHEESE_TYPE_ASPECT_FRAME cheese_aspect_frame_get_type()

#define CHEESE_ASPECT_FRAME(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  CHEESE_TYPE_ASPECT_FRAME, CheeseAspectFrame))

#define CHEESE_ASPECT_FRAME_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  CHEESE_TYPE_ASPECT_FRAME, CheeseAspectFrameClass))

#define CHEESE_IS_ASPECT_FRAME(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  CHEESE_TYPE_ASPECT_FRAME))

#define CHEESE_IS_ASPECT_FRAME_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  CHEESE_TYPE_ASPECT_FRAME))

#define CHEESE_ASPECT_FRAME_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  CHEESE_TYPE_ASPECT_FRAME, CheeseAspectFrameClass))

typedef struct _CheeseAspectFrame CheeseAspectFrame;
typedef struct _CheeseAspectFrameClass CheeseAspectFrameClass;
typedef struct _CheeseAspectFramePrivate CheeseAspectFramePrivate;

struct _CheeseAspectFrame
{
  MxBin parent;

  CheeseAspectFramePrivate *priv;
};

struct _CheeseAspectFrameClass
{
  MxBinClass parent_class;

  /* padding for future expansion */
  void (*_padding_0) (void);
  void (*_padding_1) (void);
  void (*_padding_2) (void);
  void (*_padding_3) (void);
  void (*_padding_4) (void);
};

GType           cheese_aspect_frame_get_type    (void) G_GNUC_CONST;

ClutterActor *  cheese_aspect_frame_new         (void);

void            cheese_aspect_frame_set_expand  (CheeseAspectFrame *frame,
                                             gboolean       expand);
gboolean        cheese_aspect_frame_get_expand  (CheeseAspectFrame *frame);

void            cheese_aspect_frame_set_ratio   (CheeseAspectFrame *frame,
                                             gfloat         ratio);
gfloat          cheese_aspect_frame_get_ratio   (CheeseAspectFrame *frame);

G_END_DECLS

#endif /* __CHEESE_ASPECT_FRAME_H__ */
