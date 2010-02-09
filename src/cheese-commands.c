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

#include "cheese-commands.h"
#include <gdk/gdkx.h>

void
cheese_cmd_quit (GtkAction *action, CheeseWindow *window)
{
   gtk_widget_destroy (GTK_WIDGET (window));
}

void
cheese_cmd_bring_to_front (CheeseWindow *window)
{
  guint32       startup_timestamp = gdk_x11_get_server_time (gtk_widget_get_window (GTK_WIDGET (window)));

  gdk_x11_window_set_user_time (gtk_widget_get_window (GTK_WIDGET (window)), startup_timestamp);

  gtk_window_present (GTK_WINDOW (window));
}
