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

#include "cheese-config.h"

#include <glib/gi18n.h>

#include "cheese-widget.h"
#include "cheese-camera.h"
#include "cheese-enum-types.h"

enum
{
  READY_SIGNAL,
  ERROR_SIGNAL,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_STATE
};

enum
{
  SPINNER_PAGE = 0,
  WEBCAM_PAGE  = 1,
  PROBLEM_PAGE = 2,
};

typedef struct
{
  GtkWidget *spinner;
  GtkWidget *screen;
  ClutterActor *texture;
  GtkWidget *problem;
  GSettings *settings;
  CheeseCamera *webcam;
  CheeseWidgetState state;
  GError *error;
} CheeseWidgetPrivate;

#define CHEESE_WIDGET_GET_PRIVATE(o)                     \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHEESE_TYPE_WIDGET, \
                                CheeseWidgetPrivate))

G_DEFINE_TYPE (CheeseWidget, cheese_widget, GTK_TYPE_NOTEBOOK);

static GdkPixbuf *
cheese_widget_load_pixbuf (GtkWidget  *widget,
                           const char *icon_name,
                           guint       size,
                           GError    **error)
{
  GtkIconTheme *theme;
  GdkPixbuf    *pixbuf;

  theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (widget));

  /* FIXME special case "no-webcam" and actually use the icon_name */
  pixbuf = gtk_icon_theme_load_icon (theme, "dialog-error",
                                     size, GTK_ICON_LOOKUP_USE_BUILTIN | GTK_ICON_LOOKUP_FORCE_SIZE, error);
  return pixbuf;
}

static gboolean
cheese_widget_logo_draw (GtkWidget      *w,
                         cairo_t        *cr,
                         gpointer        user_data)
{
  const char   *icon_name;
  GdkPixbuf    *pixbuf, *logo;
  GError       *error = NULL;
  GtkAllocation allocation;
  guint         s_width, s_height, d_width, d_height;
  float         ratio;

  gtk_widget_get_allocation (w, &allocation);

  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);

  icon_name = g_object_get_data (G_OBJECT (w), "icon-name");
  if (icon_name == NULL) {
    cairo_paint (cr);
    return FALSE;
  }

  cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);

  d_width  = allocation.width;
  d_height = allocation.height - (allocation.height / 3);

  pixbuf = cheese_widget_load_pixbuf (w, icon_name, d_height, &error);
  if (pixbuf == NULL)
  {
    g_warning ("Could not load icon '%s': %s", icon_name, error->message);
    g_error_free (error);
    return FALSE;
  }

  s_width  = gdk_pixbuf_get_width (pixbuf);
  s_height = gdk_pixbuf_get_height (pixbuf);

  if ((gfloat) d_width / s_width > (gfloat) d_height / s_height)
  {
    ratio = (gfloat) d_height / s_height;
  }
  else
  {
    ratio = (gfloat) d_width / s_width;
  }

  s_width  *= ratio;
  s_height *= ratio;

  logo = gdk_pixbuf_scale_simple (pixbuf, s_width, s_height, GDK_INTERP_BILINEAR);

  gdk_cairo_set_source_pixbuf (cr, logo, (allocation.width - s_width) / 2, (allocation.height - s_height) / 2);
  cairo_paint (cr);

  g_object_unref (logo);
  g_object_unref (pixbuf);

  return FALSE;
}

static void
cheese_widget_spinner_invert (GtkWidget *spinner, GtkWidget *parent)
{
  GtkStyle *style;
  guint     i;

  for (i = GTK_STATE_NORMAL; i <= GTK_STATE_INSENSITIVE; i++)
  {
    GdkColor *fg, *bg;

    style = gtk_widget_get_style (spinner);
    fg    = gdk_color_copy (&style->fg[i]);
    bg    = gdk_color_copy (&style->bg[i]);

    gtk_widget_modify_fg (spinner, i, bg);
    gtk_widget_modify_bg (spinner, i, fg);

    gtk_widget_modify_fg (parent, i, bg);
    gtk_widget_modify_bg (parent, i, fg);

    gdk_color_free (fg);
    gdk_color_free (bg);
  }
}

static void
cheese_widget_set_problem_page (CheeseWidget *widget,
                                const char   *icon_name)
{
  CheeseWidgetPrivate *priv = CHEESE_WIDGET_GET_PRIVATE (widget);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (widget), PROBLEM_PAGE);
  g_object_set_data_full (G_OBJECT (priv->problem),
                          "icon-name", g_strdup (icon_name), g_free);
  g_signal_connect (priv->problem, "draw",
                    G_CALLBACK (cheese_widget_logo_draw), widget);
}

