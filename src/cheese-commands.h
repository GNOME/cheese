/*
 * Copyright © 2010 Filippo Argiolas <filippo.argiolas@gmail.com>
 * Copyright © 2007,2008 daniel g. siegel <dgsiegel@gnome.org>
 * Copyright © 2007,2008 Jaap Haitsma <jaap@haitsma.org>
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

#ifndef __CHEESE_COMMANDS_H__
#define __CHEESE_COMMANDS_H__

#ifdef HAVE_CONFIG_H
  #include "cheese-config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "cheese-window.h"

G_BEGIN_DECLS

void cheese_cmd_quit (GtkAction *action, CheeseWindow *window);
void cheese_cmd_bring_to_front (CheeseWindow *window);
void cheese_cmd_help_contents (GtkAction *action, CheeseWindow *cheese_window);
void cheese_cmd_about (GtkAction *action, CheeseWindow *cheese_window);
void cheese_cmd_file_open (GtkWidget *widget, CheeseWindow *cheese_window);
void cheese_cmd_file_save_as (GtkWidget *widget, CheeseWindow *cheese_window);
void cheese_cmd_file_move_to_trash (GtkWidget *widget, CheeseWindow *cheese_window);
void cheese_cmd_file_move_all_to_trash (GtkWidget *widget, CheeseWindow *cheese_window);
void cheese_cmd_file_delete (GtkWidget *widget, CheeseWindow *cheese_window);

G_END_DECLS

#endif /* __CHEESE_COMMANDS_H__ */
