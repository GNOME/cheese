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

#include <gst/interfaces/xoverlay.h>
#include <glib.h>

#include <gtk/gtk.h>
#include <cairo.h>

#include <libintl.h>

#include "cheese.h"
#include "cheese_config.h"
#include "pipeline-photo.h"
#include "window.h"
#include "cairo-custom.h"

#define _(str) gettext(str)

#define DEFAULT_WIDTH  640
#define DEFAULT_HEIGHT 480
#define BOARD_COLS 4
#define BOARD_ROWS 3
#define MAX_EFFECTS (BOARD_ROWS * BOARD_COLS)

#define SHRINK(cr, x) cairo_translate (cr, (1-(x))/2.0, (1-(x))/2.0); cairo_scale (cr, (x), (x))

G_DEFINE_TYPE (Pipeline, pipeline, G_TYPE_OBJECT)

static GObjectClass *parent_class = NULL;

typedef struct _PipelinePrivate PipelinePrivate;
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

struct _PipelinePrivate
{
  int picture_requested;

  GstElement *source;
  GstElement *ffmpeg1, *ffmpeg2, *ffmpeg3;
  GstElement *tee;
  GstElement *queuevid, *queueimg;
  GstElement *caps;
  GstElement *effect;

  GArray *effects;
  gchar *used_effect;
};

#define PIPELINE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), PIPELINE_TYPE, PipelinePrivate))

// private methods
static gboolean cb_have_data(GstElement *element, GstBuffer *buffer, GstPad *pad, gpointer user_data);
static void pipeline_lens_open(Pipeline *self);
static void pipeline_set_effect(Pipeline *self);

static void paint (GtkWidget *widget, GdkEventExpose *eev, gpointer self);
static void draw_card (cairo_t *cr, gsteffects *card, int highlight);
static gboolean event_press(GtkWidget *widget, GdkEventButton *bev, gpointer self);
GtkWidget * effects_new(gpointer self);

void
pipeline_set_play(Pipeline *self)
{
  gst_element_set_state(PIPELINE(self)->pipeline, GST_STATE_PLAYING);
  return;
}

void
pipeline_set_stop(Pipeline *self)
{
  gst_element_set_state(PIPELINE(self)->pipeline, GST_STATE_NULL);
  return;
}

void
pipeline_button_clicked(GtkWidget *widget, gpointer self)
{
  pipeline_lens_open(self);
  return;
}

static void
pipeline_lens_open(Pipeline *self)
{
  PipelinePrivate *priv = PIPELINE_GET_PRIVATE(self);
  priv->picture_requested = TRUE;
  return;
}

GstElement *
pipeline_get_ximagesink(Pipeline *self)
{
  return PIPELINE(self)->ximagesink;
}

GstElement *
pipeline_get_fakesink(Pipeline *self)
{
  return PIPELINE(self)->fakesink;
}

GstElement *
pipeline_get_pipeline(Pipeline *self)
{
  return PIPELINE(self)->pipeline;
}

static void
pipeline_set_effect(Pipeline *self)
{
  PipelinePrivate *priv = PIPELINE_GET_PRIVATE(self);

  int i;
  gchar *effect = NULL;
  for (i= 0; i < MAX_EFFECTS; i++) {
    if (g_array_index(priv->effects, gsteffects, i).selected) {
      if (effect != NULL)
        if (g_array_index(priv->effects, gsteffects, i).needs_csp)
          effect = g_strjoin(" ! ffmpegcolorspace ! ", effect, g_array_index(priv->effects, gsteffects, i).effect, NULL);
        else
          effect = g_strjoin(" ! ", effect, g_array_index(priv->effects, gsteffects, i).effect, NULL);

      else
        effect = g_array_index(priv->effects, gsteffects, i).effect, NULL;
    }
  }
  if (effect == NULL) {
    effect = "identity";
  }

  if (g_ascii_strcasecmp(effect, priv->used_effect)) {
    pipeline_set_stop(self);

    gst_element_unlink(priv->ffmpeg1, priv->effect);
    gst_element_unlink(priv->effect, priv->ffmpeg2);
    gst_bin_remove(GST_BIN(PIPELINE(self)->pipeline), priv->effect);

    printf("changing to effect: %s\n", effect);
    priv->effect = gst_parse_bin_from_description(effect, TRUE, NULL);
    gst_bin_add(GST_BIN(PIPELINE(self)->pipeline), priv->effect);

    gst_element_link(priv->ffmpeg1, priv->effect);
    gst_element_link(priv->effect, priv->ffmpeg2);

    pipeline_set_play(self);
    set_screen_x_window_id();
  }
  priv->used_effect = effect;
}

