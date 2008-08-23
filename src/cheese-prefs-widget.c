/*
 * Copyright © 2008 James Liggett <jrliggett@cox.net>
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

#include "cheese-prefs-widget.h"

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


static guint prefs_widget_signals[LAST_SIGNAL] = {0};

typedef struct
{
  GtkWidget *widget;
} CheesePrefsWidgetPrivate;

#define CHEESE_PREFS_WIDGET_GET_PRIVATE(o)                     \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHEESE_TYPE_PREFS_WIDGET, \
                                CheesePrefsWidgetPrivate))

G_DEFINE_TYPE (CheesePrefsWidget, cheese_prefs_widget, G_TYPE_OBJECT);

static void
cheese_prefs_widget_init (CheesePrefsWidget *prefs_widget)
{
  CheesePrefsWidgetPrivate *priv = CHEESE_PREFS_WIDGET_GET_PRIVATE (prefs_widget);

  prefs_widget->gconf = NULL;
  priv->widget        = NULL;
}

static void
cheese_prefs_widget_finalize (GObject *object)
{
  G_OBJECT_CLASS (cheese_prefs_widget_parent_class)->finalize (object);
}

static void
cheese_prefs_widget_set_property (GObject *object, guint prop_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
  CheesePrefsWidgetPrivate *priv = CHEESE_PREFS_WIDGET_GET_PRIVATE (object);

  g_return_if_fail (CHEESE_IS_PREFS_WIDGET (object));

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
cheese_prefs_widget_get_property (GObject *object, guint prop_id,
                                  GValue *value, GParamSpec *pspec)
{
  CheesePrefsWidgetPrivate *priv = CHEESE_PREFS_WIDGET_GET_PRIVATE (object);

  g_return_if_fail (CHEESE_IS_PREFS_WIDGET (object));

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

static void
cheese_prefs_widget_changed (CheesePrefsWidget *self)
{
}

static void
cheese_prefs_widget_class_init (CheesePrefsWidgetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = cheese_prefs_widget_finalize;
  object_class->set_property = cheese_prefs_widget_set_property;
  object_class->get_property = cheese_prefs_widget_get_property;

  klass->changed     = cheese_prefs_widget_changed;
  klass->synchronize = NULL;

  g_type_class_add_private (klass, sizeof (CheesePrefsWidgetPrivate));

  prefs_widget_signals[CHANGED] =
    g_signal_new ("changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  0,
                  G_STRUCT_OFFSET (CheesePrefsWidgetClass, changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  g_object_class_install_property (object_class, PROP_WIDGET,
                                   g_param_spec_object ("widget",
                                                        "",
                                                        "The widget that this object wraps",
                                                        GTK_TYPE_WIDGET,
                                                        G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

void
cheese_prefs_widget_synchronize (CheesePrefsWidget *prefs_widget)
{
  CHEESE_PREFS_WIDGET_GET_CLASS (prefs_widget)->synchronize (prefs_widget);
}

void
cheese_prefs_widget_notify_changed (CheesePrefsWidget *prefs_widget)
{
  g_signal_emit_by_name (prefs_widget, "changed");
}
