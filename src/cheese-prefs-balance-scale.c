/*
 * Copyright © 2009 Filippo Argiolas <filippo.argiolas@gmail.com>
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

#include <string.h>
#include <glib.h>

#include "cheese-webcam.h"
#include "cheese-prefs-widget.h"
#include "cheese-prefs-balance-scale.h"

#define STEPS 20.0

enum
{
  PROP_0,
  PROP_PROPERTY_NAME,
  PROP_GCONF_KEY,
  PROP_WEBCAM
};

typedef struct CheesePrefsBalanceScalePrivate
{
  CheeseWebcam *webcam;
  gchar *property_name;
  gchar *gconf_key;
  gboolean has_been_synchronized;  /* Make sure we don't synchronize if client
                                    * sets webcam on construction. */
} CheesePrefsBalanceScalePrivate;

#define CHEESE_PREFS_BALANCE_SCALE_GET_PRIVATE(o)                     \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHEESE_TYPE_PREFS_BALANCE_SCALE, \
                                CheesePrefsBalanceScalePrivate))

G_DEFINE_TYPE (CheesePrefsBalanceScale, cheese_prefs_balance_scale, CHEESE_TYPE_PREFS_WIDGET);

static void
cheese_prefs_balance_scale_init (CheesePrefsBalanceScale *self)
{
  CheesePrefsBalanceScalePrivate *priv = CHEESE_PREFS_BALANCE_SCALE_GET_PRIVATE (self);

  priv->property_name         = NULL;
  priv->gconf_key             = NULL;
  priv->has_been_synchronized = FALSE;
}

static void
cheese_prefs_balance_scale_finalize (GObject *object)
{
  CheesePrefsBalanceScale        *self = CHEESE_PREFS_BALANCE_SCALE (object);
  CheesePrefsBalanceScalePrivate *priv = CHEESE_PREFS_BALANCE_SCALE_GET_PRIVATE (self);

  g_free (priv->property_name);
  g_free (priv->gconf_key);

  G_OBJECT_CLASS (cheese_prefs_balance_scale_parent_class)->finalize (object);
}

static void
cheese_prefs_balance_scale_value_changed (GtkRange *scale, CheesePrefsBalanceScale *self)
{
  CheesePrefsBalanceScalePrivate *priv  = CHEESE_PREFS_BALANCE_SCALE_GET_PRIVATE (self);
  gdouble                         value = gtk_range_get_value (scale);

  cheese_webcam_set_balance_property (priv->webcam, priv->property_name, value);

  g_object_set (CHEESE_PREFS_WIDGET (self)->gconf, priv->gconf_key, value, NULL);

  cheese_prefs_widget_notify_changed (CHEESE_PREFS_WIDGET (self));
}

static void
cheese_prefs_balance_scale_synchronize (CheesePrefsWidget *prefs_widget)
{
  CheesePrefsBalanceScale        *self = CHEESE_PREFS_BALANCE_SCALE (prefs_widget);
  CheesePrefsBalanceScalePrivate *priv = CHEESE_PREFS_BALANCE_SCALE_GET_PRIVATE (self);

  GtkWidget     *scale;
  GtkAdjustment *adj;
  gdouble        min, max, def;
  gdouble        stored_value;

  g_object_get (prefs_widget, "widget", &scale, NULL);

  cheese_webcam_get_balance_property_range (priv->webcam,
                                            priv->property_name, &min, &max, &def);

  adj = GTK_ADJUSTMENT (gtk_adjustment_new (def, min, max, (max - min) / STEPS, 0.0, 0.0));
  gtk_range_set_adjustment (GTK_RANGE (scale), adj);

  gtk_scale_add_mark (GTK_SCALE (scale), def, GTK_POS_BOTTOM, NULL);

  g_object_get (CHEESE_PREFS_WIDGET (self)->gconf, priv->gconf_key, &stored_value, NULL);

  gtk_range_set_value (GTK_RANGE (scale), stored_value);

  /* Disconnect to prevent a whole bunch of changed notifications */
  g_signal_handlers_disconnect_by_func (scale, cheese_prefs_balance_scale_value_changed, prefs_widget);

  g_signal_connect (G_OBJECT (scale), "value-changed",
                    G_CALLBACK (cheese_prefs_balance_scale_value_changed),
                    self);
}

static void
cheese_prefs_balance_scale_set_property (GObject *object, guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec)
{
  CheesePrefsBalanceScalePrivate *priv = CHEESE_PREFS_BALANCE_SCALE_GET_PRIVATE (object);

  switch (prop_id)
  {
    case PROP_PROPERTY_NAME:
      priv->property_name = g_value_dup_string (value);
      break;
    case PROP_GCONF_KEY:
      priv->gconf_key = g_value_dup_string (value);
      break;
    case PROP_WEBCAM:
      priv->webcam = CHEESE_WEBCAM (g_value_get_object (value));
      if (priv->has_been_synchronized)
        cheese_prefs_balance_scale_synchronize (CHEESE_PREFS_WIDGET (object));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
cheese_prefs_balance_scale_get_property (GObject *object, guint prop_id,
                                         GValue *value, GParamSpec *pspec)
{
  CheesePrefsBalanceScalePrivate *priv = CHEESE_PREFS_BALANCE_SCALE_GET_PRIVATE (object);

  g_return_if_fail (CHEESE_IS_PREFS_BALANCE_SCALE (object));

  switch (prop_id)
  {
    case PROP_PROPERTY_NAME:
      g_value_set_string (value, priv->property_name);
      break;
    case PROP_GCONF_KEY:
      g_value_set_string (value, priv->gconf_key);
      break;
    case PROP_WEBCAM:
      g_value_set_object (value, priv->webcam);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
cheese_prefs_balance_scale_class_init (CheesePrefsBalanceScaleClass *klass)
{
  GObjectClass           *object_class = G_OBJECT_CLASS (klass);
  CheesePrefsWidgetClass *parent_class = CHEESE_PREFS_WIDGET_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CheesePrefsBalanceScalePrivate));

  object_class->finalize     = cheese_prefs_balance_scale_finalize;
  object_class->set_property = cheese_prefs_balance_scale_set_property;
  object_class->get_property = cheese_prefs_balance_scale_get_property;
  parent_class->synchronize  = cheese_prefs_balance_scale_synchronize;

  g_object_class_install_property (object_class,
                                   PROP_PROPERTY_NAME,
                                   g_param_spec_string ("property_name",
                                                        "",
                                                        "Property this widget will control in the videobalance element",
                                                        "",
                                                        G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_GCONF_KEY,
                                   g_param_spec_string ("gconf_key",
                                                        "",
                                                        "GConf key for balance",
                                                        "",
                                                        G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_WEBCAM,
                                   g_param_spec_object ("webcam",
                                                        "webcam",
                                                        "Webcam object",
                                                        CHEESE_TYPE_WEBCAM,
                                                        G_PARAM_READWRITE));
}

CheesePrefsBalanceScale *
cheese_prefs_balance_scale_new (GtkWidget    *scale,
                                CheeseWebcam *webcam,
                                const gchar  *property,
                                const gchar  *gconf_key)
{
  CheesePrefsBalanceScale        *self;
  CheesePrefsBalanceScalePrivate *priv;

  self = g_object_new (CHEESE_TYPE_PREFS_BALANCE_SCALE,
                       "widget", scale,
                       "webcam", webcam,
                       "property_name", property,
                       "gconf_key", gconf_key,
                       NULL);

  priv = CHEESE_PREFS_BALANCE_SCALE_GET_PRIVATE (self);

  return self;
}
