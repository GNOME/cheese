/* Stolen from Eye Of Gnome
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Taken from gnome-utils/gnome-screensaver/screensaver-shadow.c
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
 */

/* Shadow code from anders */

/* Modified and updated by Claudio Saavedra <csaavedra@alumnos.utalca.cl> */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "eog-thumb-shadow.h"
#include <math.h>

#define BLUR_RADIUS    5
#define SHADOW_OFFSET  (BLUR_RADIUS * 4 / 5)
#define SHADOW_OPACITY 0.7

#define OUTLINE_RADIUS  4.0
#define OUTLINE_OFFSET  0.0
#define OUTLINE_OPACITY 1.0

#define RECTANGLE_OUTLINE 1
#define RECTANGLE_OPACITY 1.0

#define ROUND_BORDER 1.0

#define dist(x0, y0, x1, y1) sqrt(((x0) - (x1))*((x0) - (x1)) + ((y0) - (y1))*((y0) - (y1)))

typedef struct
{
  int size;
  double *data;
} ConvFilter;

  static double
gaussian (double x, double y, double r)
{
  return ((1 / (2 * M_PI * r)) * exp ((-(x * x + y * y)) / (2 * r * r)));
}

static ConvFilter *
create_blur_filter (int radius)
{
  ConvFilter *filter;
  int x, y;
  double sum;

  filter = g_new0 (ConvFilter, 1);
  filter->size = radius * 2 + 1;
  filter->data = g_new (double, filter->size * filter->size);

  sum = 0.0;

  for (y = 0; y < filter->size; y++)
  {
    for (x = 0; x < filter->size; x++)
    {
      sum += filter->data[y * filter->size + x] =
        gaussian (x - (filter->size >> 1), y - (filter->size >> 1),
            radius);
    }
  }

  for (y = 0; y < filter->size; y++)
  {
    for (x = 0; x < filter->size; x++)
    {
      filter->data[y * filter->size + x] /= sum;
    }
  }

  return filter;

}

static ConvFilter *
create_outline_filter (int radius)
{
  ConvFilter *filter;
  int x, y;

  filter = g_new0 (ConvFilter, 1);
  filter->size = radius * 2 + 1;
  filter->data = g_new (double, filter->size * filter->size);

  for (y = 0; y < filter->size; y++)
  {
    for (x = 0; x < filter->size; x++)
    {
      filter->data[y * filter->size + x] =
        (dist (x, y, radius, radius) < radius + 0.2) ? 1.0 : 0.0;
    }
  }

  return filter;
}

static GdkPixbuf *
create_effect (GdkPixbuf *src, ConvFilter const *filter, int radius,
               int offset, double opacity)
{
  GdkPixbuf *dest;
  int x, y, i, j;
  int src_x, src_y;
  int suma;
  int dest_width, dest_height;
  int src_width, src_height;
  int src_rowstride, dest_rowstride;
  gboolean src_has_alpha;

  guchar *src_pixels, *dest_pixels;

  src_has_alpha = gdk_pixbuf_get_has_alpha (src);

  src_width = gdk_pixbuf_get_width (src);
  src_height = gdk_pixbuf_get_height (src);
  dest_width = src_width + 2 * radius + offset;
  dest_height = src_height + 2 * radius + offset;

  dest = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (src),
      TRUE,
      gdk_pixbuf_get_bits_per_sample (src),
      dest_width, dest_height);

  gdk_pixbuf_fill (dest, 0);

  src_pixels = gdk_pixbuf_get_pixels (src);
  src_rowstride = gdk_pixbuf_get_rowstride (src);

  dest_pixels = gdk_pixbuf_get_pixels (dest);
  dest_rowstride = gdk_pixbuf_get_rowstride (dest);

  for (y = 0; y < dest_height; y++)
  {
    for (x = 0; x < dest_width; x++)
    {
      suma = 0;

      src_x = x - radius;
      src_y = y - radius;

      /* We don't need to compute effect here, since this pixel will be 
       * discarded when compositing */
      if (src_x >= 0 && src_x < src_width &&
          src_y >= 0 && src_y < src_height &&
          (!src_has_alpha ||
           src_pixels[src_y * src_rowstride + src_x * 4 + 3] == 0xFF))
        continue;

      for (i = 0; i < filter->size; i++)
      {
        for (j = 0; j < filter->size; j++)
        {
          src_y = -(radius + offset) + y - (filter->size >> 1) + i;
          src_x = -(radius + offset) + x - (filter->size >> 1) + j;

          if (src_y < 0 || src_y >= src_height ||
              src_x < 0 || src_x >= src_width)
            continue;

          suma += (src_has_alpha ?
              src_pixels[src_y * src_rowstride + src_x * 4 + 3] :
              0xFF) * filter->data[i * filter->size + j];
        }
      }

      dest_pixels[y * dest_rowstride + x * 4 + 3] =
        CLAMP (suma * opacity, 0x00, 0xFF);
    }
  }

  return dest;
}

