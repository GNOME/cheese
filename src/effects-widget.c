/*
 * Copyright (C) 2007 Copyright (C) 2007 daniel g. siegel <dgsiegel@gmail.com>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <glib.h>

#include <gtk/gtk.h>
#include <cairo.h>

#include <libintl.h>

#include "cheese.h"
#include "window.h"
#include "cheese_config.h"
#include "effects-widget.h"
#include "cairo-custom.h"

#define _(str) gettext(str)

#define DEFAULT_WIDTH  640
#define DEFAULT_HEIGHT 480
#define BOARD_COLS 4
#define BOARD_ROWS 3
#define MAX_EFFECTS (BOARD_ROWS * BOARD_COLS)

#define SHRINK(cr, x) cairo_translate (cr, (1-(x))/2.0, (1-(x))/2.0); cairo_scale (cr, (x), (x))

// FIXME: g_array_free(effects, TRUE); 


struct _effects_widget effects_widget;
typedef struct _gsteffects gsteffects;

struct _gsteffects
{
  gchar *name;
  gchar *effect;
  gchar *filename;
  gboolean selected;
  gboolean is_black;
  gboolean needs_csp;
};

struct _effects_widget
{
  GArray *effects;
  gchar *used_effect;
};

static void paint (GtkWidget *widget, GdkEventExpose *eev, gpointer self);
static void draw_card (cairo_t *cr, gsteffects *card, int highlight);
static gboolean event_press(GtkWidget *widget, GdkEventButton *bev, gpointer self);

gchar *
get_selection() {

  int i;
  gchar *effect = NULL;
  for (i= 0; i < MAX_EFFECTS; i++) {
    if (g_array_index(effects_widget.effects, gsteffects, i).selected) {
      if (effect != NULL)
        if (g_array_index(effects_widget.effects, gsteffects, i).needs_csp)
          effect = g_strjoin(" ! ffmpegcolorspace ! ", effect, g_array_index(effects_widget.effects, gsteffects, i).effect, NULL);
        else
          effect = g_strjoin(" ! ", effect, g_array_index(effects_widget.effects, gsteffects, i).effect, NULL);

      else
        effect = g_array_index(effects_widget.effects, gsteffects, i).effect, NULL;
    }
  }
  if (effect == NULL) {
    effect = "identity";
  }

  if (!g_ascii_strcasecmp(effect, effects_widget.used_effect)) {
    return NULL;
  }
  effects_widget.used_effect = effect;
  return effect;
}

static void
paint (GtkWidget *widget, GdkEventExpose *eev, gpointer self)
{
  gint width, height;
  gint i;
  cairo_t *cr;

  width  = widget->allocation.width;
  height = widget->allocation.height;

  cr = gdk_cairo_create (widget->window);

  cairo_save (cr);
  cairo_scale (cr, width, height);

  for (i = 0; i < MAX_EFFECTS; i++) {
    cairo_save (cr);
    cairo_translate (cr,
        1.0/BOARD_COLS * (i % BOARD_COLS),
        1.0/BOARD_ROWS * (i / BOARD_COLS));
    cairo_scale (cr, 1.0/BOARD_COLS, 1.0/BOARD_ROWS);
    draw_card (cr, &g_array_index(effects_widget.effects, gsteffects, i), 
        g_array_index(effects_widget.effects, gsteffects, i).selected);
    cairo_restore (cr);
  }

  cairo_restore (cr);
}


static gboolean
event_press(GtkWidget *widget, GdkEventButton *bev, gpointer self)
{
  int i;

  int col = (int) (bev->x / DEFAULT_WIDTH * BOARD_COLS);
  int row = (int) (bev->y / DEFAULT_HEIGHT * BOARD_ROWS);

  int slot = (row * BOARD_COLS + col);

  g_array_index(effects_widget.effects, gsteffects, slot).selected = ! g_array_index(effects_widget.effects, gsteffects, slot).selected;
  printf("Slot %d selected (%f, %f, %d)\n", slot, bev->x, bev->y, g_array_index(effects_widget.effects, gsteffects, slot).selected);

  if (g_array_index(effects_widget.effects, gsteffects, 0).selected == TRUE)
  {
    for (i = 0; i < MAX_EFFECTS; i++)
      g_array_index(effects_widget.effects, gsteffects, i).selected = FALSE;
  }

  gtk_widget_queue_draw(widget);

  return TRUE;
}

void
effects_widget_init()
{
  effects_widget.used_effect = "identity";

  cheese_window.widgets.effects_widget = gtk_drawing_area_new();
  gtk_widget_set_size_request(cheese_window.widgets.effects_widget, DEFAULT_WIDTH, DEFAULT_HEIGHT);

  gtk_widget_add_events(cheese_window.widgets.effects_widget, GDK_BUTTON_PRESS_MASK);

  g_signal_connect(G_OBJECT (cheese_window.widgets.effects_widget), "button_press_event",
      G_CALLBACK (event_press), NULL);
  g_signal_connect(G_OBJECT (cheese_window.widgets.effects_widget), "expose-event",
      G_CALLBACK (paint), NULL);

  effects_widget.effects = g_array_new(TRUE, FALSE, sizeof(gsteffects));

  gsteffects g[] = {
    { _("No Effect"),        "identity",
        CHEESE_DATA_DIR"/effects/identity.png",
        FALSE, FALSE, FALSE},
    { _("Mauve"),            "videobalance saturation=1.5 hue=+0.5",
        CHEESE_DATA_DIR"/effects/Mauve.png",
        FALSE, FALSE, FALSE},
    { _("Noir/Blanc"),       "videobalance saturation=0",
        CHEESE_DATA_DIR"/effects/NoirBlanc.png",
        FALSE, FALSE, FALSE},
    { _("Saturation"),       "videobalance saturation=2",
        CHEESE_DATA_DIR"/effects/Saturation.png",
        FALSE, FALSE, FALSE},
    { _("Hulk"),             "videobalance saturation=1.5 hue=-0.5",
        CHEESE_DATA_DIR"/effects/Hulk.png",
        FALSE, FALSE, FALSE},
    { _("Vertical Flip"),    "videoflip method=4",
        CHEESE_DATA_DIR"/effects/videoflip_v.png",
        FALSE, FALSE, FALSE},
    { _("Horizontal Flip"),  "videoflip method=5",
        CHEESE_DATA_DIR"/effects/videoflip_h.png",
        FALSE, FALSE, FALSE},
    { _("Shagadelic"),       "shagadelictv",
        CHEESE_DATA_DIR"/effects/shagadelictv.png",
        FALSE, FALSE, TRUE},
    { _("Vertigo"),          "vertigotv",
        CHEESE_DATA_DIR"/effects/vertigotv.png",
        FALSE, FALSE, TRUE},
    { _("Edge"),             "edgetv",
        CHEESE_DATA_DIR"/effects/edgetv.png",
        FALSE, TRUE,  TRUE},
    { _("Dice"),             "dicetv",
        CHEESE_DATA_DIR"/effects/dicetv.png",
        FALSE, FALSE, TRUE},
    { _("Warp"),             "warptv",
        CHEESE_DATA_DIR"/effects/warptv.png",
        FALSE, FALSE, TRUE}};

  effects_widget.effects = g_array_append_vals(effects_widget.effects, (gconstpointer)g, MAX_EFFECTS);
}

static void
draw_card (cairo_t *cr, gsteffects *card, int highlight)
{
  static const double border_width = .02;

  cairo_save (cr);

  SHRINK (cr, .9);

  cairo_rectangle_round(cr, 0, 0, 1.0, 1.0, 0.2);
  //cairo_rectangle (cr, 0, 0, 1.0, 1.0);
  cairo_set_source_rgb (cr, 0, 0, 0);
  cairo_set_line_width (cr, border_width);
  cairo_stroke (cr);

  //SHRINK (cr, 1-border_width);
  cairo_save (cr);

  int w, h;
  cairo_surface_t *image;
  cairo_rectangle_round(cr, 0, 0, 1.0, 1.0, 0.2);
  cairo_clip (cr);
  cairo_new_path (cr);

  image = cairo_image_surface_create_from_png (card->filename);
  w = cairo_image_surface_get_width (image);
  h = cairo_image_surface_get_height (image);

  cairo_scale (cr, 1.0/w, 1.0/h);

  cairo_set_source_surface (cr, image, 0, 0);
  cairo_paint (cr);

  cairo_surface_destroy (image);
  cairo_restore (cr);


  cairo_text_extents_t extents;
  if (card->is_black)
    cairo_set_source_rgb (cr, 1, 1, 1);
  double x,y;
  cairo_select_font_face (cr, "Sans",
      CAIRO_FONT_SLANT_NORMAL,
      CAIRO_FONT_WEIGHT_NORMAL);

  cairo_set_font_size (cr, 0.09);
  cairo_text_extents (cr, card->name, &extents);
  x = 0.5 - (extents.width / 2 + extents.x_bearing);
  y = 0.95 - (extents.height / 2 + extents.y_bearing);

  cairo_move_to(cr, x, y);
  cairo_show_text(cr, card->name);

  if (highlight) {
    cairo_rectangle_round(cr, 0, 0, 1.0, 1.0, 0.2);
    //cairo_rectangle (cr, 0, 0, 1.0, 1.0);
    cairo_set_source_rgba (cr, 0, 0, 1, 0.25);
    cairo_fill (cr);
  }

  cairo_restore (cr);
}

