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

#ifndef __CHEESE_UI_H__
#define __CHEESE_UI_H__

#include <glib.h>
#include <gtk/gtk.h>
#include "cheese-commands.h"

G_BEGIN_DECLS

const GtkActionEntry action_entries_main[] = {
  {"Cheese",       NULL,            N_("_Cheese")                       },

  {"Edit",         NULL,            N_("_Edit")                         },
  {"Help",         NULL,            N_("_Help")                         },

  {"Quit",         GTK_STOCK_QUIT,  NULL, NULL, NULL, G_CALLBACK (cheese_cmd_quit)},
  {"HelpContents", GTK_STOCK_HELP,  N_("_Contents"), "F1", N_("Help on this Application"),
   G_CALLBACK (cheese_cmd_help_contents)},
  {"About",        GTK_STOCK_ABOUT, NULL, NULL, NULL, G_CALLBACK (cheese_cmd_about)},
};

const GtkActionEntry action_entries_prefs[] = {
  {"Preferences",  GTK_STOCK_PREFERENCES, N_("Preferences"), NULL, NULL, G_CALLBACK (cheese_window_preferences_cb)},
};

const GtkRadioActionEntry action_entries_toggle[] = {
  {"Photo", NULL, N_("_Photo"), NULL, NULL, 0},
  {"Video", NULL, N_("_Video"), NULL, NULL, 1},
  {"Burst", NULL, N_("_Burst"), NULL, NULL, 2},
};

const GtkToggleActionEntry action_entries_countdown[] = {
  {"Countdown", NULL, N_("Countdown"), NULL, NULL, G_CALLBACK (cheese_window_toggle_countdown), FALSE},
};
const GtkToggleActionEntry action_entries_flash[] = {
  {"Flash", NULL, N_("Flash"), NULL, NULL, G_CALLBACK (cheese_window_toggle_flash), FALSE},
};

const GtkToggleActionEntry action_entries_effects[] = {
  {"Effects", NULL, N_("_Effects"), NULL, NULL, G_CALLBACK (cheese_window_effect_button_pressed_cb), FALSE},
};

const GtkToggleActionEntry action_entries_fullscreen[] = {
  {"Fullscreen", GTK_STOCK_FULLSCREEN, NULL, "F11", NULL, G_CALLBACK (cheese_window_toggle_fullscreen), FALSE},
};
const GtkToggleActionEntry action_entries_wide_mode[] = {
  {"WideMode", NULL, N_("_Wide mode"), NULL, NULL, G_CALLBACK (cheese_window_toggle_wide_mode), FALSE},
};

const GtkActionEntry action_entries_photo[] = {
  {"TakePhoto", NULL, N_("_Take a Photo"), "space", NULL, G_CALLBACK (cheese_window_action_button_clicked_cb)},
};
const GtkToggleActionEntry action_entries_video[] = {
  {"TakeVideo", NULL, N_("_Recording"), "space", NULL, G_CALLBACK (cheese_window_action_button_clicked_cb), FALSE},
};
const GtkActionEntry action_entries_burst[] = {
  {"TakeBurst", NULL, N_("_Take multiple Photos"), "space", NULL, G_CALLBACK (cheese_window_action_button_clicked_cb)},
};

const GtkActionEntry action_entries_file[] = {
  {"Open",        GTK_STOCK_OPEN,    N_("_Open"),          "<control>O",    NULL,
   G_CALLBACK (cheese_cmd_file_open)},
  {"SaveAs",      GTK_STOCK_SAVE_AS, N_("Save _As…"),      "<control>S",    NULL,
   G_CALLBACK (cheese_cmd_file_save_as)},
  {"MoveToTrash", "user-trash",      N_("Move to _Trash"), "Delete",        NULL,
   G_CALLBACK (cheese_cmd_file_move_to_trash)},
  {"Delete",      NULL,              N_("Delete"),         "<shift>Delete", NULL,
   G_CALLBACK (cheese_cmd_file_delete)},
};

static const GtkActionEntry action_entries_trash[] = {
  {"RemoveAll",    NULL,             N_("Move All to Trash"), NULL, NULL,
   G_CALLBACK (cheese_cmd_file_move_all_to_trash)},
};

G_END_DECLS

#endif /* __CHEESE_UI_H__ */
