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

#ifndef _CHEESE_PREFS_BURST_SPINBOX_H_
#define _CHEESE_PREFS_BURST_SPINBOX_H_

#include <glib-object.h>
#include "cheese-prefs-widget.h"
#include "cheese-webcam.h"

G_BEGIN_DECLS

#define CHEESE_TYPE_PREFS_BURST_SPINBOX (cheese_prefs_burst_spinbox_get_type ())
#define CHEESE_PREFS_BURST_SPINBOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHEESE_TYPE_PREFS_BURST_SPINBOX, \
                                                                                CheesePrefsBurstSpinbox))
#define CHEESE_PREFS_BURST_SPINBOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CHEESE_TYPE_PREFS_BURST_SPINBOX, \
                                                                             CheesePrefsBurstSpinboxClass))
#define CHEESE_IS_PREFS_BURST_SPINBOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHEESE_TYPE_PREFS_BURST_SPINBOX))
#define CHEESE_IS_PREFS_BURST_SPINBOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CHEESE_TYPE_PREFS_BURST_SPINBOX))
#define CHEESE_PREFS_BURST_SPINBOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CHEESE_TYPE_PREFS_BURST_SPINBOX, \
                                                                               CheesePrefsBurstSpinboxClass))

typedef struct _CheesePrefsBurstSpinboxClass CheesePrefsBurstSpinboxClass;
typedef struct _CheesePrefsBurstSpinbox CheesePrefsBurstSpinbox;

struct _CheesePrefsBurstSpinboxClass
{
  CheesePrefsWidgetClass parent_class;
};

struct _CheesePrefsBurstSpinbox
{
  CheesePrefsWidget parent_instance;
};

GType                    cheese_prefs_burst_spinbox_get_type (void) G_GNUC_CONST;
CheesePrefsBurstSpinbox *cheese_prefs_burst_spinbox_new (GtkWidget   *scale,
                                                         const gchar *gconf_key);

#endif /* _CHEESE_PREFS_BURST_SPINBOX_H_ */
