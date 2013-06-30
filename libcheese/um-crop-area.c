/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright 2009  Red Hat, Inc,
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Written by: Matthias Clasen <mclasen@redhat.com>
 */

#include "cheese-config.h"

#include <stdlib.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "um-crop-area.h"

/*
 * SECTION:um-crop-area
 * @short_description: A cropping widget for #CheeseAvatarChooser
 * @stability: Unstable
 *
 * #UmCropArea provides a simple cropping tool, used in #CheeseAvatarChooser to
 * crop an avatar from an image taken from a webcam.
 */

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
        gint base_width;
        gint base_height;
        gdouble aspect;
};

G_DEFINE_TYPE (UmCropArea, um_crop_area, GTK_TYPE_DRAWING_AREA);

/*
 * shift_color_byte:
 * @b: the color, as a single byte
 * @shift: the amount by which to shift the color
 *
 * Shift the supplied color @b by the amount @shift.
 *
 * Returns: the shifted color
 */
static inline guchar
shift_color_byte (guchar b,
                  gint   shift)
{
        return CLAMP(b + shift, 0, 255);
}

/*
 * shift_colors:
 * @pixbuf: a #GdkPixbuf
 * @red: amount to shift the red channel
 * @green: amount to shift the green channel
 * @blue: amount to shift the blue channel
 * @alpha: amount to shift the alpha channel
 *
 * Shift the color channels in the supplied @pixbuf by the amounts specified by
 * @red, @green, @blue and @alpha.
 */
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

/*
 * update_pixbufs:
 * @area: a #UmCropArea
 *
 * Update the #GdkPixbuf objects inside @area, by darkening the regions outside
 * the current crop area.
 */
