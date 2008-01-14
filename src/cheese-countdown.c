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
} CheeseCountdownPrivate;

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
  CheeseCountdownPrivate* priv = CHEESE_COUNTDOWN_GET_PRIVATE (widget);
  cairo_t*  pContext = NULL;
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

  /* draw the red-ish background */
  /* glossy version */
  /*
  cairo_set_source_rgba (pContext, 1.0f, 0.5f, 0.5f, 1.0f);
  cairo_rectangle (pContext, 0.0f, 0.0f, fWidth, fHeight);
  cairo_fill (pContext);
  cairo_set_source_rgba (pContext, 1.0f, 0.25f, 0.25f, 1.0f);
  cairo_rectangle (pContext, 0.0f, fHeight / 2.0f, fWidth, fHeight / 2.0f);
  cairo_fill (pContext);
  */
  /* plain version */
  cairo_set_source_rgba (pContext, 1.0f, 0.25f, 0.25f, 1.0f);
  cairo_rectangle (pContext, 0.0f, 0.0f, fWidth, fHeight);
  cairo_fill (pContext);

  /* draw the 3 */
  cairo_set_source_rgba (pContext, 1.0f, 1.0f, 1.0f, fAlpha3);
  cairo_move_to (pContext, (gdouble) iOffsetX, (gdouble) iOffsetY);
  iOffsetX += do_text (pContext,
           "3 ",
           24 * PANGO_SCALE,
           "Bitstream Charter",
           PANGO_WEIGHT_BOLD,
           PANGO_STYLE_NORMAL);
  cairo_fill (pContext);

  /* draw the 2 */
  cairo_set_source_rgba (pContext, 1.0f, 1.0f, 1.0f, fAlpha2);
  cairo_move_to (pContext, (gdouble) iOffsetX, (gdouble) iOffsetY);
  iOffsetX += do_text (pContext,
           "2 ",
           24 * PANGO_SCALE,
           "Bitstream Charter",
           PANGO_WEIGHT_BOLD,
           PANGO_STYLE_NORMAL);
  cairo_fill (pContext);

  /* draw the 1 */
  cairo_set_source_rgba (pContext, 1.0f, 1.0f, 1.0f, fAlpha1);
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
create_surface_from_svg (gchar* pcFilename)
{
  cairo_surface_t*  pSurface   = NULL;
  RsvgHandle*       pSvgHandle = NULL;
  GError*           pError     = NULL;
  RsvgDimensionData dimension;
  cairo_t*          pContext   = NULL;

  rsvg_init ();

  /* load svg-file from disk */
  pSvgHandle = rsvg_handle_new_from_file (pcFilename, &pError);
  if (!pSvgHandle)
  {
    g_print ("Could not load file %s!\n", pcFilename);
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
  cairo_set_source_rgba (pContext, 1.0f, 1.0f, 1.0f, 0.0f);
  cairo_set_operator (pContext, CAIRO_OPERATOR_OVER);
  cairo_paint (pContext);

  /* draw icon into surface-buffer */
  rsvg_handle_render_cairo (pSvgHandle, pContext);

  /* clean up */
  rsvg_handle_free (pSvgHandle);
  rsvg_term ();
  cairo_destroy (pContext);

  return pSurface;
}

static gboolean
cheese_countdown_cb (gpointer countdown)
{
  CheeseCountdownPrivate* priv = CHEESE_COUNTDOWN_GET_PRIVATE (countdown);
  if (priv->iState == STATE_OFF)
  {
    // should just ignore it, testing purposes
    g_print("Should not happen, unitialized state in countdown handler\n");
  }
  else if (priv->iState == STATE_3)
  {
    priv->iState = STATE_2;
  }
  else if (priv->iState == STATE_2)
  {
    priv->iState = STATE_1;
  }
  else if (priv->iState == STATE_1)
  {
    priv->iState = STATE_SMILE;
    if (priv->picture_callback != NULL) (priv->picture_callback)(priv->callback_data);
  }
  else if (priv->iState == STATE_SMILE)
  {
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

  /* create/load svg-icon */
  priv->pSurface = create_surface_from_svg (PACKAGE_DATADIR "/pixmaps/camera-icon.svg");
  priv->iState = STATE_OFF;
  priv->picture_callback = NULL;
  priv->hide_callback = NULL;

  g_signal_connect (G_OBJECT (countdown), "expose-event",
        G_CALLBACK (on_expose), NULL);

  g_timeout_add (100, redraw_handler, (gpointer) countdown);
}

GtkWidget * 
cheese_countdown_new ()
{
  CheeseCountdown *countdown;
  countdown = g_object_new (CHEESE_TYPE_COUNTDOWN, NULL);  

  return GTK_WIDGET (countdown);
}
