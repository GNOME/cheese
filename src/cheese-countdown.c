/*
 * Copyright (C) 2008 Mirco "MacSlow" Müller <macslow@bangang.de>
 * Copyright (C) 2008 daniel g. siegel <dgsiegel@gmail.com>
 * Copyright (C) 2008 Patryk Zawadzki <patrys@pld-linux.org>
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

#ifdef HAVE_CONFIG_H
#include <cheese-config.h>
#endif

#include <stdlib.h>
#include <gtk/gtk.h>
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>

#include "cheese-countdown.h"

#define R 0
#define G 1
#define B 2
#define A 3

#define CHEESE_COUNTDOWN_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHEESE_TYPE_COUNTDOWN, CheeseCountdownPrivate))

G_DEFINE_TYPE (CheeseCountdown, cheese_countdown, GTK_TYPE_DRAWING_AREA);

#define STATE_OFF   0
#define STATE_3     1
#define STATE_2     2
#define STATE_1     3
#define STATE_SMILE 4

typedef struct
{
  gint iState;
  cairo_surface_t* pSurface;
  cheese_countdown_cb_t picture_callback;
  cheese_countdown_cb_t hide_callback;
  gpointer callback_data;
  gdouble bg[3];
  gdouble bg_light[3];
  gdouble text[3];
} CheeseCountdownPrivate;

/* copied from gtk/gtkhsv.c, released under GPL v2 */
/* Converts from HSV to RGB */
static void
hsv_to_rgb (gdouble *h,
            gdouble *s,
            gdouble *v)
{
  gdouble hue, saturation, value;
  gdouble f, p, q, t;

  if (*s <= 0.0)
  {
    *h = *v;
    *s = *v;
    *v = *v; /* heh */
  }
  else
  {
    hue = *h * 6.0;
    saturation = *s;
    value = *v;

    if (hue >= 6.0)
      hue = 0.0;

    f = hue - (int) hue;
    p = value * (1.0 - saturation);
    q = value * (1.0 - saturation * f);
    t = value * (1.0 - saturation * (1.0 - f));

    switch ((int) hue)
    {
      case 0:
        *h = value;
        *s = t;
        *v = p;
        break;

      case 1:
        *h = q;
        *s = value;
        *v = p;
        break;

      case 2:
        *h = p;
        *s = value;
        *v = t;
        break;

      case 3:
        *h = p;
        *s = q;
        *v = value;
        break;

      case 4:
        *h = t;
        *s = p;
        *v = value;
        break;

      case 5:
        *h = value;
        *s = p;
        *v = q;
        break;

      default:
        g_assert_not_reached ();
    }
  }
}

/* Converts from RGB to HSV */
static void
rgb_to_hsv (gdouble *r,
            gdouble *g,
            gdouble *b)
{
  gdouble red, green, blue;
  gdouble h, s, v;
  gdouble min, max;
  gdouble delta;

  red = *r;
  green = *g;
  blue = *b;

  h = 0.0;

  if (red > green)
  {
    if (red > blue)
      max = red;
    else
      max = blue;

    if (green < blue)
      min = green;
    else
      min = blue;
  }
  else
  {
    if (green > blue)
      max = green;
    else
      max = blue;

    if (red < blue)
      min = red;
    else
      min = blue;
  }

  v = max;

  if (max != 0.0)
    s = (max - min) / max;
  else
    s = 0.0;

  if (s <= 0.0)
    h = 0.0;
  else
  {
    delta = max - min;

    if (red == max)
      h = (green - blue) / delta;
    else if (green == max)
      h = 2 + (blue - red) / delta;
    else if (blue == max)
      h = 4 + (red - green) / delta;

    h /= 6.0;

    if (h < 0.0)
      h += 1.0;
    else if (h > 1.0)
      h -= 1.0;
  }

  *r = h;
  *g = s;
  *b = v;
}

