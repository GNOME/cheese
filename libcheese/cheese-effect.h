/*
 * Copyright © 2010 Yuvaraj Pandian T <yuvipanda@yuvi.in>
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

#ifndef CHEESE_EFFECT_H_
#define CHEESE_EFFECT_H_

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * CheeseEffect:
 *
 * Use the accessor functions below.
 */
struct _CheeseEffect
{
  /*< private >*/
  GObject parent;
  void *unused;
};

#define CHEESE_TYPE_EFFECT cheese_effect_get_type ()
G_DECLARE_FINAL_TYPE (CheeseEffect, cheese_effect, CHEESE, EFFECT, GObject)

CheeseEffect *cheese_effect_new (const gchar *name,
                                 const gchar *pipeline_desc);
const gchar * cheese_effect_get_name (CheeseEffect *effect);
const gchar * cheese_effect_get_pipeline_desc (CheeseEffect *effect);
gboolean      cheese_effect_is_preview_connected (CheeseEffect *effect);
void          cheese_effect_enable_preview (CheeseEffect *effect);
void          cheese_effect_disable_preview (CheeseEffect *effect);

CheeseEffect *cheese_effect_load_from_file (const gchar *filename);
GList        *cheese_effect_load_effects (void);

G_END_DECLS

#endif /* CHEESE_EFFECT_H_ */
