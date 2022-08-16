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

/*
 * CheeseFlashPrivate:
 * @parent: the parent #GtkWidget, for choosing on which display to fire the
 * flash
 * @flash_timeout_tag: signal ID of the timeout to start fading in the flash
 * @fade_timeout_tag: signal ID of the timeout to start fading out the flash
 *
 * Private data for #CheeseFlash.
 */
typedef struct
{
  /*< private >*/
  GtkWidget *parent;
  guint flash_timeout_tag;
  guint fade_timeout_tag;
  gdouble opacity;
} CheeseFlashPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (CheeseFlash, cheese_flash, GTK_TYPE_WINDOW)

static void
cheese_flash_init (CheeseFlash *self)
{

}

static void
cheese_flash_dispose (GObject *object)
{
    CheeseFlashPrivate *priv = cheese_flash_get_instance_private (CHEESE_FLASH (object));

  g_clear_object (&priv->parent);

    G_OBJECT_CLASS (cheese_flash_parent_class)->dispose (object);
}

static void
cheese_flash_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
    CheeseFlashPrivate *priv = cheese_flash_get_instance_private (CHEESE_FLASH (object));

  switch (prop_id)
  {
    case PROP_PARENT: {
      GObject *parent;
      parent = g_value_get_object (value);
      if (object != NULL)
        priv->parent = g_object_ref (parent);
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

/**
 * cheese_flash_fire:
 * @flash: a #CheeseFlash
 *
 * Fire the flash.
 */
void
cheese_flash_fire (CheeseFlash *flash)
{
    CheeseFlashPrivate *priv;
  GdkRectangle        rect;
  GdkMonitor *monitor;
  GDBusProxy *proxy;
  GVariant *parameters;

  g_return_if_fail (CHEESE_IS_FLASH (flash));

    priv = cheese_flash_get_instance_private (flash);

    g_return_if_fail (priv->parent != NULL);

  monitor = gdk_display_get_monitor_at_window (gdk_display_get_default (), gtk_widget_get_window (priv->parent));
  gdk_monitor_get_geometry (monitor, &rect);

  proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE,
                                         NULL,
                                         "org.gnome.Shell.Screenshot",
                                         "/org/gnome/Shell/Screenshot",
                                         "org.gnome.Shell.Screenshot", NULL, NULL);
  parameters = g_variant_new_parsed ("(%i, %i, %i, %i)", rect.x, rect.y, rect.width, rect.height);
  g_dbus_proxy_call_sync (proxy, "FlashArea", parameters, G_DBUS_CALL_FLAGS_NONE, G_MAXINT, NULL, NULL);

  g_object_unref (proxy);
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
