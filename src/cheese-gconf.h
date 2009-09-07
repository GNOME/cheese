/*
 * Copyright © 2007,2008 daniel g. siegel <dgsiegel@gnome.org>
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

#ifndef __CHEESE_GCONF_H__
#define __CHEESE_GCONF_H__

G_BEGIN_DECLS

#define CHEESE_TYPE_GCONF (cheese_gconf_get_type ())
#define CHEESE_GCONF(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHEESE_TYPE_GCONF, CheeseGConf))
#define CHEESE_GCONF_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CHEESE_TYPE_GCONF, CheeseGConfClass))
#define CHEESE_IS_GCONF(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHEESE_TYPE_GCONF))
#define CHEESE_IS_GCONF_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CHEESE_TYPE_GCONF))
#define CHEESE_GCONF_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CHEESE_TYPE_GCONF, CheeseGConfClass))

typedef struct
{
  GObject parent;
} CheeseGConf;

typedef struct
{
  GObjectClass parent_class;
} CheeseGConfClass;

enum
{
  GCONF_PROP_0,
  GCONF_PROP_COUNTDOWN,
  GCONF_PROP_WEBCAM,
  GCONF_PROP_SELECTED_EFFECTS,
  GCONF_PROP_X_RESOLUTION,
  GCONF_PROP_Y_RESOLUTION,
  GCONF_PROP_BRIGHTNESS,
  GCONF_PROP_CONTRAST,
  GCONF_PROP_SATURATION,
  GCONF_PROP_HUE,
  GCONF_PROP_VIDEO_PATH,
  GCONF_PROP_PHOTO_PATH,
  GCONF_PROP_ENABLE_DELETE,
  GCONF_PROP_WIDE_MODE,
  GCONF_PROP_BURST_DELAY,
  GCONF_PROP_BURST_REPEAT
};

GType        cheese_gconf_get_type (void);
CheeseGConf *cheese_gconf_new ();

G_END_DECLS

#endif /* __CHEESE_GCONF_H__ */
