/*
 * Copyright © 2009 Bastien Nocera <hadess@hadess.net>
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

#ifndef _CHEESE_WIDGET_PRIVATE_H_
#define _CHEESE_WIDGET_PRIVATE_H_

#include "cheese-widget.h"

G_BEGIN_DECLS

enum
{
  SPINNER_PAGE = 0,
  WEBCAM_PAGE  = 1,
  PROBLEM_PAGE = 2,
  LAST_PAGE    = 3,
};

GObject   *cheese_widget_get_camera (CheeseWidget *widget);
GSettings *cheese_widget_get_settings (CheeseWidget *widget);
GtkWidget *cheese_widget_get_video_area (CheeseWidget *widget);

G_END_DECLS

#endif /* _CHEESE_WIDGET_PRIVATE_H_ */
