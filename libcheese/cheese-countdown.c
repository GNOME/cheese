/*
 * Copyright © 2008 Mirco "MacSlow" Müller <macslow@bangang.de>
 * Copyright © 2008 daniel g. siegel <dgsiegel@gnome.org>
 * Copyright © 2008 Patryk Zawadzki <patrys@pld-linux.org>
 * Copyright © 2008 Andrea Cimitan <andrea.cimitan@gmail.com>
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
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>
#include <math.h>

#include "cheese-countdown.h"

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
  gdouble r;
  gdouble g;
  gdouble b;
  gdouble a;
} CairoColor;

typedef struct
{
  int iState;
  cairo_surface_t *pSurface;
  cheese_countdown_cb_t picture_callback;
  cheese_countdown_cb_t hide_callback;
  gpointer callback_data;
  CairoColor bg;
  CairoColor text;
} CheeseCountdownPrivate;

/* Converts from RGB TO HLS */
static void
rgb_to_hls (gdouble *r,
            gdouble *g,
            gdouble *b)
{
  gdouble min;
  gdouble max;
  gdouble red;
  gdouble green;
  gdouble blue;
  gdouble h, l, s;
  gdouble delta;

  red   = *r;
  green = *g;
  blue  = *b;

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

  l = (max + min) / 2;
  if (fabs (max - min) < 0.0001)
  {
    h = 0;
    s = 0;
  }
  else
  {
    if (l <= 0.5)
      s = (max - min) / (max + min);
    else
      s = (max - min) / (2 - max - min);

    delta = max - min;
    if (red == max)
      h = (green - blue) / delta;
    else if (green == max)
      h = 2 + (blue - red) / delta;
    else if (blue == max)
      h = 4 + (red - green) / delta;

    h *= 60;
    if (h < 0.0)
      h += 360;
  }

  *r = h;
  *g = l;
  *b = s;
}

/* Converts from HLS to RGB */
static void
hls_to_rgb (gdouble *h,
            gdouble *l,
            gdouble *s)
{
  gdouble hue;
  gdouble lightness;
  gdouble saturation;
  gdouble m1, m2;
  gdouble r, g, b;

  lightness  = *l;
  saturation = *s;

  if (lightness <= 0.5)
    m2 = lightness * (1 + saturation);
  else
    m2 = lightness + saturation - lightness * saturation;

  m1 = 2 * lightness - m2;

  if (saturation == 0)
  {
    *h = lightness;
    *l = lightness;
    *s = lightness;
  }
  else
  {
    hue = *h + 120;
    while (hue > 360)
      hue -= 360;
    while (hue < 0)
      hue += 360;

    if (hue < 60)
      r = m1 + (m2 - m1) * hue / 60;
    else if (hue < 180)
      r = m2;
    else if (hue < 240)
      r = m1 + (m2 - m1) * (240 - hue) / 60;
    else
      r = m1;

    hue = *h;
    while (hue > 360)
      hue -= 360;
    while (hue < 0)
      hue += 360;

    if (hue < 60)
      g = m1 + (m2 - m1) * hue / 60;
    else if (hue < 180)
      g = m2;
    else if (hue < 240)
      g = m1 + (m2 - m1) * (240 - hue) / 60;
    else
      g = m1;

    hue = *h - 120;
    while (hue > 360)
      hue -= 360;
    while (hue < 0)
      hue += 360;

    if (hue < 60)
      b = m1 + (m2 - m1) * hue / 60;
    else if (hue < 180)
      b = m2;
    else if (hue < 240)
      b = m1 + (m2 - m1) * (240 - hue) / 60;
    else
      b = m1;

    *h = r;
    *l = g;
    *s = b;
  }
}

/* Performs a color shading operation */
static void
color_shade (const CairoColor *a, float k, CairoColor *b)
{
  double red;
  double green;
  double blue;
  double alpha;

  red   = a->r;
  green = a->g;
  blue  = a->b;
  alpha = a->a;

  if (k == 1.0)
  {
    b->r = red;
    b->g = green;
    b->b = blue;
    return;
  }

  rgb_to_hls (&red, &green, &blue);

  green *= k;
  if (green > 1.0)
    green = 1.0;
  else if (green < 0.0)
    green = 0.0;

  blue *= k;
  if (blue > 1.0)
    blue = 1.0;
  else if (blue < 0.0)
    blue = 0.0;

  hls_to_rgb (&red, &green, &blue);

  b->r = red;
  b->g = green;
  b->b = blue;
  b->a = alpha;
}