void
pipeline_change_effect(GtkWidget *widget, gpointer self)
{
  if (gtk_notebook_get_current_page(GTK_NOTEBOOK(cheese_window.widgets.notebook)) == 1) {
    gtk_notebook_set_current_page (GTK_NOTEBOOK(cheese_window.widgets.notebook), 0);
    gtk_label_set_text(GTK_LABEL(cheese_window.widgets.label_effects), _("Effects"));
    gtk_widget_set_sensitive(cheese_window.widgets.take_picture, TRUE);
    pipeline_set_effect(self);
  }
  else {
    if (cheese_window.widgets.effects_widget == NULL) {
      cheese_window.widgets.effects_widget = effects_new(self);
      gtk_notebook_append_page(GTK_NOTEBOOK(cheese_window.widgets.notebook), cheese_window.widgets.effects_widget,  gtk_label_new(NULL));
      gtk_widget_show(cheese_window.widgets.effects_widget);
    }
    gtk_notebook_set_current_page (GTK_NOTEBOOK(cheese_window.widgets.notebook), 1);
    gtk_label_set_text(GTK_LABEL(cheese_window.widgets.label_effects), _("Back"));
    gtk_widget_set_sensitive(GTK_WIDGET(cheese_window.widgets.take_picture), FALSE);
  }
}

void
pipeline_create(Pipeline *self) {

  GstCaps *filter;
  gboolean link_ok; 

  PipelinePrivate *priv = PIPELINE_GET_PRIVATE(self);

  PIPELINE(self)->pipeline = gst_pipeline_new("pipeline");
  priv->source = gst_element_factory_make("gconfvideosrc", "v4l2src");
  // if you want to test without having a webcam
  //source = gst_element_factory_make("videotestsrc", "v4l2src");

  gst_bin_add(GST_BIN(PIPELINE(self)->pipeline), priv->source);
  priv->ffmpeg1 = gst_element_factory_make("ffmpegcolorspace", "ffmpegcolorspace");
  gst_bin_add(GST_BIN(PIPELINE(self)->pipeline), priv->ffmpeg1);

  priv->effect = gst_element_factory_make(g_array_index(priv->effects, gsteffects, 0).effect, "effect");
  gst_bin_add(GST_BIN(PIPELINE(self)->pipeline), priv->effect);
  priv->used_effect = g_array_index(priv->effects, gsteffects, 0).effect;

  priv->ffmpeg2 = gst_element_factory_make("ffmpegcolorspace", "ffmpegcolorspace2");
  gst_bin_add(GST_BIN(PIPELINE(self)->pipeline), priv->ffmpeg2);
  priv->ffmpeg3 = gst_element_factory_make("ffmpegcolorspace", "ffmpegcolorspace3");
  gst_bin_add(GST_BIN(PIPELINE(self)->pipeline), priv->ffmpeg3);
  priv->queuevid = gst_element_factory_make("queue", "queuevid");
  gst_bin_add(GST_BIN(PIPELINE(self)->pipeline), priv->queuevid);
  priv->queueimg = gst_element_factory_make("queue", "queueimg");
  gst_bin_add(GST_BIN(PIPELINE(self)->pipeline), priv->queueimg);
  priv->caps = gst_element_factory_make("capsfilter", "capsfilter");
  gst_bin_add(GST_BIN(PIPELINE(self)->pipeline), priv->caps);
  PIPELINE(self)->ximagesink = gst_element_factory_make("gconfvideosink", "gconfvideosink");
  gst_bin_add(GST_BIN(PIPELINE(self)->pipeline), PIPELINE(self)->ximagesink);
  priv->tee = gst_element_factory_make("tee", "tee");
  gst_bin_add(GST_BIN(PIPELINE(self)->pipeline), priv->tee);
  PIPELINE(self)->fakesink = gst_element_factory_make("fakesink", "fakesink");
  gst_bin_add(GST_BIN(PIPELINE(self)->pipeline), PIPELINE(self)->fakesink);

  /*
   * the pipeline looks like this:
   * gconfvideosrc -> ffmpegcsp -> ffmpegcsp -> tee (filtered) -> queue-> ffmpegcsp -> gconfvideosink
   *                                             |
   *                                         queueimg -> fakesink -> pixbuf (gets picture from data)
   */

  gst_element_link(priv->source, priv->ffmpeg1);
  gst_element_link(priv->ffmpeg1, priv->effect);
  gst_element_link(priv->effect, priv->ffmpeg2);

  filter = gst_caps_new_simple("video/x-raw-rgb",
      "width", G_TYPE_INT, PHOTO_WIDTH,
      "height", G_TYPE_INT, PHOTO_HEIGHT,
      "framerate", GST_TYPE_FRACTION, 30, 1,
      "bpp", G_TYPE_INT, 24,
      "depth", G_TYPE_INT, 24, NULL);

  link_ok = gst_element_link_filtered(priv->ffmpeg2, priv->tee, filter);
  if (!link_ok) {
    g_warning("Failed to link elements!");
  }
  //FIXME: do we need this?
  //filter = gst_caps_new_simple("video/x-raw-yuv", NULL);

  gst_element_link(priv->tee, priv->queuevid);
  gst_element_link(priv->queuevid, priv->ffmpeg3);

  gst_element_link(priv->ffmpeg3, PIPELINE(self)->ximagesink);

  gst_element_link(priv->tee, priv->queueimg);
  gst_element_link(priv->queueimg, PIPELINE(self)->fakesink);
  g_object_set(G_OBJECT(PIPELINE(self)->fakesink), "signal-handoffs", TRUE, NULL);

  g_signal_connect(G_OBJECT(PIPELINE(self)->fakesink), "handoff", 
      G_CALLBACK(cb_have_data), self);

}

