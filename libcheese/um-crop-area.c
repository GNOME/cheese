/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright 2009  Red Hat, Inc,
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Written by: Matthias Clasen <mclasen@redhat.com>
 */

#include <stdlib.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "um-crop-area.h"

struct _UmCropAreaPrivate {
	GdkPixbuf *browse_pixbuf;
	GdkPixbuf *pixbuf;
	GdkPixbuf *color_shifted;
	gdouble scale;
	GdkRectangle image;
	GdkCursorType current_cursor;
	GdkRectangle crop;
	gint active_region;
	gint last_press_x;
	gint last_press_y;
};

G_DEFINE_TYPE (UmCropArea, um_crop_area, GTK_TYPE_DRAWING_AREA);

static inline guchar
shift_color_byte (guchar b,
                  int    shift)
{
	return CLAMP(b + shift, 0, 255);
}

static void
shift_colors (GdkPixbuf *pixbuf,
	      gint       red,
	      gint       green,
	      gint       blue,
	      gint       alpha)
{
	gint x, y, offset, y_offset, rowstride, width, height;
	guchar *pixels;
	gint channels;

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	pixels = gdk_pixbuf_get_pixels (pixbuf);
	channels = gdk_pixbuf_get_n_channels (pixbuf);

	for (y = 0; y < height; y++) {
		y_offset = y * rowstride;
		for (x = 0; x < width; x++) {
			offset = y_offset + x * channels;
			if (red != 0)
				pixels[offset] = shift_color_byte (pixels[offset], red);
			if (green != 0)
				pixels[offset + 1] = shift_color_byte (pixels[offset + 1], green);
			if (blue != 0)
				pixels[offset + 2] = shift_color_byte (pixels[offset + 2], blue);
			if (alpha != 0 && channels >= 4)
				pixels[offset + 3] = shift_color_byte (pixels[offset + 3], blue);
		}
	}
}

static void
update_pixbufs (UmCropArea *area)
{
	GtkAllocation allocation;
	GtkStyle *style;
	GtkWidget *widget;
	GdkColor *color;
	gdouble scale;
	gint width;
	gint height;
	gint dest_x, dest_y, dest_width, dest_height;
	guint32 pixel;

	widget = GTK_WIDGET (area);
	gtk_widget_get_allocation (widget, &allocation);

	if (area->priv->pixbuf == NULL ||
	    gdk_pixbuf_get_width (area->priv->pixbuf) != allocation.width ||
	    gdk_pixbuf_get_height (area->priv->pixbuf) != allocation.height) {
		if (area->priv->pixbuf != NULL)
			g_object_unref (area->priv->pixbuf);
		area->priv->pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8,
					     allocation.width, allocation.height);

		style = gtk_widget_get_style (widget);
		color = &style->bg[gtk_widget_get_state (widget)];
  		pixel = ((color->red & 0xff00) << 16) |
       			((color->green & 0xff00) << 8) |
         		 (color->blue & 0xff00);
		gdk_pixbuf_fill (area->priv->pixbuf, pixel);

		width = gdk_pixbuf_get_width (area->priv->browse_pixbuf);
		height = gdk_pixbuf_get_height (area->priv->browse_pixbuf);

		scale = allocation.height / (gdouble)height;
		if (scale * width > allocation.width)
		    scale = allocation.width / (gdouble)width;

		dest_width = width * scale;
		dest_height = height * scale;
		dest_x = (allocation.width - dest_width) / 2;
		dest_y = (allocation.height - dest_height) / 2,

		gdk_pixbuf_scale (area->priv->browse_pixbuf,
				  area->priv->pixbuf,
				  dest_x, dest_y,
				  dest_width, dest_height,
				  dest_x, dest_y,
				  scale, scale,
				  GDK_INTERP_BILINEAR);

		if (area->priv->color_shifted)
			g_object_unref (area->priv->color_shifted);
		area->priv->color_shifted = gdk_pixbuf_copy (area->priv->pixbuf);
		shift_colors (area->priv->color_shifted, -32, -32, -32, 0);

		if (area->priv->scale == 0.0) {
			area->priv->crop.width = 96.0 / scale;
			area->priv->crop.height = 96.0 / scale;
			area->priv->crop.x = (gdk_pixbuf_get_width (area->priv->browse_pixbuf) - area->priv->crop.width) / 2;
			area->priv->crop.y = (gdk_pixbuf_get_height (area->priv->browse_pixbuf) - area->priv->crop.height) / 2;
		}

		area->priv->scale = scale;
		area->priv->image.x = dest_x;
		area->priv->image.y = dest_y;
		area->priv->image.width = dest_width;
		area->priv->image.height = dest_height;
	}
}

