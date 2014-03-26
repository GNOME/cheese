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

#include <glib/gi18n-lib.h>
#include <clutter-gst/clutter-gst.h>

#include "cheese-widget.h"
#include "cheese-widget-private.h"
#include "cheese-camera.h"
#include "cheese-enums.h"
#include "totem-aspect-frame.h"

/**
 * SECTION:cheese-widget
 * @short_description: A photo/video capture widget
 * @stability: Unstable
 * @include: cheese/cheese-widget.h
 *
 * #CheeseWidget provides a basic photo and video capture widget, for embedding
 * in an application.
 */

enum
{
  READY_SIGNAL,
  ERROR_SIGNAL,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_STATE,
  PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

struct _CheeseWidgetPrivate
{
  GtkWidget *spinner;
  GtkWidget *screen;
  ClutterActor *texture;
  GtkWidget *problem;
  GSettings *settings;
  CheeseCamera *webcam;
  CheeseWidgetState state;
  GError *error;
};

G_DEFINE_TYPE_WITH_PRIVATE (CheeseWidget, cheese_widget, GTK_TYPE_NOTEBOOK);

void setup_camera (CheeseWidget *widget);

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
  GtkStyleContext *context;
  guint     i;

  for (i = GTK_STATE_NORMAL; i <= GTK_STATE_INSENSITIVE; i++)
  {
    GdkRGBA fg, bg;

    context = gtk_widget_get_style_context (spinner);
    gtk_style_context_get_color (context, gtk_style_context_get_state (context), &fg);
    gtk_style_context_get_background_color (context, gtk_style_context_get_state (context), &bg);

    gtk_widget_override_color (spinner, i, &bg);
    gtk_widget_override_background_color (spinner, i, &fg);

    gtk_widget_override_color (parent, i, &bg);
    gtk_widget_override_background_color (parent, i, &fg);
  }
}

static void
cheese_widget_set_problem_page (CheeseWidget *widget,
                                const char   *icon_name)
{
  CheeseWidgetPrivate *priv = widget->priv;

    priv->state = CHEESE_WIDGET_STATE_ERROR;
    g_object_notify_by_pspec (G_OBJECT (widget), properties[PROP_STATE]);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (widget), PROBLEM_PAGE);
  g_object_set_data_full (G_OBJECT (priv->problem),
                          "icon-name", g_strdup (icon_name), g_free);
  g_signal_connect (priv->problem, "draw",
                    G_CALLBACK (cheese_widget_logo_draw), widget);
}

static void
cheese_widget_init (CheeseWidget *widget)
{
  CheeseWidgetPrivate *priv = widget->priv = cheese_widget_get_instance_private (widget);
  GtkWidget           *box;
  ClutterActor        *stage, *frame;
  ClutterColor black = { 0x00, 0x00, 0x00, 0xff };

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
  gtk_widget_set_size_request (priv->screen, 460, 345);
  stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (priv->screen));
  clutter_actor_set_background_color (stage, &black);
  frame = totem_aspect_frame_new ();

  priv->texture = clutter_texture_new ();
  totem_aspect_frame_set_child (TOTEM_ASPECT_FRAME (frame), priv->texture);

  clutter_actor_set_layout_manager (stage, clutter_bin_layout_new (CLUTTER_BIN_ALIGNMENT_FILL, CLUTTER_BIN_ALIGNMENT_FILL));
  clutter_actor_add_child (stage, frame);

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
  CheeseWidgetPrivate *priv = ((CheeseWidget *) object)->priv;

  g_clear_object (&priv->settings);
  g_clear_object (&priv->webcam);

  G_OBJECT_CLASS (cheese_widget_parent_class)->finalize (object);
}