static void
update_pixbufs (UmCropArea *area)
{
        gint width;
        gint height;
        GtkAllocation allocation;
        gdouble scale;
        GdkRGBA color;
        guint32 pixel;
        gint dest_x, dest_y, dest_width, dest_height;
        GtkWidget *widget;
        GtkStyleContext *context;

        widget = GTK_WIDGET (area);
        gtk_widget_get_allocation (widget, &allocation);
        context = gtk_widget_get_style_context (widget);

        if (area->priv->pixbuf == NULL ||
            gdk_pixbuf_get_width (area->priv->pixbuf) != allocation.width ||
            gdk_pixbuf_get_height (area->priv->pixbuf) != allocation.height) {
                if (area->priv->pixbuf != NULL)
                        g_object_unref (area->priv->pixbuf);
                area->priv->pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
                                                     gdk_pixbuf_get_has_alpha (area->priv->browse_pixbuf),
                                                     8,
                                                     allocation.width, allocation.height);

                gtk_style_context_get_background_color (context, gtk_style_context_get_state (context), &color);
                pixel = (((gint)(color.red * 1.0)) << 16) |
                        (((gint)(color.green * 1.0)) << 8) |
                         ((gint)(color.blue * 1.0));
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
                        area->priv->crop.width = 2 * area->priv->base_width / scale;
                        area->priv->crop.height = 2 * area->priv->base_height / scale;
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

/*
 * crop_widget:
 * @area: a #UmCropArea
 * @crop: (out caller-allocates): a return location for a #GdkRectangle
 *
 * Update the supplied @crop rectangle to the current crop area.
 */
static void
crop_to_widget (UmCropArea    *area,
                GdkRectangle  *crop)
{
        crop->x = area->priv->image.x + area->priv->crop.x * area->priv->scale;
        crop->y = area->priv->image.y + area->priv->crop.y * area->priv->scale;
        crop->width = area->priv->crop.width * area->priv->scale;
        crop->height = area->priv->crop.height * area->priv->scale;
}

/*
 * Location:
 * @OUTSIDE: outside the area
 * @INSIDE: inside the area and not on a border
 * @TOP: on the top border
 * @TOP_LEFT: on the top-left corner
 * @TOP_RIGHT: on the top-right corner
 * @BOTTOM: on the bottom border
 * @BOTTOM_LEFT: on the bottom-left corner
 * @BOTTOM_RIGHT: on the bottom-right corner
 * @LEFT: on the left border
 * @RIGHT: on the right border
 *
 * The location of a point, relative to a rectangular area.
 */
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

/*
 * um_crop_area_draw:
 * @widget: a #UmCropArea
 * @cr: the #cairo_t to draw to
 *
 * Handle the #GtkWidget::draw signal and draw the @widget.
 *
 * Returns: %FALSE
 */
static gboolean
um_crop_area_draw (GtkWidget *widget,
                   cairo_t   *cr)
{
        GdkRectangle crop;
        gint width, height;
        UmCropArea *uarea = UM_CROP_AREA (widget);

        if (uarea->priv->browse_pixbuf == NULL)
                return FALSE;

        update_pixbufs (uarea);

        width = gdk_pixbuf_get_width (uarea->priv->pixbuf);
        height = gdk_pixbuf_get_height (uarea->priv->pixbuf);
        crop_to_widget (uarea, &crop);

        gdk_cairo_set_source_pixbuf (cr, uarea->priv->color_shifted, 0, 0);
        cairo_rectangle (cr, 0, 0, width, crop.y);
        cairo_rectangle (cr, 0, crop.y, crop.x, crop.height);
        cairo_rectangle (cr, crop.x + crop.width, crop.y, width - crop.x - crop.width, crop.height);
        cairo_rectangle (cr, 0, crop.y + crop.height, width, height - crop.y - crop.height);
        cairo_fill (cr);

        gdk_cairo_set_source_pixbuf (cr, uarea->priv->pixbuf, 0, 0);
        cairo_rectangle (cr, crop.x, crop.y, crop.width, crop.height);
        cairo_fill (cr);

        if (uarea->priv->active_region != OUTSIDE) {
                gint x1, x2, y1, y2;
                cairo_set_source_rgb (cr, 1, 1, 1);
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

        cairo_set_source_rgb (cr,  0, 0, 0);
        cairo_set_line_width (cr, 1.0);
        cairo_rectangle (cr,
                         crop.x + 0.5,
                         crop.y + 0.5,
                         crop.width - 1.0,
                         crop.height - 1.0);
        cairo_stroke (cr);

        cairo_set_source_rgb (cr, 1, 1, 1);
        cairo_set_line_width (cr, 2.0);
        cairo_rectangle (cr,
                         crop.x + 2.0,
                         crop.y + 2.0,
                         crop.width - 4.0,
                         crop.height - 4.0);
        cairo_stroke (cr);

        return FALSE;
}

/*
 * Range:
 * @BELOW: less than the minimum
 * @LOWER: less than or equal to the minimum
 * @BETWEEN: between the minimum and the maximum
 * @UPPER: greater than or equal to the maximum
 * @ABOVE: greater than the maximum
 *
 * Indicates where a number lies with respect to a minimum and maximum value.
 * The apparent overlap is avoided by means of a threshold value.
 *
 * @see_also: find_range()
 */
typedef enum {
        BELOW,
        LOWER,
        BETWEEN,
        UPPER,
        ABOVE
} Range;

/*
 * find_range:
 * @x: number to test
 * @min: minimum to test against
 * @max: maximum to test against
 *
 * Determine where @x lies in relation to @min and @max. A threshold is also
 * used internally, giving fives possible states.
 *
 * Returns: the #Range
 *
 * @see_also: #Range
 */
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

/*
 * find_location:
 * @rect: a #GdkRectangle
 * @x: x coordinate
 * @y: y coordinate
 *
 * Find the #Location of the specified @x and @y coordinates, relative to the
 * crop area given by @rect.
 *
 * Returns: the #Location
 */
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

/*
 * update_cursor:
 * @area: a #UmCropArea
 * @x: x coordinate
 * @y: y coordinate
 *
 * Update the type of the cursor, depending on which point of the crop
 * rectangle the pointer is over.
 */
static void
update_cursor (UmCropArea *area,
               gint           x,
               gint           y)
{
        gint cursor_type;
        GdkRectangle crop;
        gint region;

        region = area->priv->active_region;
        if (region == OUTSIDE) {
                crop_to_widget (area, &crop);
                region = find_location (&crop, x, y);
        }

        switch (region) {
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
	default:
		g_assert_not_reached ();
        }

        if (cursor_type != area->priv->current_cursor) {
                GdkCursor *cursor = gdk_cursor_new (cursor_type);
                gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (area)), cursor);
                g_object_unref (cursor);
                area->priv->current_cursor = cursor_type;
        }
}

/*
 * eval_radial_line:
 * @center_x: starting x coordinate
 * @center_y: starting y coordinate
 * @bounds_x: final x coordinate
 * @bounds_y: final y coordinate
 * @user_x: starting x coordinate for the returned y coordinate
 *
 * Calculate the value of y of the line from @center_x, @center_y to @bounds_x,
 * @bounds_y, given the x value @user_x.
 *
 * Returns: the value of y
 *
 * @see_also: um_crop_area_motion_notify_event()
 */