static void
crop_to_widget (UmCropArea    *area,
                GdkRectangle  *crop)
{
	crop->x = area->priv->image.x + area->priv->crop.x * area->priv->scale;
	crop->y = area->priv->image.y + area->priv->crop.y * area->priv->scale;
	crop->width = area->priv->crop.width * area->priv->scale;
	crop->height = area->priv->crop.height * area->priv->scale;
}

typedef enum {
	OUTSIDE,
	INSIDE,
	TOP,
	TOP_LEFT,
	TOP_RIGHT,
	BOTTOM,
	BOTTOM_LEFT,
	BOTTOM_RIGHT,
	LEFT,
	RIGHT
} Location;

static gboolean
um_crop_area_expose (GtkWidget      *widget,
		     GdkEventExpose *event)
{
	GtkStateType state;
	GtkStyle *style;
	cairo_t *cr;
	GdkRectangle area;
	GdkRectangle crop;
	GdkWindow *window;
	gint width, height;
	UmCropArea *uarea = UM_CROP_AREA (widget);

	if (uarea->priv->browse_pixbuf == NULL)
		return FALSE;

	update_pixbufs (uarea);

	window = gtk_widget_get_window (widget);
	style = gtk_widget_get_style (widget);
	state = gtk_widget_get_state (widget);

	width = gdk_pixbuf_get_width (uarea->priv->pixbuf);
	height = gdk_pixbuf_get_height (uarea->priv->pixbuf);
	crop_to_widget (uarea, &crop);

	area.x = 0;
	area.y = 0;
	area.width = width;
	area.height = crop.y;
	gdk_rectangle_intersect (&area, &event->area, &area);
	gdk_draw_pixbuf (window,
	                 style->fg_gc[state],
			 uarea->priv->color_shifted,
		 	 area.x, area.y,
		 	 area.x, area.y,
		 	 area.width, area.height,
		 	 GDK_RGB_DITHER_NONE, 0, 0);

	area.x = 0;
	area.y = crop.y;
	area.width = crop.x;
	area.height = crop.height;
	gdk_rectangle_intersect (&area, &event->area, &area);
	gdk_draw_pixbuf (window,
	                 style->fg_gc[state],
			 uarea->priv->color_shifted,
		 	 area.x, area.y,
		 	 area.x, area.y,
		 	 area.width, area.height,
		 	 GDK_RGB_DITHER_NONE, 0, 0);

	area.x = crop.x;
	area.y = crop.y;
	area.width = crop.width;
	area.height = crop.height;
	gdk_rectangle_intersect (&area, &event->area, &area);
	gdk_draw_pixbuf (window,
	                 style->fg_gc[state],
			 uarea->priv->pixbuf,
			 area.x, area.y,
			 area.x, area.y,
			 area.width, area.height,
			 GDK_RGB_DITHER_NONE, 0, 0);

	area.x = crop.x + crop.width;
	area.y = crop.y;
	area.width = width - area.x;
	area.height = crop.height;
	gdk_rectangle_intersect (&area, &event->area, &area);
	gdk_draw_pixbuf (window,
	                 style->fg_gc[state],
			 uarea->priv->color_shifted,
		 	 area.x, area.y,
		 	 area.x, area.y,
		 	 area.width, area.height,
		 	 GDK_RGB_DITHER_NONE, 0, 0);

	area.x = 0;
	area.y = crop.y + crop.width;
	area.width = width;
	area.height = height - area.y;
	gdk_rectangle_intersect (&area, &event->area, &area);
	gdk_draw_pixbuf (window,
	                 style->fg_gc[state],
			 uarea->priv->color_shifted,
		 	 area.x, area.y,
		 	 area.x, area.y,
		 	 area.width, area.height,
		 	 GDK_RGB_DITHER_NONE, 0, 0);

	cr = gdk_cairo_create (window);
	gdk_cairo_rectangle (cr, &event->area);
	cairo_clip (cr);

	if (uarea->priv->active_region != OUTSIDE) {
		gint x1, x2, y1, y2;
		gdk_cairo_set_source_color (cr, &style->white);
		cairo_set_line_width (cr, 1.0);
		x1 = crop.x + crop.width / 3.0;
		x2 = crop.x + 2 * crop.width / 3.0;
		y1 = crop.y + crop.height / 3.0;
		y2 = crop.y + 2 * crop.height / 3.0;

		cairo_move_to (cr, x1 + 0.5, crop.y);
		cairo_line_to (cr, x1 + 0.5, crop.y + crop.height);

		cairo_move_to (cr, x2 + 0.5, crop.y);
		cairo_line_to (cr, x2 + 0.5, crop.y + crop.height);

		cairo_move_to (cr, crop.x, y1 + 0.5);
		cairo_line_to (cr, crop.x + crop.width, y1 + 0.5);

		cairo_move_to (cr, crop.x, y2 + 0.5);
		cairo_line_to (cr, crop.x + crop.width, y2 + 0.5);
		cairo_stroke (cr);
	}

	gdk_cairo_set_source_color (cr, &style->black);
	cairo_set_line_width (cr, 1.0);
	cairo_rectangle (cr,
                         crop.x + 0.5,
                         crop.y + 0.5,
                         crop.width - 1.0,
                         crop.height - 1.0);
        cairo_stroke (cr);

	gdk_cairo_set_source_color (cr, &style->white);
	cairo_set_line_width (cr, 2.0);
	cairo_rectangle (cr,
                         crop.x + 2.0,
                         crop.y + 2.0,
                         crop.width - 4.0,
                         crop.height - 4.0);
        cairo_stroke (cr);

	cairo_destroy (cr);

	return FALSE;
}

