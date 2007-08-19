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

#ifndef __CHEESE_PIPELINE_H__
#define __CHEESE_PIPELINE_H__

#include <gst/gst.h>

void cheese_pipeline_init (void);
void cheese_pipeline_finalize (void);
GstElement *cheese_pipeline_get_fakesink (void);
GstElement *cheese_pipeline_get_pipeline (void);
GstElement *cheese_pipeline_get_ximagesink (void);
gboolean    cheese_pipeline_countdown_is_active (void);
gboolean    cheese_pipeline_pipeline_is_photo (void);
void        cheese_pipeline_button_clicked (GtkWidget *);
void        cheese_pipeline_change_effect (void);
void        cheese_pipeline_change_pipeline_type (void);
void        cheese_pipeline_create (void);
void        cheese_pipeline_set_countdown (gboolean);
void        cheese_pipeline_set_play (void);
void        cheese_pipeline_set_stop (void);

#endif /* __CHEESE_PIPELINE_H__ */