static gint
eval_radial_line (gdouble center_x, gdouble center_y,
                  gdouble bounds_x, gdouble bounds_y,
                  gdouble user_x)
{
        gdouble decision_slope;
        gdouble decision_intercept;

        decision_slope = (bounds_y - center_y) / (bounds_x - center_x);
        decision_intercept = -(decision_slope * bounds_x);

        return (gint) (decision_slope * user_x + decision_intercept);
}

/*
 * um_crop_area_motion_notify_event:
 * @widget: a #UmCropArea
 * @event: the #GdkEventMotion
 *
 * Update the cropped region, and redraw it, based on the current cursor
 * position.
 *
 * Returns: %FALSE
 *
 * @see_also: eval_radial_line()
 */
static gboolean
um_crop_area_motion_notify_event (GtkWidget      *widget,
                                  GdkEventMotion *event)
{
        UmCropArea *area = UM_CROP_AREA (widget);
        gint x, y;
        gint delta_x, delta_y;
        gint width, height;
        gint adj_width, adj_height;
        gint pb_width, pb_height;
        GdkRectangle damage;
        gint left, right, top, bottom;
        gdouble new_width, new_height;
        gdouble center_x, center_y;
        gint min_width, min_height;

        if (area->priv->browse_pixbuf == NULL)
                return FALSE;

        update_cursor (area, event->x, event->y);

        crop_to_widget (area, &damage);
        gtk_widget_queue_draw_area (widget,
                                    damage.x - 1, damage.y - 1,
                                    damage.width + 2, damage.height + 2);

        pb_width = gdk_pixbuf_get_width (area->priv->browse_pixbuf);
        pb_height = gdk_pixbuf_get_height (area->priv->browse_pixbuf);

        x = (event->x - area->priv->image.x) / area->priv->scale;
        y = (event->y - area->priv->image.y) / area->priv->scale;

        delta_x = x - area->priv->last_press_x;
        delta_y = y - area->priv->last_press_y;
        area->priv->last_press_x = x;
        area->priv->last_press_y = y;

        left = area->priv->crop.x;
        right = area->priv->crop.x + area->priv->crop.width - 1;
        top = area->priv->crop.y;
        bottom = area->priv->crop.y + area->priv->crop.height - 1;

        center_x = (left + right) / 2.0;
        center_y = (top + bottom) / 2.0;

        switch (area->priv->active_region) {
        case INSIDE:
                width = right - left + 1;
                height = bottom - top + 1;

                left += delta_x;
                right += delta_x;
                top += delta_y;
                bottom += delta_y;

                if (left < 0)
                        left = 0;
                if (top < 0)
                        top = 0;
                if (right > pb_width)
                        right = pb_width;
                if (bottom > pb_height)
                        bottom = pb_height;

                adj_width = right - left + 1;
                adj_height = bottom - top + 1;
                if (adj_width != width) {
                        if (delta_x < 0)
                                right = left + width - 1;
                        else
                                left = right - width + 1;
                }
                if (adj_height != height) {
                        if (delta_y < 0)
                                bottom = top + height - 1;
                        else
                                top = bottom - height + 1;
                }

                break;

        case TOP_LEFT:
                if (area->priv->aspect < 0) {
                        top = y;
                        left = x;
                }
                else if (y < eval_radial_line (center_x, center_y, left, top, x)) {
                        top = y;
                        new_width = (bottom - top) * area->priv->aspect;
                        left = right - new_width;
                }
                else {
                        left = x;
                        new_height = (right - left) / area->priv->aspect;
                        top = bottom - new_height;
                }
                break;

        case TOP:
                top = y;
                if (area->priv->aspect > 0) {
                        new_width = (bottom - top) * area->priv->aspect;
                        right = left + new_width;
                }
                break;

        case TOP_RIGHT:
                if (area->priv->aspect < 0) {
                        top = y;
                        right = x;
                }
                else if (y < eval_radial_line (center_x, center_y, right, top, x)) {
                        top = y;
                        new_width = (bottom - top) * area->priv->aspect;
                        right = left + new_width;
                }
                else {
                        right = x;
                        new_height = (right - left) / area->priv->aspect;
                        top = bottom - new_height;
                }
                break;

        case LEFT:
                left = x;
                if (area->priv->aspect > 0) {
                        new_height = (right - left) / area->priv->aspect;
                        bottom = top + new_height;
                }
                break;

        case BOTTOM_LEFT:
                if (area->priv->aspect < 0) {
                        bottom = y;
                        left = x;
                }
                else if (y < eval_radial_line (center_x, center_y, left, bottom, x)) {
                        left = x;
                        new_height = (right - left) / area->priv->aspect;
                        bottom = top + new_height;
                }
                else {
                        bottom = y;
                        new_width = (bottom - top) * area->priv->aspect;
                        left = right - new_width;
                }
                break;

        case RIGHT:
                right = x;
                if (area->priv->aspect > 0) {
                        new_height = (right - left) / area->priv->aspect;
                        bottom = top + new_height;
                }
                break;

        case BOTTOM_RIGHT:
                if (area->priv->aspect < 0) {
                        bottom = y;
                        right = x;
                }
                else if (y < eval_radial_line (center_x, center_y, right, bottom, x)) {
                        right = x;
                        new_height = (right - left) / area->priv->aspect;
                        bottom = top + new_height;
                }
                else {
                        bottom = y;
                        new_width = (bottom - top) * area->priv->aspect;
                        right = left + new_width;
                }
                break;

        case BOTTOM:
                bottom = y;
                if (area->priv->aspect > 0) {
                        new_width = (bottom - top) * area->priv->aspect;
                        right= left + new_width;
                }
                break;

        default:
                return FALSE;
        }

        min_width = area->priv->base_width / area->priv->scale;
        min_height = area->priv->base_height / area->priv->scale;

        width = right - left + 1;
        height = bottom - top + 1;
        if (area->priv->aspect < 0) {
                if (left < 0)
                        left = 0;
                if (top < 0)
                        top = 0;
                if (right > pb_width)
                        right = pb_width;
                if (bottom > pb_height)
                        bottom = pb_height;

                width = right - left + 1;
                height = bottom - top + 1;

                switch (area->priv->active_region) {
                case LEFT:
                case TOP_LEFT:
                case BOTTOM_LEFT:
                        if (width < min_width)
                                left = right - min_width;
                        break;
                case RIGHT:
                case TOP_RIGHT:
                case BOTTOM_RIGHT:
                        if (width < min_width)
                                right = left + min_width;
                        break;

                default: ;
                }

                switch (area->priv->active_region) {
                case TOP:
                case TOP_LEFT:
                case TOP_RIGHT:
                        if (height < min_height)
                                top = bottom - min_height;
                        break;
                case BOTTOM:
                case BOTTOM_LEFT:
                case BOTTOM_RIGHT:
                        if (height < min_height)
                                bottom = top + min_height;
                        break;

                default: ;
                }
        }
        else {
                if (left < 0 || top < 0 ||
                    right > pb_width || bottom > pb_height ||
                    width < min_width || height < min_height) {
                        left = area->priv->crop.x;
                        right = area->priv->crop.x + area->priv->crop.width - 1;
                        top = area->priv->crop.y;
                        bottom = area->priv->crop.y + area->priv->crop.height - 1;
                }
        }

        area->priv->crop.x = left;
        area->priv->crop.y = top;
        area->priv->crop.width = right - left + 1;
        area->priv->crop.height = bottom - top + 1;

        crop_to_widget (area, &damage);
        gtk_widget_queue_draw_area (widget,
                                    damage.x - 1, damage.y - 1,
                                    damage.width + 2, damage.height + 2);

        return FALSE;
}

