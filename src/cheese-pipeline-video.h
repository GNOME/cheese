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

#ifndef __CHEESE_PIPELINE_VIDEO_H__
#define __CHEESE_PIPELINE_VIDEO_H__
#include <glib.h>
#include <glib-object.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define PIPELINE_VIDEO_TYPE             (cheese_pipeline_video_get_type())
#define PIPELINE_VIDEO(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), PIPELINE_VIDEO_TYPE, PipelineVideo))
#define PIPELINE_VIDEO_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), PIPELINE_VIDEO_TYPE, PipelineVideoClass))
#define IS_PIPELINE_VIDEO(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PIPELINE_VIDEO_TYPE))
#define IS_PIPELINE_VIDEO_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), PIPELINE_VIDEO_TYPE))
#define PIPELINE_VIDEO_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), PIPELINE_VIDEO_TYPE, PipelineVideoClass))

typedef struct _PipelineVideo PipelineVideo;
typedef struct _PipelineVideoClass PipelineVideoClass;

struct _PipelineVideo 
{
	GObject parent;
};

struct _PipelineVideoClass 
{
	GObjectClass parent_class;
};

PipelineVideo*    cheese_pipeline_video_new               (void);
GType             cheese_pipeline_video_get_type          (void);
void              cheese_pipeline_video_set_play          (PipelineVideo *self);
void              cheese_pipeline_video_set_stop          (PipelineVideo *self);
void              cheese_pipeline_video_create            (gchar *source_pipeline, PipelineVideo *self);
GstElement       *cheese_pipeline_video_get_ximagesink    (PipelineVideo *self);
GstElement       *cheese_pipeline_video_get_fakesink      (PipelineVideo *self);
GstElement       *cheese_pipeline_video_get_pipeline      (PipelineVideo *self);
void              cheese_pipeline_video_button_clicked    (GtkWidget *widget, gpointer self);
void              cheese_pipeline_video_change_effect     (gchar *effect, gpointer self);

G_END_DECLS


#endif /* __CHEESE_PIPELINE_VIDEO_H__ */
