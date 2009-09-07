/*
 * Copyright © 2007,2008 daniel g. siegel <dgsiegel@gnome.org>
 * Copyright © 2007,2008 Jaap Haitsma <jaap@haitsma.org>
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

#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <cairo.h>

#include "cheese-effect-chooser.h"
#include "cheese-webcam.h"

#define BOARD_COLS  4
#define BOARD_ROWS  3
#define NUM_EFFECTS (BOARD_ROWS * BOARD_COLS)

#define SHRINK(cr, x) cairo_translate (cr, (1 - (x)) / 2.0, (1 - (x)) / 2.0); cairo_scale (cr, (x), (x))

#define CHEESE_EFFECT_CHOOSER_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHEESE_TYPE_EFFECT_CHOOSER, CheeseEffectChooserPrivate))

G_DEFINE_TYPE (CheeseEffectChooser, cheese_effect_chooser, GTK_TYPE_DRAWING_AREA);

typedef struct
{
  gboolean selected[NUM_EFFECTS];
} CheeseEffectChooserPrivate;


typedef struct
{
  CheeseWebcamEffect effect;
  char *name;
  char *filename;
  gboolean is_black;
} GstEffect;

static const GstEffect GST_EFFECT[] = {
  {CHEESE_WEBCAM_EFFECT_NO_EFFECT,       N_("No Effect"),
   PACKAGE_DATADIR "/effects/identity.png", FALSE},
  {CHEESE_WEBCAM_EFFECT_MAUVE,           N_("Mauve"),
   PACKAGE_DATADIR "/effects/Mauve.png", FALSE},
  {CHEESE_WEBCAM_EFFECT_NOIR_BLANC,      N_("Noir/Blanc"),
   PACKAGE_DATADIR "/effects/NoirBlanc.png", FALSE},
  {CHEESE_WEBCAM_EFFECT_SATURATION,      N_("Saturation"),
   PACKAGE_DATADIR "/effects/Saturation.png", FALSE},
  {CHEESE_WEBCAM_EFFECT_HULK,            N_("Hulk"),
   PACKAGE_DATADIR "/effects/Hulk.png", FALSE},
  {CHEESE_WEBCAM_EFFECT_VERTICAL_FLIP,   N_("Vertical Flip"),
   PACKAGE_DATADIR "/effects/videoflip_v.png", FALSE},
  {CHEESE_WEBCAM_EFFECT_HORIZONTAL_FLIP, N_("Horizontal Flip"),
   PACKAGE_DATADIR "/effects/videoflip_h.png", FALSE},
  {CHEESE_WEBCAM_EFFECT_SHAGADELIC,      N_("Shagadelic"),
   PACKAGE_DATADIR "/effects/shagadelictv.png", FALSE},
  {CHEESE_WEBCAM_EFFECT_VERTIGO,         N_("Vertigo"),
   PACKAGE_DATADIR "/effects/vertigotv.png", FALSE},
  {CHEESE_WEBCAM_EFFECT_EDGE,            N_("Edge"),
   PACKAGE_DATADIR "/effects/edgetv.png", TRUE},
  {CHEESE_WEBCAM_EFFECT_DICE,            N_("Dice"),
   PACKAGE_DATADIR "/effects/dicetv.png", FALSE},
  {CHEESE_WEBCAM_EFFECT_WARP,            N_("Warp"),
   PACKAGE_DATADIR "/effects/warptv.png", FALSE}
};


static void
cheese_cairo_rectangle_round (cairo_t *cr,
                              double x0, double y0,
                              double width, double height, double radius)
{
  double x1, y1;

  x1 = x0 + width;
  y1 = y0 + height;

  if (!width || !height)
    return;
  if (width / 2 < radius)
  {
    if (height / 2 < radius)
    {
      cairo_move_to (cr, x0, (y0 + y1) / 2);
      cairo_curve_to (cr, x0, y0, x0, y0, (x0 + x1) / 2, y0);
      cairo_curve_to (cr, x1, y0, x1, y0, x1, (y0 + y1) / 2);
      cairo_curve_to (cr, x1, y1, x1, y1, (x1 + x0) / 2, y1);
      cairo_curve_to (cr, x0, y1, x0, y1, x0, (y0 + y1) / 2);
    }
    else
    {
      cairo_move_to (cr, x0, y0 + radius);
      cairo_curve_to (cr, x0, y0, x0, y0, (x0 + x1) / 2, y0);
      cairo_curve_to (cr, x1, y0, x1, y0, x1, y0 + radius);
      cairo_line_to (cr, x1, y1 - radius);
      cairo_curve_to (cr, x1, y1, x1, y1, (x1 + x0) / 2, y1);
      cairo_curve_to (cr, x0, y1, x0, y1, x0, y1 - radius);
    }
  }
  else
  {
    if (height / 2 < radius)
    {
      cairo_move_to (cr, x0, (y0 + y1) / 2);
      cairo_curve_to (cr, x0, y0, x0, y0, x0 + radius, y0);
      cairo_line_to (cr, x1 - radius, y0);
      cairo_curve_to (cr, x1, y0, x1, y0, x1, (y0 + y1) / 2);
      cairo_curve_to (cr, x1, y1, x1, y1, x1 - radius, y1);
      cairo_line_to (cr, x0 + radius, y1);
      cairo_curve_to (cr, x0, y1, x0, y1, x0, (y0 + y1) / 2);
    }
    else
    {
      cairo_move_to (cr, x0, y0 + radius);
      cairo_curve_to (cr, x0, y0, x0, y0, x0 + radius, y0);
      cairo_line_to (cr, x1 - radius, y0);
      cairo_curve_to (cr, x1, y0, x1, y0, x1, y0 + radius);
      cairo_line_to (cr, x1, y1 - radius);
      cairo_curve_to (cr, x1, y1, x1, y1, x1 - radius, y1);
      cairo_line_to (cr, x0 + radius, y1);
      cairo_curve_to (cr, x0, y1, x0, y1, x0, y1 - radius);
    }
  }
  cairo_close_path (cr);
}

static void
cheese_cairo_draw_card (cairo_t *cr, const GstEffect *card, gboolean highlight)
{
  int              w, h;
  cairo_surface_t *image;

  cairo_save (cr);

  SHRINK (cr, .9);

  cheese_cairo_rectangle_round (cr, 0, 0, 1.0, 1.0, 0.1);
  cairo_set_source_rgb (cr, 0, 0, 0);

  cairo_save (cr);

  cheese_cairo_rectangle_round (cr, 0, 0, 1.0, 1.0, 0.1);
  cairo_clip (cr);
  cairo_new_path (cr);

  image = cairo_image_surface_create_from_png (card->filename);
  w     = cairo_image_surface_get_width (image);
  h     = cairo_image_surface_get_height (image);

  cairo_scale (cr, 1.0 / w, 1.0 / h);

  cairo_set_source_surface (cr, image, 0, 0);
  cairo_paint (cr);

  cairo_surface_destroy (image);
  cairo_restore (cr);

  cairo_text_extents_t extents;
  if (card->is_black)
    cairo_set_source_rgb (cr, 1, 1, 1);

  double x, y;
  cairo_select_font_face (cr, "Sans",
                          CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

  cairo_set_font_size (cr, 0.09);
  gchar *name = gettext (card->name);
  cairo_text_extents (cr, name, &extents);
  x = 0.5 - (extents.width / 2 + extents.x_bearing);
  y = 0.92 - (extents.height / 2 + extents.y_bearing);

  cairo_move_to (cr, x, y);
  cairo_show_text (cr, name);

  if (highlight)
  {
    cheese_cairo_rectangle_round (cr, 0, 0, 1.0, 1.0, 0.1);
    cairo_set_source_rgba (cr, 0, 0, 1, 0.25);
    cairo_fill (cr);
  }
  cairo_restore (cr);
}

static void
cheese_effect_chooser_expose_cb (GtkWidget *widget, GdkEventExpose *expose_event, gpointer data)
{
  CheeseEffectChooserPrivate *priv = CHEESE_EFFECT_CHOOSER_GET_PRIVATE (widget);

  int      width, height;
  int      i;
  cairo_t *cr;

  width  = widget->allocation.width;
  height = widget->allocation.height;

  cr = gdk_cairo_create (gtk_widget_get_window (widget));

  cairo_save (cr);
  cairo_scale (cr, width, height);

  for (i = 0; i < NUM_EFFECTS; i++)
  {
    cairo_save (cr);
    cairo_translate (cr, 1.0 / BOARD_COLS * (i % BOARD_COLS),
                     1.0 / BOARD_ROWS * (i / BOARD_COLS));
    cairo_scale (cr, 1.0 / BOARD_COLS, 1.0 / BOARD_ROWS);
    cheese_cairo_draw_card (cr, &(GST_EFFECT[i]), priv->selected[i]);
    cairo_restore (cr);
  }

  cairo_restore (cr);
}

static gboolean
cheese_effect_chooser_button_press_event_cb (GtkWidget *widget, GdkEventButton *button_event, gpointer data)
{
  CheeseEffectChooserPrivate *priv = CHEESE_EFFECT_CHOOSER_GET_PRIVATE (widget);

  int i;
  int col  = (int) (button_event->x / widget->allocation.width * BOARD_COLS);
  int row  = (int) (button_event->y / widget->allocation.height * BOARD_ROWS);
  int slot = (row * BOARD_COLS + col);

  priv->selected[slot] = !priv->selected[slot];

  if (priv->selected[0] == TRUE)
  {
    for (i = 0; i < NUM_EFFECTS; i++)
      priv->selected[i] = FALSE;
  }

  gtk_widget_queue_draw (widget);

  return TRUE;
}

CheeseWebcamEffect
cheese_effect_chooser_get_selection (CheeseEffectChooser *effect_chooser)
{
  CheeseEffectChooserPrivate *priv = CHEESE_EFFECT_CHOOSER_GET_PRIVATE (effect_chooser);

  int                i;
  CheeseWebcamEffect effect = 0;

  for (i = 0; i < NUM_EFFECTS; i++)
  {
    if (priv->selected[i])
    {
      effect |= GST_EFFECT[i].effect;
    }
  }
  return effect;
}

char *
cheese_effect_chooser_get_selection_string (CheeseEffectChooser *effect_chooser)
{
  CheeseEffectChooserPrivate *priv = CHEESE_EFFECT_CHOOSER_GET_PRIVATE (effect_chooser);

  int   i;
  char *effects = NULL;

  for (i = 0; i < NUM_EFFECTS; i++)
  {
    if (priv->selected[i])
    {
      if (effects == NULL)
        effects = GST_EFFECT[i].name;
      else
        effects = g_strjoin (",", effects, GST_EFFECT[i].name, NULL);
    }
  }
  return effects;
}

static void
cheese_effect_chooser_finalize (GObject *object)
{
  G_OBJECT_CLASS (cheese_effect_chooser_parent_class)->finalize (object);
}

static void
cheese_effect_chooser_class_init (CheeseEffectChooserClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = cheese_effect_chooser_finalize;

  g_type_class_add_private (klass, sizeof (CheeseEffectChooserPrivate));
}

static void
cheese_effect_chooser_init (CheeseEffectChooser *effect_chooser)
{
  gtk_widget_add_events (GTK_WIDGET (effect_chooser), GDK_BUTTON_PRESS_MASK);

  g_signal_connect (G_OBJECT (effect_chooser), "button_press_event",
                    G_CALLBACK (cheese_effect_chooser_button_press_event_cb), NULL);
  g_signal_connect (G_OBJECT (effect_chooser), "expose-event",
                    G_CALLBACK (cheese_effect_chooser_expose_cb), NULL);
}

GtkWidget *
cheese_effect_chooser_new (char *selected_effects)
{
  CheeseEffectChooser *effect_chooser;
  int                  i;

  effect_chooser = g_object_new (CHEESE_TYPE_EFFECT_CHOOSER, NULL);

  CheeseEffectChooserPrivate *priv = CHEESE_EFFECT_CHOOSER_GET_PRIVATE (effect_chooser);

  if (selected_effects != NULL)
  {
    for (i = 1; i < NUM_EFFECTS; i++)
    {
      if (strstr (selected_effects, GST_EFFECT[i].name) != NULL)
        priv->selected[i] = TRUE;
    }
  }

  return GTK_WIDGET (effect_chooser);
}