/*
 * um_crop_area_button_press_event:
 * @widget: a #UmCropArea
 * @event: a #GdkEventButton
 *
 * Handle the mouse button being pressed on the widget, by initiating a crop
 * region selection and redrawing the cropped area.
 *
 * Returns: %FALSE
 */
static gboolean
um_crop_area_button_press_event (GtkWidget      *widget,
                                 GdkEventButton *event)
{
        UmCropArea *area = UM_CROP_AREA (widget);
        GdkRectangle crop;

        if (area->priv->browse_pixbuf == NULL)
                return FALSE;

        crop_to_widget (area, &crop);

        area->priv->last_press_x = (event->x - area->priv->image.x) / area->priv->scale;
        area->priv->last_press_y = (event->y - area->priv->image.y) / area->priv->scale;
        area->priv->active_region = find_location (&crop, event->x, event->y);

        gtk_widget_queue_draw_area (widget,
                                    crop.x - 1, crop.y - 1,
                                    crop.width + 2, crop.height + 2);

        return FALSE;
}

/*
 * um_crop_area_button_release_event:
 * @widget: a #UmCropArea
 * @event: a #GdkEventButton
 *
 * Handle the mouse button being released on the widget, by redrawing the
 * cropped region.
 *
 * Returns: %FALSE
 */
