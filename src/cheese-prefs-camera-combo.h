/*
 * Copyright © 2008 James Liggett <jrliggett@cox.net>
 * Copyright © 2008 Ryan Zeigler <zeiglerr@gmail.com>
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

#ifndef _CHEESE_PREFS_CAMERA_COMBO_H_
#define _CHEESE_PREFS_CAMERA_COMBO_H_

#include <glib-object.h>
#include <cheese-camera.h>
#include "cheese-prefs-widget.h"

G_BEGIN_DECLS

#define CHEESE_TYPE_PREFS_CAMERA_COMBO (cheese_prefs_camera_combo_get_type ())
#define CHEESE_PREFS_CAMERA_COMBO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHEESE_TYPE_PREFS_CAMERA_COMBO, \
                                                                               CheesePrefsCameraCombo))
#define CHEESE_PREFS_CAMERA_COMBO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CHEESE_TYPE_PREFS_CAMERA_COMBO, \
                                                                            CheesePrefsCameraComboClass))
#define CHEESE_IS_PREFS_CAMERA_COMBO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHEESE_TYPE_PREFS_CAMERA_COMBO))
#define CHEESE_IS_PREFS_CAMERA_COMBO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CHEESE_TYPE_PREFS_CAMERA_COMBO))
#define CHEESE_PREFS_CAMERA_COMBO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CHEESE_TYPE_PREFS_CAMERA_COMBO, \
                                                                              CheesePrefsCameraComboClass))

typedef struct _CheesePrefsCameraComboClass CheesePrefsCameraComboClass;
typedef struct _CheesePrefsCameraCombo CheesePrefsCameraCombo;

struct _CheesePrefsCameraComboClass
{
  CheesePrefsWidgetClass parent_class;
};

struct _CheesePrefsCameraCombo
{
  CheesePrefsWidget parent_instance;
};

GType                   cheese_prefs_camera_combo_get_type (void) G_GNUC_CONST;
CheesePrefsCameraCombo *cheese_prefs_camera_combo_new (GtkWidget    *combo_box,
                                                       CheeseCamera *camera,
                                                       const gchar  *camera_device_key);

char *cheese_prefs_camera_combo_get_selected_camera (CheesePrefsCameraCombo *camera);

#endif /* _CHEESE_PREFS_CAMERA_COMBO_H_ */
