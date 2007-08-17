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

#ifndef __CHEESE_PIPELINE_PHOTO_H__
#define __CHEESE_PIPELINE_PHOTO_H__
#include <glib.h>
#include <glib-object.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define PIPELINE_PHOTO_TYPE             (cheese_pipeline_photo_get_type())
#define PIPELINE_PHOTO(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), PIPELINE_PHOTO_TYPE, PipelinePhoto))
#define PIPELINE_PHOTO_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), PIPELINE_PHOTO_TYPE, PipelinePhotoClass))
#define IS_PIPELINE_PHOTO(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PIPELINE_PHOTO_TYPE))
#define IS_PIPELINE_PHOTO_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), PIPELINE_PHOTO_TYPE))
#define PIPELINE_PHOTO_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), PIPELINE_PHOTO_TYPE, PipelinePhotoClass))

typedef struct _PipelinePhoto PipelinePhoto;
typedef struct _PipelinePhotoClass PipelinePhotoClass;

struct _PipelinePhoto
{
  GObject parent;

  GstElement *pipeline;
  GstElement *ximagesink;
  GstElement *fakesink;
};

struct _PipelinePhotoClass
{
  GObjectClass parent_class;
};

PipelinePhoto  *cheese_pipeline_photo_new (void);
GType           cheese_pipeline_photo_get_type (void);
void            cheese_pipeline_photo_set_play (PipelinePhoto *);
void            cheese_pipeline_photo_set_stop (PipelinePhoto *);
void            cheese_pipeline_photo_create (gchar *, PipelinePhoto *);
GstElement     *cheese_pipeline_photo_get_ximagesink (PipelinePhoto *);
GstElement     *cheese_pipeline_photo_get_fakesink (PipelinePhoto *);
GstElement     *cheese_pipeline_photo_get_pipeline (PipelinePhoto *);
void            cheese_pipeline_photo_button_clicked (GtkWidget *, gpointer);
void            cheese_pipeline_photo_change_effect (gchar *, gpointer);

G_END_DECLS

#endif /* __CHEESE_PIPELINE_PHOTO_H__ */