typedef enum {
	BELOW,
	LOWER,
	BETWEEN,
	UPPER,
	ABOVE
} Range;

static Range
find_range (gint x,
            gint min,
            gint max)
{
	gint tolerance = 12;

	if (x < min - tolerance)
		return BELOW;
	if (x <= min + tolerance)
		return LOWER;
	if (x < max - tolerance)
		return BETWEEN;
	if (x <= max + tolerance)
		return UPPER;
	return ABOVE;
}

static Location
find_location (GdkRectangle *rect,
	       gint          x,
	       gint          y)
{
	Range x_range, y_range;
	Location location[5][5] = {
		{ OUTSIDE, OUTSIDE,     OUTSIDE, OUTSIDE,      OUTSIDE },
		{ OUTSIDE, TOP_LEFT,    TOP,     TOP_RIGHT,    OUTSIDE },
		{ OUTSIDE, LEFT,        INSIDE,  RIGHT,        OUTSIDE },
		{ OUTSIDE, BOTTOM_LEFT, BOTTOM,  BOTTOM_RIGHT, OUTSIDE },
		{ OUTSIDE, OUTSIDE,     OUTSIDE, OUTSIDE,      OUTSIDE }
	};

	x_range = find_range (x, rect->x, rect->x + rect->width);
	y_range = find_range (y, rect->y, rect->y + rect->height);

	return location[y_range][x_range];
}

static void
update_cursor (UmCropArea *area,
               gint           x,
               gint           y)
{
	gint cursor_type;
	GdkRectangle crop;

	crop_to_widget (area, &crop);

	switch (find_location (&crop, x, y)) {
	case OUTSIDE:
		cursor_type = GDK_LEFT_PTR;
		break;
	case TOP_LEFT:
		cursor_type = GDK_TOP_LEFT_CORNER;
		break;
	case TOP:
		cursor_type = GDK_TOP_SIDE;
		break;
	case TOP_RIGHT:
		cursor_type = GDK_TOP_RIGHT_CORNER;
		break;
	case LEFT:
		cursor_type = GDK_LEFT_SIDE;
		break;
	case INSIDE:
		cursor_type = GDK_FLEUR;
		break;
	case RIGHT:
		cursor_type = GDK_RIGHT_SIDE;
		break;
	case BOTTOM_LEFT:
		cursor_type = GDK_BOTTOM_LEFT_CORNER;
		break;
	case BOTTOM:
		cursor_type = GDK_BOTTOM_SIDE;
		break;
	case BOTTOM_RIGHT:
		cursor_type = GDK_BOTTOM_RIGHT_CORNER;
		break;
	}

	if (cursor_type != area->priv->current_cursor) {
		GdkCursor *cursor = gdk_cursor_new (cursor_type);
		gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (area)), cursor);
		gdk_cursor_unref (cursor);
		area->priv->current_cursor = cursor_type;
	}
}

