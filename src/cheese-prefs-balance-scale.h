/*
 * Copyright © 2009 Filippo Argiolas <filippo.argiolas@gmail.com>
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

#ifndef _CHEESE_PREFS_BALANCE_SCALE_H_
#define _CHEESE_PREFS_BALANCE_SCALE_H_

#include <glib-object.h>
#include "cheese-prefs-widget.h"
#include "cheese-webcam.h"

G_BEGIN_DECLS

#define CHEESE_TYPE_PREFS_BALANCE_SCALE (cheese_prefs_balance_scale_get_type ())
#define CHEESE_PREFS_BALANCE_SCALE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHEESE_TYPE_PREFS_BALANCE_SCALE, \
                                                                                CheesePrefsBalanceScale))
#define CHEESE_PREFS_BALANCE_SCALE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CHEESE_TYPE_PREFS_BALANCE_SCALE, \
                                                                             CheesePrefsBalanceScaleClass))
#define CHEESE_IS_PREFS_BALANCE_SCALE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHEESE_TYPE_PREFS_BALANCE_SCALE))
#define CHEESE_IS_PREFS_BALANCE_SCALE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CHEESE_TYPE_PREFS_BALANCE_SCALE))
#define CHEESE_PREFS_BALANCE_SCALE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CHEESE_TYPE_PREFS_BALANCE_SCALE, \
                                                                               CheesePrefsBalanceScaleClass))

typedef struct _CheesePrefsBalanceScaleClass CheesePrefsBalanceScaleClass;
typedef struct _CheesePrefsBalanceScale CheesePrefsBalanceScale;

struct _CheesePrefsBalanceScaleClass
{
  CheesePrefsWidgetClass parent_class;
};

struct _CheesePrefsBalanceScale
{
  CheesePrefsWidget parent_instance;
};

GType                    cheese_prefs_balance_scale_get_type (void) G_GNUC_CONST;
CheesePrefsBalanceScale *cheese_prefs_balance_scale_new (GtkWidget    *scale,
                                                         CheeseWebcam *webcam,
                                                         const gchar  *property,
                                                         const gchar  *balance_key);

#endif /* _CHEESE_PREFS_BALANCE_SCALE_H_ */
