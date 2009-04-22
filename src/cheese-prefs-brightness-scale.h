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

#ifndef _CHEESE_PREFS_BRIGHTNESS_SCALE_H_
#define _CHEESE_PREFS_BRIGHTNESS_SCALE_H_

#include <glib-object.h>
#include "cheese-prefs-widget.h"
#include "cheese-webcam.h"

G_BEGIN_DECLS

#define CHEESE_TYPE_PREFS_BRIGHTNESS_SCALE (cheese_prefs_brightness_scale_get_type ())
#define CHEESE_PREFS_BRIGHTNESS_SCALE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHEESE_TYPE_PREFS_BRIGHTNESS_SCALE, \
                                                                             CheesePrefsBrightnessScale))
#define CHEESE_PREFS_BRIGHTNESS_SCALE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CHEESE_TYPE_PREFS_BRIGHTNESS_SCALE, \
                                                                          CheesePrefsBrightnessScaleClass))
#define CHEESE_IS_PREFS_BRIGHTNESS_SCALE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHEESE_TYPE_PREFS_BRIGHTNESS_SCALE))
#define CHEESE_IS_PREFS_BRIGHTNESS_SCALE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CHEESE_TYPE_PREFS_BRIGHTNESS_SCALE))
#define CHEESE_PREFS_BRIGHTNESS_SCALE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CHEESE_TYPE_PREFS_BRIGHTNESS_SCALE, CheesePrefsBrightnessScaleClass))

typedef struct _CheesePrefsBrightnessScaleClass CheesePrefsBrightnessScaleClass;
typedef struct _CheesePrefsBrightnessScale CheesePrefsBrightnessScale;

struct _CheesePrefsBrightnessScaleClass
{
  CheesePrefsWidgetClass parent_class;
};

struct _CheesePrefsBrightnessScale
{
  CheesePrefsWidget parent_instance;
};

GType                 cheese_prefs_brightness_scale_get_type (void) G_GNUC_CONST;
CheesePrefsBrightnessScale *cheese_prefs_brightness_scale_new (GtkWidget    *scale,
                                                   CheeseWebcam *webcam);

char *cheese_prefs_brightness_scale_get_selected_webcam (CheesePrefsBrightnessScale *webcam);

#endif /* _CHEESE_PREFS_BRIGHTNESS_SCALE_H_ */
