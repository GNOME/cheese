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

#ifndef __CHEESE_COMMAND_HANDLER_H__
#define __CHEESE_COMMAND_HANDLER_H__

void cheese_command_handler_init();
void cheese_command_handler_finalize();
void cheese_command_handler_url_show(GtkWidget *, GtkTreePath *path);
void cheese_command_handler_run_command_from_string(GtkWidget *, gchar *);
void cheese_command_handler_move_to_trash(GtkWidget *, gchar *);

#endif /* __CHEESE_COMMAND_HANDLER_H__ */