static void
cheese_widget_init (CheeseWidget *widget)
{
  CheeseWidgetPrivate *priv = CHEESE_WIDGET_GET_PRIVATE (widget);
  GtkWidget           *box;
  ClutterActor        *stage;

  priv->state = CHEESE_WIDGET_STATE_NONE;
  priv->error = NULL;

  /* XXX
   * remove this line if you want to debug */
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (widget), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (widget), FALSE);

  /* Spinner page */
  priv->spinner = gtk_spinner_new ();
  box           = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (box), priv->spinner);
  cheese_widget_spinner_invert (priv->spinner, box);
  gtk_widget_show_all (box);

  gtk_notebook_append_page (GTK_NOTEBOOK (widget),
                            box, gtk_label_new ("spinner"));

  /* Webcam page */
  priv->screen = gtk_clutter_embed_new ();
  stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (priv->screen));
  priv->texture = clutter_texture_new ();

  clutter_actor_set_size (priv->texture, 400, 300);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), priv->texture);

  gtk_widget_show (priv->screen);
  clutter_actor_show (priv->texture);
  gtk_notebook_append_page (GTK_NOTEBOOK (widget),
                            priv->screen, gtk_label_new ("webcam"));

  /* Problem page */
  priv->problem = gtk_drawing_area_new ();
  gtk_widget_show (priv->problem);
  gtk_notebook_append_page (GTK_NOTEBOOK (widget),
                            priv->problem,
                            gtk_label_new ("got problems"));

  priv->settings = g_settings_new ("org.gnome.Cheese");
}

static void
cheese_widget_finalize (GObject *object)
{
  CheeseWidgetPrivate *priv = CHEESE_WIDGET_GET_PRIVATE (object);

  if (priv->settings)
  {
    g_object_unref (priv->settings);
    priv->settings = NULL;
  }
  if (priv->webcam)
  {
    g_object_unref (priv->webcam);
    priv->webcam = NULL;
  }

  G_OBJECT_CLASS (cheese_widget_parent_class)->finalize (object);
}

