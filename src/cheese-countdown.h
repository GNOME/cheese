/*
 * Copyright (C) 2008 Mirco "MacSlow" Müller <macslow@bangang.de>
 * Copyright (C) 2007 daniel g. siegel <dgsiegel@gmail.com>
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

#ifndef __CHEESE_COUNTDOWN_H__
#define __CHEESE_COUNTDOWN_H__

G_BEGIN_DECLS

#define CHEESE_TYPE_COUNTDOWN              (cheese_countdown_get_type ())
#define CHEESE_COUNTDOWN(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHEESE_TYPE_COUNTDOWN, CheeseCountdown))
#define CHEESE_COUNTDOWN_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass),  CHEESE_TYPE_COUNTDOWN, CheeseCountdownClass))
#define CHEESE_IS_COUNTDOWN(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHEESE_TYPE_COUNTDOWN))
#define CHEESE_IS_COUNTDOWN_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass),  CHEESE_TYPE_COUNTDOWN))
#define CHEESE_COUNTDOWN_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj),  CHEESE_TYPE_COUNTDOWN, CheeseCountdownClass))

typedef struct 
{
  GtkDrawingArea parent;
} CheeseCountdown;

typedef struct 
{
  GtkDrawingAreaClass parent_class;
} CheeseCountdownClass;

typedef void (*t_cheese_countdown_cb) (gpointer data, gboolean hide);

GType              cheese_countdown_get_type (void);
GtkWidget         *cheese_countdown_new ();

void cheese_countdown_start (CheeseCountdown *countdown, t_cheese_countdown_cb cb, gpointer data);

G_END_DECLS

#endif /* __CHEESE_COUNTDOWN_H__ */
