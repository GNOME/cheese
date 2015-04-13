/*
 * Copyright © 2009 Bastien Nocera <hadess@hadess.net>
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

#ifndef CHEESE_WIDGET_H_
#define CHEESE_WIDGET_H_

#include <glib-object.h>
#include <gtk/gtk.h>
#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>

G_BEGIN_DECLS

#define CHEESE_TYPE_WIDGET (cheese_widget_get_type ())
G_DECLARE_FINAL_TYPE (CheeseWidget, cheese_widget, CHEESE, WIDGET, GtkNotebook)

GtkWidget *cheese_widget_new (void);
void       cheese_widget_get_error (CheeseWidget *widget, GError **error);


/**
 * CheeseWidgetState:
 * @CHEESE_WIDGET_STATE_NONE: Default state, camera uninitialized
 * @CHEESE_WIDGET_STATE_READY: The camera should be ready and the widget should be displaying the preview
 * @CHEESE_WIDGET_STATE_ERROR: An error occurred while setting up the camera, check what went wrong with cheese_widget_get_error()
 *
 * Current #CheeseWidget state.
 *
 */
typedef enum
{
  CHEESE_WIDGET_STATE_NONE,
  CHEESE_WIDGET_STATE_READY,
  CHEESE_WIDGET_STATE_ERROR
} CheeseWidgetState;

G_END_DECLS

#endif /* CHEESE_WIDGET_H_ */