void
eog_thumb_shadow_add_shadow (GdkPixbuf **src)
{
  GdkPixbuf *dest;
  static ConvFilter *filter = NULL;

  if (!filter)
    filter = create_blur_filter (BLUR_RADIUS);

  dest = create_effect (*src, filter,
      BLUR_RADIUS, SHADOW_OFFSET, SHADOW_OPACITY);

  if (dest == NULL)
    return;

  gdk_pixbuf_composite (*src, dest,
      BLUR_RADIUS, BLUR_RADIUS,
      gdk_pixbuf_get_width (*src),
      gdk_pixbuf_get_height (*src),
      BLUR_RADIUS, BLUR_RADIUS, 1.0, 1.0,
      GDK_INTERP_NEAREST, 255);
  g_object_unref (*src);
  *src = dest;
}

void
eog_thumb_shadow_add_border (GdkPixbuf **src)
{
  GdkPixbuf *dest;
  static ConvFilter *filter = NULL;

  if (!filter)
    filter = create_outline_filter (OUTLINE_RADIUS);

  dest = create_effect (*src, filter,
      OUTLINE_RADIUS, OUTLINE_OFFSET, OUTLINE_OPACITY);

  if (dest == NULL)
    return;

  gdk_pixbuf_composite (*src, dest,
      OUTLINE_RADIUS, OUTLINE_RADIUS,
      gdk_pixbuf_get_width (*src),
      gdk_pixbuf_get_height (*src),
      OUTLINE_RADIUS, OUTLINE_RADIUS, 1.0, 1.0,
      GDK_INTERP_NEAREST, 255);
  g_object_unref (*src);
  *src = dest;
}

void
eog_thumb_shadow_add_rectangle (GdkPixbuf **src)
{
  GdkPixbuf *dest;
  int width, height, rowstride;
  int dest_width, dest_height;
  int x, y;
  guchar *pixels, *p;
  guint a;

  width = gdk_pixbuf_get_width (*src);
  height = gdk_pixbuf_get_height (*src);

  dest = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
      TRUE, 8,
      width + 2 * RECTANGLE_OUTLINE,
      height + 2 * RECTANGLE_OUTLINE);

  rowstride = gdk_pixbuf_get_rowstride (dest);
  pixels = gdk_pixbuf_get_pixels (dest);

  dest_width = width + 2 * RECTANGLE_OUTLINE;
  dest_height = height + 2 * RECTANGLE_OUTLINE;

  a = RECTANGLE_OPACITY * 0xFF;

  /* draw horizontal lines */
  for (x = 0; x < dest_width; x++)
  {
    for (y = 0; y < RECTANGLE_OUTLINE; y++)
    {
      p = pixels + y * rowstride + 4 * x;
      p[0] = p[1] = p[2] = 0x00;
      p[3] = a;

      p = pixels + (y + height + RECTANGLE_OUTLINE) * rowstride + 4 * x;
      p[0] = p[1] = p[2] = 0x00;
      p[3] = a;
    }
  }

  /* draw vertical lines */
  for (y = 0; y < dest_height; y++)
  {
    for (x = 0; x < RECTANGLE_OUTLINE; x++)
    {
      p = pixels + y * rowstride + 4 * x;
      p[0] = p[1] = p[2] = 0x00;
      p[3] = a;

      p = pixels + y * rowstride + 4 * (x + width + RECTANGLE_OUTLINE);
      p[0] = p[1] = p[2] = 0x00;
      p[3] = a;
    }
  }

  gdk_pixbuf_copy_area (*src,
      0, 0, width, height,
      dest, RECTANGLE_OUTLINE, RECTANGLE_OUTLINE);

  g_object_unref (*src);
  *src = dest;
}

