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

#ifndef __CHEESE_WINDOW_H__
#define __CHEESE_WINDOW_H__

#include <gtk/gtk.h>
#include <glade/glade.h>

struct _widgets
{
  GtkWidget *take_picture;
  GtkWidget *screen;
  GtkWidget *notebook;
  GtkWidget *table;
  GtkWidget *button_video;
  GtkWidget *button_photo;
  GtkWidget *button_effects;
  GtkWidget *label_effects;
  GtkWidget *label_photo;
  GtkWidget *label_video;
  GtkWidget *label_take_photo;
  GtkWidget *menubar;
  GtkWidget *file_menu;
  GtkWidget *help_menu;
  GtkWidget *effects_widget;
  GtkWidget *image_take_photo;
  GtkWidget *popup_menu;
};

struct _cheese_window
{
  GladeXML *gxml;
  GtkWidget *window;
  struct _widgets widgets;
};

extern struct _cheese_window cheese_window;

void cheese_window_init();
void cheese_window_finalize();
void cheese_window_change_effect(GtkWidget *, gpointer);
gboolean cheese_window_expose_cb(GtkWidget *, GdkEventExpose *, gpointer);
int show_move_to_trash_confirm_dialog(gchar *);

#endif /* __CHEESE_WINDOW_H__ */