static void
cheese_widget_get_property (GObject *object, guint prop_id,
                            GValue *value, GParamSpec *pspec)
{
  CheeseWidgetPrivate *priv = ((CheeseWidget *) object)->priv;

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

/*
 * webcam_state_changed:
 * @camera: the camera on which there was a state change
 * @state: the new state
 * @widget: the widget which should be updated
 *
 * Handle the state of the @camera changing, and update @widget accordingly,
 * such as by displaying an error.
 */
static void
webcam_state_changed (CheeseCamera *camera, GstState state,
                      CheeseWidget *widget)
{
    switch (state)
    {
        case GST_STATE_NULL:
            cheese_widget_set_problem_page (widget, "error");
            break;
        default:
            /* TODO: Handle other cases. */
            break;
    }
}

void
setup_camera (CheeseWidget *widget)
{
  CheeseWidgetPrivate *priv = widget->priv;
  gchar *webcam_device = NULL;
  gint x_resolution;
  gint y_resolution;
  gdouble brightness;
  gdouble contrast;
  gdouble saturation;
  gdouble hue;

  g_settings_get (priv->settings, "photo-x-resolution", "i", &x_resolution);
  g_settings_get (priv->settings, "photo-y-resolution", "i", &y_resolution);
  g_settings_get (priv->settings, "camera",       "s", &webcam_device);
  g_settings_get (priv->settings, "brightness",   "d", &brightness);
  g_settings_get (priv->settings, "contrast",     "d", &contrast);
  g_settings_get (priv->settings, "saturation",   "d", &saturation);
  g_settings_get (priv->settings, "hue",          "d", &hue);

  priv->webcam = cheese_camera_new (CLUTTER_TEXTURE (priv->texture),
                                    webcam_device,
                                    x_resolution,
                                    y_resolution);

  g_free (webcam_device);

  cheese_camera_setup (priv->webcam, NULL, &priv->error);

  gtk_spinner_stop (GTK_SPINNER (priv->spinner));

  if (priv->error != NULL)
  {
    cheese_widget_set_problem_page (CHEESE_WIDGET (widget), "error");
  }
  else
  {
    cheese_camera_set_balance_property (priv->webcam, "brightness", brightness);
    cheese_camera_set_balance_property (priv->webcam, "contrast", contrast);
    cheese_camera_set_balance_property (priv->webcam, "saturation", saturation);
    cheese_camera_set_balance_property (priv->webcam, "hue", hue);
    priv->state = CHEESE_WIDGET_STATE_READY;
    g_object_notify_by_pspec (G_OBJECT (widget), properties[PROP_STATE]);
    g_signal_connect (priv->webcam, "state-flags-changed",
                      G_CALLBACK (webcam_state_changed), widget);
    cheese_camera_play (priv->webcam);
    gtk_notebook_set_current_page (GTK_NOTEBOOK (widget), WEBCAM_PAGE);
  }
}

static void
cheese_widget_realize (GtkWidget *widget)
{
  CheeseWidgetPrivate *priv = ((CheeseWidget *) widget)->priv;

  GTK_WIDGET_CLASS (cheese_widget_parent_class)->realize (widget);

  gtk_spinner_start (GTK_SPINNER (priv->spinner));

  gtk_widget_realize (priv->screen);

  setup_camera (CHEESE_WIDGET (widget));

  gtk_widget_set_app_paintable (priv->problem, TRUE);
  gtk_widget_realize (priv->problem);

  return;
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
   * Connect to notify::state signal to get notified about state changes.
   * Useful to update other widgets sensitivities when the camera is ready or
   * to handle errors if camera setup fails.
   */
  properties[PROP_STATE] = g_param_spec_enum ("state",
                                              "State",
                                              "The current state of the widget",
                                              CHEESE_TYPE_WIDGET_STATE,
                                              CHEESE_WIDGET_STATE_NONE,
                                              G_PARAM_READABLE |
                                              G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST, properties);
}

/**
 * cheese_widget_new:
 *
 * Creates a new #CheeseWidget. Make sure that you call cheese_gtk_init(), and
 * check for errors during initialization, before calling this function.
 *
 * Returns: a new #CheeseWidget
 */
GtkWidget *
cheese_widget_new (void)
{
  return g_object_new (CHEESE_TYPE_WIDGET, NULL);
}

GSettings *
cheese_widget_get_settings (CheeseWidget *widget)
{
  g_return_val_if_fail (CHEESE_WIDGET (widget), NULL);

  return widget->priv->settings;
}

/*
 * cheese_widget_get_camera:
 * @widget: a #CheeseWidget
 *
 * Returns: a #CheeseCamera for this #CheeseWidget
 */
GObject *
cheese_widget_get_camera (CheeseWidget *widget)
{
  g_return_val_if_fail (CHEESE_WIDGET (widget), NULL);

  return G_OBJECT (widget->priv->webcam);
}

/*
 * cheese_widget_get_video_area:
 * @widget: a #CheeseWidget
 *
 * Returns: a #GtkClutterEmbed of the stream from the video capture device
 */
GtkWidget *
cheese_widget_get_video_area (CheeseWidget *widget)
{
  g_return_val_if_fail (CHEESE_WIDGET (widget), NULL);

  return widget->priv->screen;
}

/**
 * cheese_widget_get_error:
 * @widget: a #CheeseWidget
 * @error: return location for the error
 *
 * Listen for notify::state signals and call this when the current state is
 * %CHEESE_WIDGET_STATE_ERROR.
 *
 * The returned #GError will contain more details on what went wrong.
 */
void
cheese_widget_get_error (CheeseWidget *widget, GError **error)
{
  CheeseWidgetPrivate *priv;

  g_return_if_fail (CHEESE_WIDGET (widget));

  priv = (widget)->priv;

  g_propagate_error (error, priv->error);

  priv->error = NULL;
}
