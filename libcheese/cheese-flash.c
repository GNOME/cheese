/*
 * Copyright © 2008 Alexander “weej” Jones <alex@weej.com>
 * Copyright © 2008 Thomas Perl <thp@thpinfo.com>
 * Copyright © 2009 daniel g. siegel <dgsiegel@gnome.org>
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

#include <gtk/gtk.h>

#include "cheese-camera.h"
#include "cheese-flash.h"

/**
 * SECTION:cheese-flash
 * @short_description: Flash the screen, like a real camera flash
 * @stability: Unstable
 * @include: cheese/cheese-flash.h
 *
 * #CheeseFlash is a window that you can create and invoke a method "flash" on
 * to temporarily flood the screen with white.
 */

enum
{
  PROP_0,
  PROP_PARENT,
  PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

/* How long to hold the flash for, in milliseconds. */
static const guint FLASH_DURATION = 250;

/* The factor which defines how much the flash fades per frame */
static const gdouble FLASH_FADE_FACTOR = 0.95;

/* How many frames per second */
static const guint FLASH_ANIMATION_RATE = 50;

/* When to consider the flash finished so we can stop fading */
static const gdouble FLASH_LOW_THRESHOLD = 0.01;

/*
 * CheeseFlashPrivate:
 * @parent: the parent #GtkWidget, for choosing on which display to fire the
 * flash
 * @flash_timeout_tag: signal ID of the timeout to start fading in the flash
 * @fade_timeout_tag: signal ID of the timeout to start fading out the flash
 *
 * Private data for #CheeseFlash.
 */
struct _CheeseFlashPrivate
{
  /*< private >*/
  GtkWidget *parent;
  guint flash_timeout_tag;
  guint fade_timeout_tag;
};

G_DEFINE_TYPE_WITH_PRIVATE (CheeseFlash, cheese_flash, GTK_TYPE_WINDOW)

static void
cheese_flash_init (CheeseFlash *self)
{
  CheeseFlashPrivate *priv = self->priv = cheese_flash_get_instance_private (self);
  cairo_region_t *input_region;
  GtkWindow *window = GTK_WINDOW (self);
  const GdkRGBA white = { 1.0, 1.0, 1.0, 1.0 };

  priv->flash_timeout_tag = 0;
  priv->fade_timeout_tag  = 0;

  /* make it so it doesn't look like a window on the desktop (+fullscreen) */
  gtk_window_set_decorated (window, FALSE);
  gtk_window_set_skip_taskbar_hint (window, TRUE);
  gtk_window_set_skip_pager_hint (window, TRUE);
  gtk_window_set_keep_above (window, TRUE);

  /* Don't take focus */
  gtk_window_set_accept_focus (window, FALSE);
  gtk_window_set_focus_on_map (window, FALSE);

  /* Make it white */
  gtk_widget_override_background_color (GTK_WIDGET (window), GTK_STATE_NORMAL,
                                        &white);

  /* Don't consume input */
  gtk_widget_realize (GTK_WIDGET (window));
  input_region = cairo_region_create ();
  gdk_window_input_shape_combine_region (gtk_widget_get_window (GTK_WIDGET (window)), input_region, 0, 0);
  cairo_region_destroy (input_region);
}

static void
cheese_flash_dispose (GObject *object)
{
  CheeseFlashPrivate *priv = CHEESE_FLASH (object)->priv;

  g_clear_object (&priv->parent);

  if (G_OBJECT_CLASS (cheese_flash_parent_class)->dispose)
    G_OBJECT_CLASS (cheese_flash_parent_class)->dispose (object);
}

static void
cheese_flash_finalize (GObject *object)
{
  if (G_OBJECT_CLASS (cheese_flash_parent_class)->finalize)
    G_OBJECT_CLASS (cheese_flash_parent_class)->finalize (object);
}

static void
cheese_flash_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  CheeseFlashPrivate *priv = CHEESE_FLASH (object)->priv;