static gint
do_text (cairo_t*    pContext,
         gchar*      pcText,
         gint        iFontSize,
         gchar*      pcFontFamily,
         PangoWeight fontWeight,
         PangoStyle  fontStyle)
{
  PangoFontDescription* pDesc       = NULL;
  PangoLayout*          pLayout     = NULL;
  GString*              pTextString = NULL;
  PangoRectangle        logicalRect;
  gint                  iAdvanceWidth;

  /* setup a new pango-layout based on the source-context */
  pLayout = pango_cairo_create_layout (pContext);
  if (!pLayout)
  {
    g_print ("do_text(): ");
    g_print ("Could not create pango-layout!\n");
    return 0;
  }

  /* get a new pango-description */
  pDesc = pango_font_description_new ();
  if (!pDesc)
  {
    g_print ("do_text(): ");
    g_print ("Could not create pango-font-description!\n");
    g_object_unref (pLayout);
    return 0;
  }

  /* set some desired font-attributes */
  pango_font_description_set_absolute_size (pDesc, iFontSize);
  pango_font_description_set_family_static (pDesc, pcFontFamily);
  pango_font_description_set_weight (pDesc, fontWeight);
  pango_font_description_set_style (pDesc, fontStyle);
  pango_layout_set_font_description (pLayout, pDesc);
  pango_font_description_free (pDesc);

  /* get buffer to hold string */
  pTextString = g_string_new (NULL);
  if (!pTextString)
  {
    g_print ("do_text(): ");
    g_print ("Could not create string-buffer!\n");
    g_object_unref (pLayout);
    return 0;
  }

  /* print and layout string (pango-wise) */
  g_string_printf (pTextString, pcText);
  pango_layout_set_text (pLayout, pTextString->str, -1);
  g_string_free (pTextString, TRUE);

  pango_layout_get_extents (pLayout, NULL, &logicalRect);
  iAdvanceWidth = logicalRect.width / PANGO_SCALE;

  /* draw pango-text as path to our cairo-context */
  pango_cairo_layout_path (pContext, pLayout);

  /* clean up */
  g_object_unref (pLayout);

  return iAdvanceWidth;
}

static gboolean
on_expose (GtkWidget* widget, GdkEventExpose* pEvent, gpointer data)
{
  CheeseCountdownPrivate *priv = CHEESE_COUNTDOWN_GET_PRIVATE (widget);
  cairo_t  *pContext = NULL;
  gdouble   fWidth   = (gdouble) widget->allocation.width;
  gdouble   fHeight  = (gdouble) widget->allocation.height;
  gint      iOffsetX = (widget->allocation.width - 4 * 24) / 2;
  gint      iOffsetY = (widget->allocation.height - 30) / 2;
  gdouble   fAlpha1;
  gdouble   fAlpha2;
  gdouble   fAlpha3;
  gdouble   fAlphaClick;

  /* deal with the timing stuff */
  if (priv->iState == STATE_3)
    fAlpha3 = 1.0f;
  else
    fAlpha3 = 0.5f;
  if (priv->iState == STATE_2)
    fAlpha2 = 1.0f;
  else
    fAlpha2 = 0.5f;
  if (priv->iState == STATE_1)
    fAlpha1 = 1.0f;
  else
    fAlpha1 = 0.5f;
  if (priv->iState == STATE_SMILE)
    fAlphaClick = 1.0f;
  else
    fAlphaClick = 0.5f;

  /* clear drawing-context */
  pContext = gdk_cairo_create (widget->window);
  cairo_set_operator (pContext, CAIRO_OPERATOR_CLEAR);
  cairo_paint (pContext);
  cairo_set_operator (pContext, CAIRO_OPERATOR_OVER);

  /* set the clipping-rectangle */
  cairo_rectangle (pContext, 0.0f, 0.0f, fWidth, fHeight);
  cairo_clip (pContext);

  /* draw the color-ish background */
  /* glossy version */
  cairo_set_source_rgba (pContext, priv->bg_light[R], priv->bg_light[G], priv->bg_light[B], 1.0f);
  cairo_rectangle (pContext, 0.0f, 0.0f, fWidth, fHeight);
  cairo_fill (pContext);

  cairo_set_source_rgba (pContext, priv->bg[R], priv->bg[G], priv->bg[B], 1.0f);
  cairo_rectangle (pContext, 0.0f, fHeight / 2.0f, fWidth, fHeight / 2.0f);
  cairo_fill (pContext);
  /* plain version */
  /*
  cairo_set_source_rgba (pContext, bg[R], bg[G], bg[B], 1.0f);
  cairo_rectangle (pContext, 0.0f, 0.0f, fWidth, fHeight);
  cairo_fill (pContext);
  */

  /* draw the 3 */
  cairo_set_source_rgba (pContext, priv->text[R], priv->text[G], priv->text[B], fAlpha3);
  cairo_move_to (pContext, (gdouble) iOffsetX, (gdouble) iOffsetY);
  iOffsetX += do_text (pContext,
                       "3 ",
                       24 * PANGO_SCALE,
                       "Bitstream Charter",
                       PANGO_WEIGHT_BOLD,
                       PANGO_STYLE_NORMAL);
  cairo_fill (pContext);

  /* draw the 2 */
  cairo_set_source_rgba (pContext, priv->text[R], priv->text[G], priv->text[B], fAlpha2);
  cairo_move_to (pContext, (gdouble) iOffsetX, (gdouble) iOffsetY);
  iOffsetX += do_text (pContext,
                       "2 ",
                       24 * PANGO_SCALE,
                       "Bitstream Charter",
                       PANGO_WEIGHT_BOLD,
                       PANGO_STYLE_NORMAL);
  cairo_fill (pContext);

  /* draw the 1 */
  cairo_set_source_rgba (pContext, priv->text[R], priv->text[G], priv->text[B], fAlpha1);
  cairo_move_to (pContext, (gdouble) iOffsetX, (gdouble) iOffsetY);
  iOffsetX += do_text (pContext,
                       "1 ",
                       24 * PANGO_SCALE,
                       "Bitstream Charter",
                       PANGO_WEIGHT_BOLD,
                       PANGO_STYLE_NORMAL);
  cairo_fill (pContext);

  /* draw the camera */
  cairo_set_source_surface (pContext,
                            priv->pSurface,
                            (gdouble) iOffsetX,
                            (gdouble) iOffsetY);
  cairo_paint_with_alpha (pContext, fAlphaClick);

  cairo_destroy (pContext);

  return FALSE;
}

