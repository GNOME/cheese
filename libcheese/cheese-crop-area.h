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

#ifndef CHEESE_CROP_AREA_H_
#define CHEESE_CROP_AREA_H_

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CHEESE_TYPE_CROP_AREA (cheese_crop_area_get_type ())
#define CHEESE_CROP_AREA(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHEESE_TYPE_CROP_AREA, \
                                                           CheeseCropArea))
#define CHEESE_CROP_AREA_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), CHEESE_TYPE_CROP_AREA, \
                                                                CheeseCropAreaClass))
#define CHEESE_IS_CROP_AREA(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHEESE_TYPE_CROP_AREA))
#define CHEESE_IS_CROP_AREA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CHEESE_TYPE_CROP_AREA))
#define CHEESE_CROP_AREA_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CHEESE_TYPE_CROP_AREA, \
                                                                    CheeseCropAreaClass))

typedef struct _CheeseCropAreaClass CheeseCropAreaClass;
typedef struct _CheeseCropArea CheeseCropArea;

/*
 * CheeseCropAreaClass:
 *
 * Use the accessor functions below.
 */
struct _CheeseCropAreaClass {
	/*< private >*/
    GtkDrawingAreaClass parent_class;
};

/*
 * CheeseCropArea:
 *
 * Use the accessor functions below.
 */
struct _CheeseCropArea {
	/*< private >*/
    GtkDrawingArea parent_instance;
};

GType cheese_crop_area_get_type (void);

GtkWidget *cheese_crop_area_new (void);
GdkPixbuf *cheese_crop_area_get_picture (CheeseCropArea *area);
void cheese_crop_area_set_picture (CheeseCropArea *area,
                                   GdkPixbuf  *pixbuf);
void cheese_crop_area_set_min_size (CheeseCropArea *area,
                                    gint width,
                                    gint height);
void cheese_crop_area_set_constrain_aspect (CheeseCropArea *area,
                                            gboolean constrain);

G_END_DECLS

#endif /* CHEESE_CROP_AREA_H_ */