static gboolean
um_crop_area_button_release_event (GtkWidget      *widget,
                                   GdkEventButton *event)
{
        UmCropArea *area = UM_CROP_AREA (widget);
        GdkRectangle crop;

        if (area->priv->browse_pixbuf == NULL)
                return FALSE;

        crop_to_widget (area, &crop);

        area->priv->last_press_x = -1;
        area->priv->last_press_y = -1;
        area->priv->active_region = OUTSIDE;

        gtk_widget_queue_draw_area (widget,
                                    crop.x - 1, crop.y - 1,
                                    crop.width + 2, crop.height + 2);

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

        object_class->finalize = um_crop_area_finalize;
        widget_class->draw = um_crop_area_draw;
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
        area->priv->base_width = 48;
        area->priv->base_height = 48;
        area->priv->aspect = 1;
}

/*
 * um_crop_area_new:
 *
 * Creates a new #UmCropArea widget.
 *
 * Returns: a #UmCropArea
 */
GtkWidget *
um_crop_area_new (void)
{
        return g_object_new (UM_TYPE_CROP_AREA, NULL);
}

/*
 * um_crop_area_get_picture:
 * @area: a #UmCropArea
 *
 * Returns the cropped image, or the whole image if no crop region has been
 * set, as a #GdkPixbuf.
 *
 * Returns: a #GdkPixbuf
 */
GdkPixbuf *
um_crop_area_get_picture (UmCropArea *area)
{
        gint width, height;

        width = gdk_pixbuf_get_width (area->priv->browse_pixbuf);
        height = gdk_pixbuf_get_height (area->priv->browse_pixbuf);
        width = MIN (area->priv->crop.width, width - area->priv->crop.x);
        height = MIN (area->priv->crop.height, height - area->priv->crop.y);

        return gdk_pixbuf_new_subpixbuf (area->priv->browse_pixbuf,
                                         area->priv->crop.x,
                                         area->priv->crop.y,
                                         width, height);
}

/*
 * um_crop_area_set_picture:
 * @area: a #UmCropArea
 * @pixbuf: (allow-none): the #GdkPixbuf to set, or %NULL to clear the current
 * picture
 *
 * Set the image to be used inside the @area to @pixbuf.
 */
void
um_crop_area_set_picture (UmCropArea *area,
                          GdkPixbuf  *pixbuf)
{
        gint width;
        gint height;

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

        area->priv->crop.width = 2 * area->priv->base_width;
        area->priv->crop.height = 2 * area->priv->base_height;
        area->priv->crop.x = (width - area->priv->crop.width) / 2;
        area->priv->crop.y = (height - area->priv->crop.height) / 2;

        area->priv->scale = 0.0;
        area->priv->image.x = 0;
        area->priv->image.y = 0;
        area->priv->image.width = 0;
        area->priv->image.height = 0;

        gtk_widget_queue_draw (GTK_WIDGET (area));
}

/*
 * um_crop_area_set_min_size:
 * @area: a #UmCropArea
 * @width: a minimum width, in pixels
 * @height: a minimum height, in pixels
 *
 * Sets the minimum size that the cropped image will be allowed to have.
 */
void
um_crop_area_set_min_size (UmCropArea *area,
                           gint        width,
                           gint        height)
{
        area->priv->base_width = width;
        area->priv->base_height = height;

        if (area->priv->aspect > 0) {
                area->priv->aspect = area->priv->base_width / (gdouble)area->priv->base_height;
        }
}

/*
 * um_crop_area_set_constrain_aspect:
 * @area: a #UmCropArea
 * @constrain: whether to constrain the aspect ratio of the cropped image
 *
 * Controls whether the aspect ratio of the cropped area of the image should be
 * constrained.
 */
void
um_crop_area_set_constrain_aspect (UmCropArea *area,
                                   gboolean    constrain)
{
        if (constrain) {
                area->priv->aspect = area->priv->base_width / (gdouble)area->priv->base_height;
        }
        else {
                area->priv->aspect = -1;
        }
}

