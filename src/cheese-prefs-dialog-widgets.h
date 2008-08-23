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

#ifndef _CHEESE_PREFS_DIALOG_WIDGETS_H_
#define _CHEESE_PREFS_DIALOG_WIDGETS_H_

#include <glib-object.h>
#include "cheese-prefs-widget.h"

G_BEGIN_DECLS

#define CHEESE_TYPE_PREFS_DIALOG_WIDGETS (cheese_prefs_dialog_widgets_get_type ())
#define CHEESE_PREFS_DIALOG_WIDGETS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),                            \
                                                                                 CHEESE_TYPE_PREFS_DIALOG_WIDGETS, \
                                                                                 CheesePrefsDialogWidgets))
#define CHEESE_PREFS_DIALOG_WIDGETS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CHEESE_TYPE_PREFS_DIALOG_WIDGETS, \
                                                                              CheesePrefsDialogWidgetsClass))
#define CHEESE_IS_PREFS_DIALOG_WIDGETS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                                                                 CHEESE_TYPE_PREFS_DIALOG_WIDGETS))
#define CHEESE_IS_PREFS_DIALOG_WIDGETS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CHEESE_TYPE_PREFS_DIALOG_WIDGETS))
#define CHEESE_PREFS_DIALOG_WIDGETS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CHEESE_TYPE_PREFS_DIALOG_WIDGETS, \
                                                                                CheesePrefsDialogWidgetsClass))

typedef struct _CheesePrefsDialogWidgetsClass CheesePrefsDialogWidgetsClass;
typedef struct _CheesePrefsDialogWidgets CheesePrefsDialogWidgets;

struct _CheesePrefsDialogWidgetsClass
{
  GObjectClass parent_class;
};

struct _CheesePrefsDialogWidgets
{
  GObject parent_instance;
};

GType                     cheese_prefs_dialog_widgets_get_type (void) G_GNUC_CONST;
CheesePrefsDialogWidgets *cheese_prefs_dialog_widgets_new (CheeseGConf *gconf);

void cheese_prefs_dialog_widgets_add (CheesePrefsDialogWidgets *prefs_widgets,
                                      CheesePrefsWidget        *widget);
void cheese_prefs_dialog_widgets_synchronize (CheesePrefsDialogWidgets *prefs_widgets);

G_END_DECLS

#endif /* _CHEESE_PREFS_DIALOG_WIDGETS_H_ */