#if 0
static void
cheese_widget_set_property (GObject *object, guint prop_id,
                            const GValue *value,
                            GParamSpec *pspec)
{
  CheeseWidgetPrivate *priv = CHEESE_WIDGET_GET_PRIVATE (object);

  g_return_if_fail (CHEESE_IS_WIDGET (object));

  switch (prop_id)
  {
    case PROP_STATE:
      priv->state = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

#endif

static void
cheese_widget_get_property (GObject *object, guint prop_id,
                            GValue *value, GParamSpec *pspec)
{
  CheeseWidgetPrivate *priv = CHEESE_WIDGET_GET_PRIVATE (object);

  g_return_if_fail (CHEESE_IS_WIDGET (object));

  switch (prop_id)
  {
    case PROP_STATE:
      g_value_set_enum (value, priv->state);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

#if 0
static void
cheese_widget_changed (CheeseWidget *self)
{
}

#endif

void
setup_camera (CheeseWidget *widget)
{
  CheeseWidgetPrivate *priv          = CHEESE_WIDGET_GET_PRIVATE (widget);
  char                *webcam_device = NULL;
  int                  x_resolution;
  int                  y_resolution;
  gdouble              brightness;
  gdouble              contrast;
  gdouble              saturation;
  gdouble              hue;

  g_settings_get (priv->settings, "photo-x-resolution", "i", &x_resolution);
  g_settings_get (priv->settings, "photo-y-resolution", "i", &y_resolution);
  g_settings_get (priv->settings, "camera",       "s", &webcam_device);
  g_settings_get (priv->settings, "brightness",   "d", &brightness);
  g_settings_get (priv->settings, "contrast",     "d", &contrast);
  g_settings_get (priv->settings, "saturation",   "d", &saturation);
  g_settings_get (priv->settings, "hue",          "d", &hue);

  gdk_threads_enter ();
  priv->webcam = cheese_camera_new (CLUTTER_TEXTURE (priv->texture),
                                    webcam_device,
                                    x_resolution,
                                    y_resolution);
  gdk_threads_leave ();

  g_free (webcam_device);

  cheese_camera_setup (priv->webcam, NULL, &priv->error);

  gdk_threads_enter ();

  gtk_spinner_stop (GTK_SPINNER (priv->spinner));

  if (priv->error != NULL)
  {
    priv->state = CHEESE_WIDGET_STATE_ERROR;
    g_object_notify (G_OBJECT (widget), "state");
    cheese_widget_set_problem_page (CHEESE_WIDGET (widget), "error");
  }
  else
  {
    cheese_camera_set_balance_property (priv->webcam, "brightness", brightness);
    cheese_camera_set_balance_property (priv->webcam, "contrast", contrast);
    cheese_camera_set_balance_property (priv->webcam, "saturation", saturation);
    cheese_camera_set_balance_property (priv->webcam, "hue", hue);
    priv->state = CHEESE_WIDGET_STATE_READY;
    g_object_notify (G_OBJECT (widget), "state");
    cheese_camera_play (priv->webcam);
    gtk_notebook_set_current_page (GTK_NOTEBOOK (widget), WEBCAM_PAGE);
  }

  gdk_threads_leave ();
}

static void
cheese_widget_realize (GtkWidget *widget)
{
  GdkWindow           *window;
  CheeseWidgetPrivate *priv = CHEESE_WIDGET_GET_PRIVATE (widget);

  GTK_WIDGET_CLASS (cheese_widget_parent_class)->realize (widget);

  gtk_spinner_start (GTK_SPINNER (priv->spinner));

  gtk_widget_realize (priv->screen);
  window = gtk_widget_get_window (priv->screen);
  if (!gdk_window_ensure_native (window))
  {
    /* abort: no native window, no xoverlay, no cheese. */
    g_warning ("Could not create a native X11 window for the drawing area");
    goto error;
  }

  gtk_widget_set_app_paintable (priv->screen, TRUE);
  gtk_widget_set_double_buffered (priv->screen, FALSE);

  if (!g_thread_create ((GThreadFunc) setup_camera, widget, FALSE, &priv->error))
  {
    g_warning ("Failed to create setup thread: %s", priv->error->message);
    goto error;
  }

  gtk_widget_set_app_paintable (priv->problem, TRUE);
  gtk_widget_realize (priv->problem);

  return;

error:
  gtk_spinner_stop (GTK_SPINNER (priv->spinner));
  priv->state = CHEESE_WIDGET_STATE_ERROR;
  g_object_notify (G_OBJECT (widget), "state");
  cheese_widget_set_problem_page (CHEESE_WIDGET (widget), "error");
}

static void
cheese_widget_class_init (CheeseWidgetClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = cheese_widget_finalize;
#if 0
  object_class->set_property = cheese_widget_set_property;
#endif
  object_class->get_property = cheese_widget_get_property;
  widget_class->realize      = cheese_widget_realize;

  /**
   * CheeseWidget:state:
   *
   * Current state of the widget.
   *
   * Connect to notify::state signal to get notified about state
   * changes. Useful to update other widgets sensitiveness when the
   * camera is ready or to handle errors if camera setup fails.
   */
  g_object_class_install_property (object_class, PROP_STATE,
                                   g_param_spec_enum ("state",
                                                      NULL,
                                                      NULL,
                                                      CHEESE_TYPE_WIDGET_STATE,
                                                      CHEESE_WIDGET_STATE_NONE,
                                                      G_PARAM_READABLE));

  g_type_class_add_private (klass, sizeof (CheeseWidgetPrivate));
}

/**
 * cheese_widget_new:
 *
 * Returns a new #CheeseWidget widget.
 *
 * Return value: a #CheeseWidget widget.
 **/
GtkWidget *
cheese_widget_new (void)
{
  return g_object_new (CHEESE_TYPE_WIDGET, NULL);
}

GSettings *
cheese_widget_get_settings (CheeseWidget *widget)
{
  CheeseWidgetPrivate *priv;

  g_return_val_if_fail (CHEESE_WIDGET (widget), NULL);

  priv = CHEESE_WIDGET_GET_PRIVATE (widget);

  return priv->settings;
}

GObject *
cheese_widget_get_camera (CheeseWidget *widget)
{
  CheeseWidgetPrivate *priv;

  g_return_val_if_fail (CHEESE_WIDGET (widget), NULL);

  priv = CHEESE_WIDGET_GET_PRIVATE (widget);

  return G_OBJECT (priv->webcam);
}

GtkWidget *
cheese_widget_get_video_area (CheeseWidget *widget)
{
  CheeseWidgetPrivate *priv;

  g_return_val_if_fail (CHEESE_WIDGET (widget), NULL);

  priv = CHEESE_WIDGET_GET_PRIVATE (widget);

  return priv->screen;
}

/**
 * cheese_widget_get_error:
 * @widget: a #CheeseWidget
 * @error: return location for the error
 *
 * Listen for notify::state signals and call this when the current state is %CHEESE_WIDGET_STATE_ERROR.
 *
 * The returned #GError will contain more details on what went wrong.
 **/

void
cheese_widget_get_error (CheeseWidget *widget, GError **error)
{
  CheeseWidgetPrivate *priv;

  g_return_if_fail (CHEESE_WIDGET (widget));

  priv = CHEESE_WIDGET_GET_PRIVATE (widget);

  g_propagate_error (error, priv->error);

  priv->error = NULL;
}

/*
 * vim: sw=2 ts=8 cindent noai bs=2
 */
