/* Eye Of Gnome - Thumbnail Navigator
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef EOG_THUMB_NAV_H_
#define EOG_THUMB_NAV_H_

#include "cheese-thumb-view.h"

#include <gtk/gtk.h>
#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define EOG_TYPE_THUMB_NAV (eog_thumb_nav_get_type ())
G_DECLARE_FINAL_TYPE (EogThumbNav, eog_thumb_nav, EOG, THUMB_NAV, GtkBox)

GtkWidget *eog_thumb_nav_new (GtkWidget *thumbview,
                              gboolean   show_buttons);

gboolean eog_thumb_nav_get_show_buttons (EogThumbNav *nav);

void eog_thumb_nav_set_show_buttons (EogThumbNav *nav,
                                     gboolean     show_buttons);

gboolean eog_thumb_nav_is_vertical (EogThumbNav *nav);

void eog_thumb_nav_set_vertical (EogThumbNav *nav,
                                 gboolean     vertical);

void eog_thumb_nav_set_policy (EogThumbNav  *nav,
                               GtkPolicyType hscrollbar_policy,
                               GtkPolicyType vscrollbar_policy);

G_END_DECLS

#endif /* EOG_THUMB_NAV_H__ */