static gboolean 
um_crop_area_motion_notify_event (GtkWidget      *widget,
				  GdkEventMotion *event)
{
	gint x, y;
	gint x2, y2;
	gint delta_x, delta_y;
	gint width, height, d;
	UmCropArea *area = UM_CROP_AREA (widget);

	width = gdk_pixbuf_get_width (area->priv->browse_pixbuf);
	height = gdk_pixbuf_get_height (area->priv->browse_pixbuf);

	x = (event->x - area->priv->image.x) / area->priv->scale;
	y = (event->y - area->priv->image.y) / area->priv->scale;
	x = CLAMP (x, 0, width);
	y = CLAMP (y, 0, height);

	delta_x = x - area->priv->last_press_x;
	delta_y = y - area->priv->last_press_y;
	area->priv->last_press_x = x;
	area->priv->last_press_y = y;

	x2 = area->priv->crop.x + area->priv->crop.width;
	y2 = area->priv->crop.y + area->priv->crop.height;

	switch (area->priv->active_region) {
 	case INSIDE:
		area->priv->crop.x = CLAMP (area->priv->crop.x + delta_x, 0, width - area->priv->crop.width);
		area->priv->crop.y = CLAMP (area->priv->crop.y + delta_y, 0, height - area->priv->crop.height);
		break;

	case TOP_LEFT:
		d = MAX (x2 - x, y2 - y);
		if (d < 48 / area->priv->scale)
			d = 48 / area->priv->scale;
		if (x2 - d < 0)
			d = x2;
		if (y2 - d < 0)
			d = y2;
		area->priv->crop.x = x2 - d;
		area->priv->crop.y = y2 - d;
		area->priv->crop.width = area->priv->crop.height = d;
		break;

	case TOP:
	case TOP_RIGHT:
		d = MAX (y2 - y, x - area->priv->crop.x);
		if (d < 48 / area->priv->scale)
			d = 48 / area->priv->scale;
		if (area->priv->crop.x + d > width)
			d = width - area->priv->crop.x;
		if (y2 - d < 0)
			d = y2;
		area->priv->crop.y = y2 - d;
		area->priv->crop.width = area->priv->crop.height = d;
		break;

	case LEFT:
	case BOTTOM_LEFT:
		d = MAX (x2 - x, y - area->priv->crop.y);
		if (d < 48 / area->priv->scale)
			d = 48 / area->priv->scale;
		if (area->priv->crop.y + d > height)
			d = height - area->priv->crop.y;
		if (x2 - d < 0)
			d = x2;
		area->priv->crop.x = x2 - d;
		area->priv->crop.width = area->priv->crop.height = d;
		break;

	case RIGHT:
	case BOTTOM_RIGHT:
	case BOTTOM:
		area->priv->crop.width = MAX (x - area->priv->crop.x, y - area->priv->crop.y);
		if (area->priv->crop.width < 48 / area->priv->scale)
			area->priv->crop.width = 48 / area->priv->scale;
		if (area->priv->crop.x + area->priv->crop.width > width)
			area->priv->crop.width = width - area->priv->crop.x;
		area->priv->crop.height = area->priv->crop.width;
		if (area->priv->crop.y + area->priv->crop.height > height)
			area->priv->crop.height = height - area->priv->crop.y;
		area->priv->crop.width = area->priv->crop.height;
		break;

	case OUTSIDE:
		break;
	default: ;
	}

	update_cursor (area, event->x, event->y);
	gtk_widget_queue_draw (widget);

	return FALSE;
}