static gboolean
redraw_handler (gpointer data)
{
  GtkWidget* widget = (GtkWidget*) data;

  gtk_widget_queue_draw (widget);
  return TRUE;
}

static cairo_surface_t*
create_surface_from_svg (GtkWidget *widget, gchar* pcFilename)
{
  cairo_surface_t*  pSurface   = NULL;
  RsvgHandle*       pSvgHandle = NULL;
  GError*           pError     = NULL;
  cairo_t*          pContext   = NULL;
  RsvgDimensionData dimension;

  CheeseCountdownPrivate* priv = CHEESE_COUNTDOWN_GET_PRIVATE (widget);

  rsvg_init ();

  /* load svg-file from disk */
  pSvgHandle = rsvg_handle_new_from_file (pcFilename, &pError);
  if (!pSvgHandle)
  {
    g_print ("Could not load file %s!\n", pcFilename);
    g_error_free (pError);
    return NULL;
  }

  /* get size of svg */
  rsvg_handle_get_dimensions (pSvgHandle, &dimension);

  /* create a surface-buffer */
  pSurface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                         dimension.width,
                                         dimension.height);
  if (cairo_surface_status (pSurface) != CAIRO_STATUS_SUCCESS)
  {
    g_print ("Could not create surface for svg-icon!\n");
    return NULL;
  }

  /* create a temporary drawing-context */
  pContext = cairo_create (pSurface);
  if (cairo_status (pContext) != CAIRO_STATUS_SUCCESS)
  {
    g_print ("Could not create temp drawing-context!\n");
    return NULL;
  }

  /* clear that context */
  cairo_set_source_rgba (pContext, priv->text[R], priv->text[G], priv->text[B], 1.0f);
  cairo_set_operator (pContext, CAIRO_OPERATOR_XOR);
  cairo_paint (pContext);

  /* draw icon into surface-buffer */
  rsvg_handle_render_cairo (pSvgHandle, pContext);

  /* clean up */
  rsvg_handle_free (pSvgHandle);
  rsvg_term ();
  cairo_destroy (pContext);

  return pSurface;
}

