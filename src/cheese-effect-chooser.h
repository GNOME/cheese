/*
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

#ifndef __CHEESE_EFFECT_CHOOSER_H__
#define __CHEESE_EFFECT_CHOOSER_H__

#include <gtk/gtk.h>
#include "cheese-webcam.h"

G_BEGIN_DECLS

#define CHEESE_TYPE_EFFECT_CHOOSER (cheese_effect_chooser_get_type ())
#define CHEESE_EFFECT_CHOOSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHEESE_TYPE_EFFECT_CHOOSER, \
                                                                           CheeseEffectChooser))
#define CHEESE_EFFECT_CHOOSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CHEESE_TYPE_EFFECT_CHOOSER, \
                                                                        CheeseEffectChooserClass))
#define CHEESE_IS_EFFECT_CHOOSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHEESE_TYPE_EFFECT_CHOOSER))
#define CHEESE_IS_EFFECT_CHOOSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CHEESE_TYPE_EFFECT_CHOOSER))
#define CHEESE_EFFECT_CHOOSER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CHEESE_TYPE_EFFECT_CHOOSER, \
                                                                          CheeseEffectChooserClass))

typedef struct
{
  GtkDrawingArea parent;
} CheeseEffectChooser;

typedef struct
{
  GtkDrawingAreaClass parent_class;
} CheeseEffectChooserClass;


GType      cheese_effect_chooser_get_type (void);
GtkWidget *cheese_effect_chooser_new (char *selected_effects);

CheeseWebcamEffect cheese_effect_chooser_get_selection (CheeseEffectChooser *effect_chooser);
char *             cheese_effect_chooser_get_selection_string (CheeseEffectChooser *effect_chooser);
void               cheese_effect_chooser_unselect_all (CheeseEffectChooser *effect_chooser);

G_END_DECLS

#endif /* __CHEESE_EFFECT_CHOOSER_H__ */