/* This could be optimized such that the pixbuf is created only once during the EOG 
   lifetime*/

static GdkPixbuf *
create_round_border (int radius)
{
  int x, y;
  GdkPixbuf *dest;
  guchar *dest_pixels;
  int dest_rowstride;
  int value;

  dest = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 2 * radius, 2 * radius);

  dest_pixels = gdk_pixbuf_get_pixels (dest);
  dest_rowstride = gdk_pixbuf_get_rowstride (dest);

  for (y = 0; y < 2 * radius; y++)
  {
    for (x = 0; x < 2 * radius; x++)
    {
      value = (dist (x, y, radius, radius) < radius) ? 0xFF : 0x00;
      dest_pixels[x * dest_rowstride + y * 4] = value;  /* R */
      dest_pixels[x * dest_rowstride + y * 4 + 1] = value;  /* G */
      dest_pixels[x * dest_rowstride + y * 4 + 2] = value;  /* B */
      dest_pixels[x * dest_rowstride + y * 4 + 3] = value;  /* A */
    }
  }
  return dest;
}

void
eog_thumb_shadow_add_round_border (GdkPixbuf **src)
{
  GdkPixbuf *dest;
  int width, height;
  GdkPixbuf *border;

  width = gdk_pixbuf_get_width (*src);
  height = gdk_pixbuf_get_height (*src);

  dest = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
      TRUE, 8,
      width + 2 * ROUND_BORDER, height + 2 * ROUND_BORDER);

  gdk_pixbuf_fill (dest, 0xFFFFFFFF);

  border = create_round_border (ROUND_BORDER);

  /* upper left */
  gdk_pixbuf_copy_area (border, 0, 0, ROUND_BORDER, ROUND_BORDER, dest, 0, 0);
  /* down left */
  gdk_pixbuf_copy_area (border,
      0, ROUND_BORDER, ROUND_BORDER, ROUND_BORDER,
      dest, 0, height + ROUND_BORDER);
  /* upper right */
  gdk_pixbuf_copy_area (border,
      ROUND_BORDER, 0, ROUND_BORDER, ROUND_BORDER,
      dest, width + ROUND_BORDER, 0);
  /* down right */
  gdk_pixbuf_copy_area (border,
      ROUND_BORDER, ROUND_BORDER, ROUND_BORDER,
      ROUND_BORDER, dest, width + ROUND_BORDER,
      height + ROUND_BORDER);

  gdk_pixbuf_copy_area (*src,
      0, 0, width, height,
      dest, ROUND_BORDER, ROUND_BORDER);

  g_object_unref (*src);
  g_object_unref (border);
  *src = dest;
}

void
eog_thumb_shadow_add_frame (GdkPixbuf **src)
{
  int width, height, f_width;
  width = gdk_pixbuf_get_width (*src);
  height = gdk_pixbuf_get_height (*src);

  f_width = MAX (width, height);

  GdkPixbuf *dest = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
      TRUE, 8,
      f_width, f_width);

  gdk_pixbuf_fill (dest, 0x0);

  gdk_pixbuf_copy_area (*src,
      0, 0, width, height,
      dest,
      (width > height) ? 0 : (f_width - width) / 2,
      (width > height) ? (f_width - height) / 2 : 0);

  g_object_unref (*src);
  *src = dest;
}
