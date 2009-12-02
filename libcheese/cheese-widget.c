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

#include "cheese-widget.h"
#include "cheese-gconf.h"
#include "cheese-camera.h"

enum
{
  CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_WIDGET
};

enum {
	SPINNER_PAGE = 0,
	WEBCAM_PAGE  = 1,
	PROBLEM_PAGE = 2,
};

static guint widget_signals[LAST_SIGNAL] = {0};

typedef struct
{
  GtkWidget *spinner;
  GtkWidget *screen;
  GtkWidget *problem;
  CheeseGConf *gconf;
  CheeseCamera *webcam;
} CheeseWidgetPrivate;

#define CHEESE_WIDGET_GET_PRIVATE(o)                     \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHEESE_TYPE_WIDGET, \
                                CheeseWidgetPrivate))

G_DEFINE_TYPE (CheeseWidget, cheese_widget, GTK_TYPE_NOTEBOOK);

static GdkPixbuf *
cheese_widget_load_pixbuf (GtkWidget *widget,
			   const char *icon_name,
			   guint size,
			   GError **error)
{
  GtkIconTheme *theme;
  GdkPixbuf *pixbuf;

  theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (widget));
  //FIXME special case "no-webcam" and actually use the icon_name
  pixbuf = gtk_icon_theme_load_icon (theme, "error",
				     size, 0, error);
  return pixbuf;
}

static gboolean
cheese_widget_logo_expose (GtkWidget *w,
			   GdkEventExpose *event,
			   gpointer user_data)
{
  const char *icon_name;
  GdkPixbuf *pixbuf, *logo;
  GError *error = NULL;
  cairo_t *cr;
  GtkAllocation allocation;
  guint s_width, s_height, d_width, d_height;
  float ratio;

  gdk_draw_rectangle (w->window, w->style->black_gc, TRUE,
                      0, 0, w->allocation.width, w->allocation.height);
  icon_name = g_object_get_data (G_OBJECT (w), "icon-name");
  if (icon_name == NULL)
    return FALSE;

  cr = gdk_cairo_create (gtk_widget_get_window (w));
  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
  gtk_widget_get_allocation (w, &allocation);
  cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);

  d_width = allocation.width;
  d_height = allocation.height - (allocation.height / 3);

  pixbuf = cheese_widget_load_pixbuf (w, icon_name, d_height, &error);
  if (pixbuf == NULL) {
    g_warning ("Could not load icon '%s': %s", icon_name, error->message);
    g_error_free (error);
    return FALSE;
  }

  s_width = gdk_pixbuf_get_width (pixbuf);
  s_height = gdk_pixbuf_get_height (pixbuf);

  if ((gfloat) d_width / s_width > (gfloat) d_height / s_height) {
    ratio = (gfloat) d_height / s_height;
  } else {
    ratio = (gfloat) d_width / s_width;
  }

  s_width *= ratio;
  s_height *= ratio;

  logo = gdk_pixbuf_scale_simple (pixbuf, s_width, s_height, GDK_INTERP_BILINEAR);

  gdk_cairo_set_source_pixbuf (cr, logo, (allocation.width - s_width) / 2, (allocation.height - s_height) / 2);
  cairo_paint (cr);
  cairo_destroy (cr);

  g_object_unref (logo);
  g_object_unref (pixbuf);

  return FALSE;
}

static void
cheese_widget_spinner_invert (GtkWidget *spinner, GtkWidget *parent)
{
  GtkStyle *style;
  guint i;

  for (i = GTK_STATE_NORMAL; i <= GTK_STATE_INSENSITIVE; i++) {
    GdkColor *fg, *bg;

    style = gtk_widget_get_style (spinner);
    fg = gdk_color_copy (&style->fg[i]);
    bg = gdk_color_copy (&style->bg[i]);

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
				const char *icon_name)
{
  CheeseWidgetPrivate *priv = CHEESE_WIDGET_GET_PRIVATE (widget);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (widget), PROBLEM_PAGE);
  g_object_set_data_full (G_OBJECT (priv->problem),
			  "icon-name", g_strdup (icon_name), g_free);
  g_signal_connect (priv->problem, "expose-event",
                    G_CALLBACK (cheese_widget_logo_expose), widget);
}

static void
cheese_widget_init (CheeseWidget *widget)
{
  CheeseWidgetPrivate *priv = CHEESE_WIDGET_GET_PRIVATE (widget);
  GtkWidget *box;

  /* XXX
   * remove this line if you want to debug */
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (widget), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (widget), FALSE);

  /* Spinner page */
  priv->spinner = gtk_spinner_new ();
  box = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (box), priv->spinner);
  cheese_widget_spinner_invert (priv->spinner, box);
  gtk_widget_show_all (box);

  gtk_notebook_append_page (GTK_NOTEBOOK (widget),
			    box, gtk_label_new ("spinner"));

  /* Webcam page */
  priv->screen = gtk_drawing_area_new ();
  gtk_notebook_append_page (GTK_NOTEBOOK (widget),
  			    priv->screen, gtk_label_new ("webcam"));

  /* Problem page */
  priv->problem = gtk_drawing_area_new ();
  gtk_notebook_append_page (GTK_NOTEBOOK (widget),
			    priv->problem,
			    gtk_label_new ("got problems"));

  priv->gconf = cheese_gconf_new ();
}