static gboolean
um_crop_area_button_press_event (GtkWidget      *widget,
				 GdkEventButton *event)
{
	GdkRectangle crop;
	UmCropArea *area = UM_CROP_AREA (widget);

	crop_to_widget (area, &crop);

	area->priv->last_press_x = (event->x - area->priv->image.x) / area->priv->scale;
	area->priv->last_press_y = (event->y - area->priv->image.y) / area->priv->scale;
	area->priv->active_region = find_location (&crop, event->x, event->y);

	gtk_widget_queue_draw (widget);

	return FALSE;
}

static gboolean
um_crop_area_button_release_event (GtkWidget      *widget,
				   GdkEventButton *event)
{
	UmCropArea *area = UM_CROP_AREA (widget);

	area->priv->last_press_x = -1;
	area->priv->last_press_y = -1;
	area->priv->active_region = OUTSIDE;

	gtk_widget_queue_draw (widget);

	return FALSE;
}

static void
um_crop_area_finalize (GObject *object)
{
	UmCropArea *area = UM_CROP_AREA (object);

	if (area->priv->browse_pixbuf) {
		g_object_unref (area->priv->browse_pixbuf);
		area->priv->browse_pixbuf = NULL;
	}
	if (area->priv->pixbuf) {
		g_object_unref (area->priv->pixbuf);
		area->priv->pixbuf = NULL;
	}
	if (area->priv->color_shifted) {
		g_object_unref (area->priv->color_shifted);
		area->priv->color_shifted = NULL;
	}
}

static void
um_crop_area_class_init (UmCropAreaClass *klass)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	object_class->finalize     = um_crop_area_finalize;
	widget_class->expose_event = um_crop_area_expose;
	widget_class->button_press_event = um_crop_area_button_press_event;
	widget_class->button_release_event = um_crop_area_button_release_event;
	widget_class->motion_notify_event = um_crop_area_motion_notify_event;

	g_type_class_add_private (klass, sizeof (UmCropAreaPrivate));
}

static void
um_crop_area_init (UmCropArea *area)
{
	area->priv = (G_TYPE_INSTANCE_GET_PRIVATE ((area), UM_TYPE_CROP_AREA,
						   UmCropAreaPrivate));

	gtk_widget_add_events (GTK_WIDGET (area), GDK_POINTER_MOTION_MASK |
			       GDK_BUTTON_PRESS_MASK |
			       GDK_BUTTON_RELEASE_MASK);

	area->priv->scale = 0.0;
	area->priv->image.x = 0;
	area->priv->image.y = 0;
	area->priv->image.width = 0;
	area->priv->image.height = 0;
	area->priv->active_region = OUTSIDE;
}

GtkWidget *
um_crop_area_new (void)
{
	return g_object_new (UM_TYPE_CROP_AREA, NULL);
}

GdkPixbuf *
um_crop_area_get_picture (UmCropArea *area)
{
	return gdk_pixbuf_new_subpixbuf (area->priv->browse_pixbuf,
					 area->priv->crop.x, area->priv->crop.y,
					 area->priv->crop.width, area->priv->crop.height);
}

void
um_crop_area_set_picture (UmCropArea *area,
			  GdkPixbuf  *pixbuf)
{
	int width;
	int height;

	if (area->priv->browse_pixbuf) {
		g_object_unref (area->priv->browse_pixbuf);
		area->priv->browse_pixbuf = NULL;
	}
	if (pixbuf) {
		area->priv->browse_pixbuf = g_object_ref (pixbuf);
		width = gdk_pixbuf_get_width (pixbuf);
		height = gdk_pixbuf_get_height (pixbuf);
	} else {
		width = 0;
		height = 0;
	}

#if 0
	gtk_widget_get_allocation (um->browse_drawing_area, &allocation);
	um->priv->crop.width = 96;
	um->priv->crop.height = 96;
	um->priv->crop.x = (allocation.width - um->priv->crop.width) / 2;
	um->priv->crop.y = (allocation.height - um->priv->crop.height) / 2;
#else
	area->priv->crop.width = 96;
	area->priv->crop.height = 96;
	area->priv->crop.x = (width - area->priv->crop.width) / 2;
	area->priv->crop.y = (height - area->priv->crop.height) / 2;
#endif
	area->priv->scale = 0.0;
	area->priv->image.x = 0;
	area->priv->image.y = 0;
	area->priv->image.width = 0;
	area->priv->image.height = 0;

	gtk_widget_queue_draw (GTK_WIDGET (area));
}