static gboolean
cb_have_data(GstElement *element, GstBuffer *buffer, GstPad *pad, gpointer self)
{
  PipelinePrivate *priv = PIPELINE_GET_PRIVATE(self);

  unsigned char *data_photo = (unsigned char *)GST_BUFFER_DATA(buffer);
  if (priv->picture_requested) {
    priv->picture_requested = FALSE;
    create_photo(data_photo);
  }
  return TRUE;
}

GtkWidget *
effects_new(gpointer self)
{
  GtkWidget     *canvas;

  canvas = gtk_drawing_area_new();
  gtk_widget_set_size_request(canvas, DEFAULT_WIDTH, DEFAULT_HEIGHT);

  g_signal_connect(G_OBJECT (canvas), "expose-event",
      G_CALLBACK (paint), self);

  gtk_widget_add_events(canvas, GDK_BUTTON_PRESS_MASK);

  g_signal_connect(G_OBJECT (canvas), "button_press_event",
      G_CALLBACK (event_press), self);

  return canvas;
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

static void
paint (GtkWidget *widget, GdkEventExpose *eev, gpointer self)
{
  PipelinePrivate *priv = PIPELINE_GET_PRIVATE(self);
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
    draw_card (cr, &g_array_index(priv->effects, gsteffects, i), 
        g_array_index(priv->effects, gsteffects, i).selected);
    cairo_restore (cr);
  }

  cairo_restore (cr);
}


static gboolean
event_press (GtkWidget *widget, GdkEventButton *bev, gpointer self)
{
  PipelinePrivate *priv = PIPELINE_GET_PRIVATE(self);
  int i;

  int col = (int) (bev->x / DEFAULT_WIDTH * BOARD_COLS);
  int row = (int) (bev->y / DEFAULT_HEIGHT * BOARD_ROWS);

  int slot = (row * BOARD_COLS + col);

  g_array_index(priv->effects, gsteffects, slot).selected = ! g_array_index(priv->effects, gsteffects, slot).selected;
  printf("Slot %d selected (%f, %f, %d)\n", slot, bev->x, bev->y, g_array_index(priv->effects, gsteffects, slot).selected);

  if (g_array_index(priv->effects, gsteffects, 0).selected == TRUE)
  {
    for (i = 0; i < MAX_EFFECTS; i++)
      g_array_index(priv->effects, gsteffects, i).selected = FALSE;
  }

  gtk_widget_queue_draw(widget);


  return TRUE;
}

void
pipeline_finalize(GObject *object)
{
  PipelinePrivate *priv = PIPELINE_GET_PRIVATE(object);
  g_array_free(priv->effects, TRUE); 

	(*parent_class->finalize) (object);

	return;
}

Pipeline *
pipeline_new(void)
{
	Pipeline *self = g_object_new(PIPELINE_TYPE, NULL);

	return self;
}

void
pipeline_class_init(PipelineClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent(klass);
	object_class = (GObjectClass*) klass;

	object_class->finalize = pipeline_finalize;
  g_type_class_add_private(klass, sizeof(PipelinePrivate));

	G_OBJECT_CLASS(klass)->finalize = (GObjectFinalizeFunc) pipeline_finalize;

}

void
pipeline_init(Pipeline *self)
{
  PipelinePrivate *priv = PIPELINE_GET_PRIVATE(self);

  priv->effects = g_array_new(TRUE, FALSE, sizeof(gsteffects));

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


  priv->effects = g_array_append_vals(priv->effects, (gconstpointer)g, MAX_EFFECTS);

}