static void
on_style_set_cb (GtkWidget *widget, GtkStyle *previous_style, gpointer data)
{
  CheeseCountdownPrivate* priv = CHEESE_COUNTDOWN_GET_PRIVATE (data);

  GdkColor *color_bg = &GTK_WIDGET(widget)->style->bg[GTK_STATE_SELECTED];
  GdkColor *color_text = &GTK_WIDGET(widget)->style->fg[GTK_STATE_SELECTED];
  priv->bg[R] = ((double)color_bg->red) / 65535;
  priv->bg[G] = ((double)color_bg->green) / 65535;
  priv->bg[B] = ((double)color_bg->blue) / 65535;
  priv->text[R] = ((double)color_text->red) / 65535;
  priv->text[G] = ((double)color_text->green) / 65535;
  priv->text[B] = ((double)color_text->blue) / 65535;
  gdouble h, s, v;
  gdouble s_mid, s_light, s_dark;

  h = priv->bg[R]; s = priv->bg[G]; v = priv->bg[B];
  rgb_to_hsv (&h, &s, &v);

  s_mid = s;
  if (s_mid <= 0.125)
    s_mid = 0.125;
  if (s_mid >= 0.875)
    s_mid = 0.875;
  s_dark = s_mid - 0.125;
  s_light = s_mid + 0.125;

  priv->bg[R] = h; priv->bg[G] = s_light; priv->bg[B] = v;
  hsv_to_rgb (&priv->bg[R], &priv->bg[G], &priv->bg[B]);

  priv->bg_light[R] = h; priv->bg_light[G] = s_dark; priv->bg_light[B] = v;
  hsv_to_rgb (&priv->bg_light[R], &priv->bg_light[G], &priv->bg_light[B]);

  /* create/load svg-icon */
  g_free(priv->pSurface);
  priv->pSurface = create_surface_from_svg (widget, PACKAGE_DATADIR "/pixmaps/camera-icon.svg");
}

static gboolean
cheese_countdown_cb (gpointer countdown)
{
  CheeseCountdownPrivate* priv = CHEESE_COUNTDOWN_GET_PRIVATE (countdown);

  switch (priv->iState)
  {
    case STATE_OFF:
      /* Countdown was cancelled */
      return FALSE;

    case STATE_3:
      priv->iState = STATE_2;
      break;

    case STATE_2:
      priv->iState = STATE_1;
      break;

    case STATE_1:
      priv->iState = STATE_SMILE;
      if (priv->picture_callback != NULL) (priv->picture_callback)(priv->callback_data);
      break;

    case STATE_SMILE:
      priv->iState = STATE_OFF;
      if (priv->hide_callback != NULL) (priv->hide_callback)(priv->callback_data);
      return FALSE;
  }

  return TRUE;
}

void
cheese_countdown_start (CheeseCountdown *countdown, cheese_countdown_cb_t picture_cb, cheese_countdown_cb_t hide_cb, gpointer data)
{
  CheeseCountdownPrivate* priv = CHEESE_COUNTDOWN_GET_PRIVATE (countdown);
  if (priv->iState != STATE_OFF)
  {
    g_print ("Should not happen, state is not off.\n");
  }
  priv->iState = STATE_3;
  priv->picture_callback = picture_cb;
  priv->hide_callback = hide_cb;
  priv->callback_data = data;
  g_timeout_add_seconds (1, cheese_countdown_cb, (gpointer) countdown);
}

void
cheese_countdown_cancel (CheeseCountdown *countdown)
{
  CheeseCountdownPrivate* priv = CHEESE_COUNTDOWN_GET_PRIVATE (countdown);
  priv->iState = STATE_OFF;
}

static void
cheese_countdown_finalize (GObject *object)
{
  G_OBJECT_CLASS (cheese_countdown_parent_class)->finalize (object);
}

static void
cheese_countdown_class_init (CheeseCountdownClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = cheese_countdown_finalize;

  g_type_class_add_private (klass, sizeof (CheeseCountdownPrivate));
}

static void
cheese_countdown_init (CheeseCountdown *countdown)
{
  CheeseCountdownPrivate* priv = CHEESE_COUNTDOWN_GET_PRIVATE (countdown);

  priv->iState = STATE_OFF;
  priv->picture_callback = NULL;
  priv->hide_callback = NULL;

  g_signal_connect (G_OBJECT (countdown), "expose-event",
                    G_CALLBACK (on_expose), NULL);
  g_signal_connect (G_OBJECT (countdown), "style-set",
                    G_CALLBACK (on_style_set_cb), countdown);

  g_timeout_add (100, redraw_handler, (gpointer) countdown);
}

GtkWidget * 
cheese_countdown_new ()
{
  CheeseCountdown *countdown;
  countdown = g_object_new (CHEESE_TYPE_COUNTDOWN, NULL);  

  return GTK_WIDGET (countdown);
}
