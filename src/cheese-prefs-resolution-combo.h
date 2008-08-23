/*
 * Copyright © 2008 James Liggett <jrliggett@cox.net>
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

#ifndef _CHEESE_PREFS_RESOLUTION_COMBO_H_
#define _CHEESE_PREFS_RESOLUTION_COMBO_H_

#include <glib-object.h>
#include "cheese-prefs-widget.h"
#include "cheese-webcam.h"

G_BEGIN_DECLS

#define CHEESE_TYPE_PREFS_RESOLUTION_COMBO (cheese_prefs_resolution_combo_get_type ())
#define CHEESE_PREFS_RESOLUTION_COMBO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),                              \
                                                                                   CHEESE_TYPE_PREFS_RESOLUTION_COMBO, \
                                                                                   CheesePrefsResolutionCombo))
#define CHEESE_PREFS_RESOLUTION_COMBO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),                            \
                                                                                CHEESE_TYPE_PREFS_RESOLUTION_COMBO, \
                                                                                CheesePrefsResolutionComboClass))
#define CHEESE_IS_PREFS_RESOLUTION_COMBO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                                                                   CHEESE_TYPE_PREFS_RESOLUTION_COMBO))
#define CHEESE_IS_PREFS_RESOLUTION_COMBO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                                                                CHEESE_TYPE_PREFS_RESOLUTION_COMBO))
#define CHEESE_PREFS_RESOLUTION_COMBO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),                              \
                                                                                  CHEESE_TYPE_PREFS_RESOLUTION_COMBO, \
                                                                                  CheesePrefsResolutionComboClass))

typedef struct _CheesePrefsResolutionComboClass CheesePrefsResolutionComboClass;
typedef struct _CheesePrefsResolutionCombo CheesePrefsResolutionCombo;

struct _CheesePrefsResolutionComboClass
{
  CheesePrefsWidgetClass parent_class;
};

struct _CheesePrefsResolutionCombo
{
  CheesePrefsWidget parent_instance;
};

GType                       cheese_prefs_resolution_combo_get_type (void) G_GNUC_CONST;
CheesePrefsResolutionCombo *cheese_prefs_resolution_combo_new (GtkWidget    *combo_box,
                                                               CheeseWebcam *webcam,
                                                               const gchar  *x_resolution_key,
                                                               const gchar  *y_resolution_key,
                                                               unsigned int  max_x_resolution,
                                                               unsigned int  max_y_resolution);

CheeseVideoFormat *cheese_prefs_resolution_combo_get_selected_format (CheesePrefsResolutionCombo *resolution_combo);

G_END_DECLS

#endif /* _CHEESE_PREFS_RESOLUTION_COMBO_H_ */