static int
do_text (cairo_t    *pContext,
         gchar      *pcText,
         int         iFontSize,
         gchar      *pcFontFamily,
         PangoWeight fontWeight,
         PangoStyle  fontStyle)
{
  PangoFontDescription *pDesc       = NULL;
  PangoLayout          *pLayout     = NULL;
  GString              *pTextString = NULL;
  PangoRectangle        logicalRect;
  int                   iAdvanceWidth;

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
  g_string_printf (pTextString, "%s", pcText);
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
on_expose (GtkWidget *widget, GdkEventExpose *pEvent, gpointer data)
{
  CheeseCountdownPrivate *priv     = CHEESE_COUNTDOWN_GET_PRIVATE (widget);
  cairo_t                *pContext = NULL;
  cairo_pattern_t        *pattern;
  CairoColor              bgBorder;
  CairoColor              bgHighlight;
  CairoColor              bgShade1;
  CairoColor              bgShade2;
  CairoColor              bgShade3;
  CairoColor              bgShade4;
  gdouble                 fWidth  = (gdouble) widget->allocation.width;
  gdouble                 fHeight = (gdouble) widget->allocation.height;

  /* 3 * 26 are the three numbers, 30 is the width of camera-icon.svg */
  int     iOffsetX = (widget->allocation.width - 3 * 26 - 30) / 2;
  int     iOffsetY = (widget->allocation.height - 30) / 2;
  gdouble fAlpha1;
  gdouble fAlpha2;
  gdouble fAlpha3;
  gdouble fAlphaClick;

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

  /* shade the colors used in the pattern */
  color_shade (&priv->bg, 0.65f, &bgBorder);
  color_shade (&priv->bg, 1.26f, &bgHighlight);
  color_shade (&priv->bg, 1.16f, &bgShade1);
  color_shade (&priv->bg, 1.08f, &bgShade2);
  color_shade (&priv->bg, 1.00f, &bgShade3);
  color_shade (&priv->bg, 1.08f, &bgShade4);

  /* create cairo context */
  pContext = gdk_cairo_create (gtk_widget_get_window (widget));
  cairo_set_line_width (pContext, 1.0f);

  /* clear drawing-context */
  cairo_set_operator (pContext, CAIRO_OPERATOR_CLEAR);
  cairo_paint (pContext);
  cairo_set_operator (pContext, CAIRO_OPERATOR_OVER);

  /* set the clipping-rectangle */
  cairo_rectangle (pContext, 0.0f, 0.0f, fWidth, fHeight);
  cairo_clip (pContext);

  /* draw the color-ish background */
  cairo_rectangle (pContext, 0.0f, 0.0f, fWidth, fHeight);
  pattern = cairo_pattern_create_linear (0.0f, 0.0f, 0.0f, fHeight);
  cairo_pattern_add_color_stop_rgba (pattern, 0.00f, bgShade1.r, bgShade1.g, bgShade1.b, 1.0f);
  cairo_pattern_add_color_stop_rgba (pattern, 0.49f, bgShade2.r, bgShade2.g, bgShade2.b, 1.0f);
  cairo_pattern_add_color_stop_rgba (pattern, 0.49f, bgShade3.r, bgShade3.g, bgShade3.b, 1.0f);
  cairo_pattern_add_color_stop_rgba (pattern, 1.00f, bgShade4.r, bgShade4.g, bgShade4.b, 1.0f);
  cairo_set_source (pContext, pattern);
  cairo_pattern_destroy (pattern);
  cairo_fill (pContext);

  /* draw border */
  cairo_rectangle (pContext, 0.5f, 0.5f, fWidth - 1.0f, fHeight - 1.0f);
  cairo_set_source_rgba (pContext, bgBorder.r, bgBorder.g, bgBorder.b, 0.6f);
  cairo_stroke (pContext);

  /* draw inner highlight */
  cairo_rectangle (pContext, 1.5f, 1.5f, fWidth - 3.0f, fHeight - 3.0f);
  cairo_set_source_rgba (pContext, bgHighlight.r, bgHighlight.g, bgHighlight.b, 0.5f);
  cairo_stroke (pContext);

  /* plain version */

  /*
   * cairo_set_source_rgba (pContext, bg.r, bg.g, bg.b, 1.0f);
   * cairo_rectangle (pContext, 0.0f, 0.0f, fWidth, fHeight);
   * cairo_fill (pContext);
   */

  char *number;

  /* draw the 3 */
  cairo_set_source_rgba (pContext, priv->text.r, priv->text.g, priv->text.b, fAlpha3);
  cairo_move_to (pContext, (gdouble) iOffsetX, (gdouble) iOffsetY);

  /* TRANSLATORS:
   * This is the countdown number when taking the photo.
   * If you leave as is (that is, %d), it will show 3, 2, 1, 0.
   * To enable to show the numbers in your own language, use %Id instead.
   * Please leave the additional whitespace after the number
   */
  number    = g_strdup_printf (_("%d "), 3);
  iOffsetX += do_text (pContext,
                       number,
                       26 * PANGO_SCALE,
                       "Bitstream Vera Sans Bold",
                       PANGO_WEIGHT_BOLD,
                       PANGO_STYLE_NORMAL);
  cairo_fill (pContext);

  /* draw the 2 */
  cairo_set_source_rgba (pContext, priv->text.r, priv->text.g, priv->text.b, fAlpha2);
  cairo_move_to (pContext, (gdouble) iOffsetX, (gdouble) iOffsetY);

  /* TRANSLATORS:
   * This is the countdown number when taking the photo.
   * If you leave as is (that is, %d), it will show 3, 2, 1, 0.
   * To enable to show the numbers in your own language, use %Id instead.
   * Please leave the additional whitespace after the number
   */
  number    = g_strdup_printf (_("%d "), 2);
  iOffsetX += do_text (pContext,
                       number,
                       26 * PANGO_SCALE,
                       "Bitstream Vera Sans Bold",
                       PANGO_WEIGHT_BOLD,
                       PANGO_STYLE_NORMAL);
  cairo_fill (pContext);

  /* draw the 1 */
  cairo_set_source_rgba (pContext, priv->text.r, priv->text.g, priv->text.b, fAlpha1);
  cairo_move_to (pContext, (gdouble) iOffsetX, (gdouble) iOffsetY);

  /* TRANSLATORS:
   * This is the countdown number when taking the photo.
   * If you leave as is (that is, %d), it will show 3, 2, 1, 0.
   * To enable to show the numbers in your own language, use %Id instead.
   * Please leave the additional whitespace after the number
   */
  number    = g_strdup_printf (_("%d "), 1);
  iOffsetX += do_text (pContext,
                       number,
                       26 * PANGO_SCALE,
                       "Bitstream Vera Sans Bold",
                       PANGO_WEIGHT_BOLD,
                       PANGO_STYLE_NORMAL);
  cairo_fill (pContext);
  g_free (number);

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
  GtkWidget *widget = (GtkWidget *) data;

  gtk_widget_queue_draw (widget);
  return TRUE;
}

static cairo_surface_t *
create_surface_from_svg (GtkWidget *widget, gchar *pcFilename)
{
  cairo_surface_t  *pSurface   = NULL;
  RsvgHandle       *pSvgHandle = NULL;
  GError           *pError     = NULL;
  cairo_t          *pContext   = NULL;
  RsvgDimensionData dimension;

  CheeseCountdownPrivate *priv = CHEESE_COUNTDOWN_GET_PRIVATE (widget);

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
  cairo_set_source_rgba (pContext, priv->text.r, priv->text.g, priv->text.b, 1.0f);
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
  CheeseCountdownPrivate *priv  = CHEESE_COUNTDOWN_GET_PRIVATE (data);
  GtkStyle               *style = gtk_widget_get_style (GTK_WIDGET (widget));

  GdkColor *color_bg   = &style->bg[GTK_STATE_SELECTED];
  GdkColor *color_text = &style->fg[GTK_STATE_SELECTED];

  priv->bg.r   = ((double) color_bg->red) / 65535;
  priv->bg.g   = ((double) color_bg->green) / 65535;
  priv->bg.b   = ((double) color_bg->blue) / 65535;
  priv->bg.a   = 1.0f;
  priv->text.r = ((double) color_text->red) / 65535;
  priv->text.g = ((double) color_text->green) / 65535;
  priv->text.b = ((double) color_text->blue) / 65535;
  priv->text.a = 1.0f;

  /* create/load svg-icon */
  g_free (priv->pSurface);
  priv->pSurface = create_surface_from_svg (widget, PACKAGE_DATADIR "/pixmaps/camera-icon.svg");
}

static gboolean
cheese_countdown_cb (gpointer countdown)
{
  CheeseCountdownPrivate *priv = CHEESE_COUNTDOWN_GET_PRIVATE (countdown);

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
cheese_countdown_start (CheeseCountdown      *countdown,
                        cheese_countdown_cb_t picture_cb,
                        cheese_countdown_cb_t hide_cb,
                        gpointer              data)
{
  CheeseCountdownPrivate *priv = CHEESE_COUNTDOWN_GET_PRIVATE (countdown);

  if (priv->iState != STATE_OFF)
  {
    g_print ("Should not happen, state is not off.\n");
    return;
  }
  priv->iState           = STATE_3;
  priv->picture_callback = picture_cb;
  priv->hide_callback    = hide_cb;
  priv->callback_data    = data;
  g_timeout_add_seconds (1, cheese_countdown_cb, (gpointer) countdown);
}

void
cheese_countdown_cancel (CheeseCountdown *countdown)
{
  CheeseCountdownPrivate *priv = CHEESE_COUNTDOWN_GET_PRIVATE (countdown);

  priv->iState = STATE_OFF;
}

int
cheese_countdown_get_state (CheeseCountdown *countdown)
{
  CheeseCountdownPrivate *priv = CHEESE_COUNTDOWN_GET_PRIVATE (countdown);

  return priv->iState;
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
  CheeseCountdownPrivate *priv = CHEESE_COUNTDOWN_GET_PRIVATE (countdown);

  priv->iState           = STATE_OFF;
  priv->picture_callback = NULL;
  priv->hide_callback    = NULL;

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
