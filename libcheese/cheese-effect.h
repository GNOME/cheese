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

#ifndef _CHEESE_EFFECT_H_
#define _CHEESE_EFFECT_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define CHEESE_TYPE_EFFECT cheese_effect_get_type ()

#define CHEESE_EFFECT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHEESE_TYPE_EFFECT, CheeseEffect))

#define CHEESE_EFFECT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), CHEESE_TYPE_EFFECT, CheeseEffectClass))

#define CHEESE_IS_EFFECT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHEESE_TYPE_EFFECT))

#define CHEESE_IS_EFFECT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), CHEESE_TYPE_EFFECT))

#define CHEESE_EFFECT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), CHEESE_TYPE_EFFECT, CheeseEffectClass))

typedef struct
{
  GObject parent;
} CheeseEffect;

typedef struct
{
  GObjectClass parent_class;
} CheeseEffectClass;

GType cheese_effect_get_type (void);

CheeseEffect *cheese_effect_new (void);
gboolean      cheese_effect_is_preview_connected (CheeseEffect *self);
void          cheese_effect_enable_preview (CheeseEffect *self);
void          cheese_effect_disable_preview (CheeseEffect *self);

CheeseEffect *cheese_effect_load_from_file (const gchar *fname);
GList        *cheese_effect_load_effects ();

G_END_DECLS

#endif /* _CHEESE_EFFECT_H_ */