static void
cheese_widget_finalize (GObject *object)
{
  CheeseWidgetPrivate *priv = CHEESE_WIDGET_GET_PRIVATE (object);

  if (priv->gconf) {
  	  g_object_unref (priv->gconf);
  	  priv->gconf = NULL;
  }
  if (priv->webcam) {
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
    case PROP_WIDGET:
      priv->widget = GTK_WIDGET (g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
cheese_widget_get_property (GObject *object, guint prop_id,
                                  GValue *value, GParamSpec *pspec)
{
  CheeseWidgetPrivate *priv = CHEESE_WIDGET_GET_PRIVATE (object);

  g_return_if_fail (CHEESE_IS_WIDGET (object));

  switch (prop_id)
  {
    case PROP_WIDGET:
      g_value_set_object (value, priv->widget);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
#endif
#if 0
static void
cheese_widget_changed (CheeseWidget *self)
{
}
#endif

void
setup_camera (CheeseWidget *widget)
{
  CheeseWidgetPrivate *priv = CHEESE_WIDGET_GET_PRIVATE (widget);
  char      *webcam_device = NULL;
  int        x_resolution;
  int        y_resolution;
  gdouble    brightness;
  gdouble    contrast;
  gdouble    saturation;
  gdouble    hue;
  GError    *error = NULL;

  g_object_get (priv->gconf,
                "gconf_prop_x_resolution", &x_resolution,
                "gconf_prop_y_resolution", &y_resolution,
                "gconf_prop_camera", &webcam_device,
                "gconf_prop_brightness", &brightness,
                "gconf_prop_contrast", &contrast,
                "gconf_prop_saturation", &saturation,
                "gconf_prop_hue", &hue,
                NULL);

  gdk_threads_enter ();
  priv->webcam = cheese_camera_new (priv->screen,
                                             webcam_device, x_resolution,
                                             y_resolution);
  gdk_threads_leave ();

  g_free (webcam_device);

  cheese_camera_setup (priv->webcam, NULL, &error);
  if (error != NULL)
  {
    g_warning ("Error setting up webcam %s", error->message);
    cheese_widget_set_problem_page (CHEESE_WIDGET (widget), "no-webcam");
    return;
  }

  gdk_threads_enter ();
  gtk_spinner_stop (GTK_SPINNER (priv->spinner));

  if (cheese_camera_get_num_camera_devices (priv->webcam) == 0) {
    cheese_widget_set_problem_page (CHEESE_WIDGET (widget), "no-webcam");
  } else {
    cheese_camera_play (priv->webcam);
    gtk_notebook_set_current_page (GTK_NOTEBOOK (widget), WEBCAM_PAGE);
  }

  gdk_threads_leave ();
}

static void
cheese_widget_realize (GtkWidget *widget)
{
  GdkWindow *window;
  CheeseWidgetPrivate *priv = CHEESE_WIDGET_GET_PRIVATE (widget);
  GError *error = NULL;

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

  gdk_window_set_back_pixmap (gtk_widget_get_window (priv->screen), NULL, FALSE);
  gtk_widget_set_app_paintable (priv->screen, TRUE);
  gtk_widget_set_double_buffered (priv->screen, FALSE);

  if (!g_thread_create ((GThreadFunc) setup_camera, widget, FALSE, &error))
  {
    g_warning ("Failed to create setup thread: %s\n", error->message);
    g_error_free (error);
    goto error;
  }

  gtk_widget_set_app_paintable (priv->problem, TRUE);
  gtk_widget_realize (priv->problem);
  gdk_window_set_back_pixmap (gtk_widget_get_window (priv->problem), NULL, FALSE);

  return;

error:
  gtk_spinner_stop (GTK_SPINNER (priv->spinner));
  cheese_widget_set_problem_page (CHEESE_WIDGET (widget), "error");
}

static void
cheese_widget_class_init (CheeseWidgetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize     = cheese_widget_finalize;
//  object_class->set_property = cheese_widget_set_property;
//  object_class->get_property = cheese_widget_get_property;

//  klass->changed     = cheese_widget_changed;
//  klass->synchronize = NULL;

  widget_class->realize      = cheese_widget_realize;

  g_type_class_add_private (klass, sizeof (CheeseWidgetPrivate));
}

#if 0
void
cheese_widget_synchronize (CheeseWidget *widget)
{
  CHEESE_WIDGET_GET_CLASS (widget)->synchronize (widget);
}
#endif
#if 0
void
cheese_widget_notify_changed (CheeseWidget *widget)
{
  g_signal_emit_by_name (widget, "changed");
}
#endif

GtkWidget *
cheese_widget_new (void)
{
	return g_object_new (CHEESE_TYPE_WIDGET, NULL);
}