  switch (prop_id)
  {
    case PROP_PARENT: {
      GObject *object;
      object = g_value_get_object (value);
      if (object != NULL)
        priv->parent = g_object_ref (object);
      else
        priv->parent = NULL;
    }
    break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
cheese_flash_class_init (CheeseFlashClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = cheese_flash_set_property;
  object_class->dispose      = cheese_flash_dispose;
  object_class->finalize     = cheese_flash_finalize;

  /**
   * CheeseFlash:parent:
   *
   * Parent #GtkWidget for the #CheeseFlash. The flash will be fired on the
   * screen where the parent widget is shown.
   */
  properties[PROP_PARENT] = g_param_spec_object ("parent",
                                                 "Parent widget",
                                                 "The flash will be fired on the screen where the parent widget is shown",
                                                 GTK_TYPE_WIDGET,
                                                 G_PARAM_WRITABLE |
                                                 G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST, properties);
}

/*
 * cheese_flash_opacity_fade:
 * @data: the #CheeseFlash
 *
 * Fade the flash out.
 *
 * Returns: %TRUE if the fade was completed, %FALSE if the flash must continue
 * to fade
 */
static gboolean
cheese_flash_opacity_fade (gpointer data)
{
  GtkWidget *flash_window = GTK_WIDGET (data);
  gdouble opacity = gtk_widget_get_opacity (flash_window);

  /* exponentially decrease */
  gtk_widget_set_opacity (flash_window, opacity * FLASH_FADE_FACTOR);

  if (opacity <= FLASH_LOW_THRESHOLD)
  {
    /* the flasher has finished when we reach the quit value */
    gtk_widget_hide (flash_window);
    return G_SOURCE_REMOVE;
  }

  return G_SOURCE_CONTINUE;
}

/*
 * cheese_flash_start_fade:
 * @data: the #CheeseFlash
 *
 * Add a timeout to start the fade animation.
 *
 * Returns: %FALSE
 */
static gboolean
cheese_flash_start_fade (gpointer data)
{
  CheeseFlashPrivate *flash_priv = CHEESE_FLASH (data)->priv;

  GtkWindow *flash_window = GTK_WINDOW (data);

  /* If the screen is non-composited, just hide and finish up */
  if (!gdk_screen_is_composited (gtk_window_get_screen (flash_window)))
  {
    gtk_widget_hide (GTK_WIDGET (flash_window));
    return G_SOURCE_REMOVE;
  }

  flash_priv->fade_timeout_tag = g_timeout_add (1000.0 / FLASH_ANIMATION_RATE, cheese_flash_opacity_fade, data);
  return G_SOURCE_REMOVE;
}

/**
 * cheese_flash_fire:
 * @flash: a #CheeseFlash
 *
 * Fire the flash.
 */
void
cheese_flash_fire (CheeseFlash *flash)
{
  CheeseFlashPrivate *flash_priv;
  GtkWidget          *parent;
  GdkScreen          *screen;
  GdkRectangle        rect, work_rect;
  int                 monitor;
  GtkWindow *flash_window;

  g_return_if_fail (CHEESE_IS_FLASH (flash));

  flash_priv = flash->priv;

  g_return_if_fail (flash_priv->parent != NULL);

  flash_window = GTK_WINDOW (flash);

  if (flash_priv->flash_timeout_tag > 0)
    g_source_remove (flash_priv->flash_timeout_tag);
  if (flash_priv->fade_timeout_tag > 0)
    g_source_remove (flash_priv->fade_timeout_tag);

  parent  = gtk_widget_get_toplevel (flash_priv->parent);
  screen  = gtk_widget_get_screen (parent);
  monitor = gdk_screen_get_monitor_at_window (screen,
					      gtk_widget_get_window (parent));

  gdk_screen_get_monitor_geometry (screen, monitor, &rect);
  gdk_screen_get_monitor_workarea (screen, monitor, &work_rect);
  gdk_rectangle_intersect (&work_rect, &rect, &rect);

  gtk_window_set_transient_for (GTK_WINDOW (flash_window), GTK_WINDOW (parent));
  gtk_window_resize (flash_window, rect.width, rect.height);
  gtk_window_move (flash_window, rect.x, rect.y);

  gtk_widget_set_opacity (GTK_WIDGET (flash_window), 1);
  gtk_widget_show_all (GTK_WIDGET (flash_window));
  flash_priv->flash_timeout_tag = g_timeout_add (FLASH_DURATION, cheese_flash_start_fade, (gpointer) flash);
}

/**
 * cheese_flash_new:
 * @parent: a parent #GtkWidget
 *
 * Create a new #CheeseFlash, associated with the @parent widget.
 *
 * Returns: a new #CheeseFlash
 */
CheeseFlash *
cheese_flash_new (GtkWidget *parent)
{
  return g_object_new (CHEESE_TYPE_FLASH,
                       "parent", parent,
		       "type", GTK_WINDOW_POPUP,
                       NULL);
}
