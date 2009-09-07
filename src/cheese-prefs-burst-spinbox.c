/*
 * Copyright © 2009 Aidan Delaney <a.j.delaney@brighton.ac.uk>
 * Copyright © 2009 daniel g. siegel <dgsiegel@gnome.org>
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

#include "cheese-prefs-widget.h"
#include "cheese-prefs-burst-spinbox.h"

enum
{
  PROP_0,
  PROP_GCONF_KEY,
};

typedef struct CheesePrefsBurstSpinboxPrivate
{
  gchar *gconf_key;
} CheesePrefsBurstSpinboxPrivate;

#define CHEESE_PREFS_BURST_SPINBOX_GET_PRIVATE(o)                     \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHEESE_TYPE_PREFS_BURST_SPINBOX, \
                                CheesePrefsBurstSpinboxPrivate))

G_DEFINE_TYPE (CheesePrefsBurstSpinbox, cheese_prefs_burst_spinbox, CHEESE_TYPE_PREFS_WIDGET);

static void
cheese_prefs_burst_spinbox_init (CheesePrefsBurstSpinbox *self)
{
  CheesePrefsBurstSpinboxPrivate *priv = CHEESE_PREFS_BURST_SPINBOX_GET_PRIVATE (self);

  priv->gconf_key = NULL;
}

static void
cheese_prefs_burst_spinbox_finalize (GObject *object)
{
  CheesePrefsBurstSpinbox        *self = CHEESE_PREFS_BURST_SPINBOX (object);
  CheesePrefsBurstSpinboxPrivate *priv = CHEESE_PREFS_BURST_SPINBOX_GET_PRIVATE (self);

  g_free (priv->gconf_key);

  G_OBJECT_CLASS (cheese_prefs_burst_spinbox_parent_class)->finalize (object);
}

static void
cheese_prefs_burst_spinbox_value_changed (GtkSpinButton *spinbox, CheesePrefsBurstSpinbox *self)
{
  CheesePrefsBurstSpinboxPrivate *priv  = CHEESE_PREFS_BURST_SPINBOX_GET_PRIVATE (self);
  int                             value = 0;

  if (strcmp ("gconf_prop_burst_delay", priv->gconf_key) == 0)
  {
    /* Inputted f is eg: 1.5s. Convert to millisec */
    gdouble d = gtk_spin_button_get_value (spinbox);
    value = (int) (d * 1000);
  }
  else
  {
    value = gtk_spin_button_get_value_as_int (spinbox);
  }

  g_object_set (CHEESE_PREFS_WIDGET (self)->gconf, priv->gconf_key, value, NULL);
  cheese_prefs_widget_notify_changed (CHEESE_PREFS_WIDGET (self));
}

static void
cheese_prefs_burst_spinbox_synchronize (CheesePrefsWidget *prefs_widget)
{
  CheesePrefsBurstSpinbox        *self = CHEESE_PREFS_BURST_SPINBOX (prefs_widget);
  CheesePrefsBurstSpinboxPrivate *priv = CHEESE_PREFS_BURST_SPINBOX_GET_PRIVATE (self);

  GtkWidget *spinbox;
  int        stored_value;

  g_object_get (prefs_widget, "widget", &spinbox, NULL);

  g_object_get (CHEESE_PREFS_WIDGET (self)->gconf, priv->gconf_key, &stored_value, NULL);

  if (!g_ascii_strncasecmp ("gconf_prop_burst_delay", priv->gconf_key, 22))
  {
    stored_value = (int) (stored_value / 1000.0);
  }
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinbox), stored_value);

  /* Disconnect to prevent a whole bunch of changed notifications */
  g_signal_handlers_disconnect_by_func (spinbox, cheese_prefs_burst_spinbox_value_changed, prefs_widget);

  g_signal_connect (G_OBJECT (spinbox), "value-changed",
                    G_CALLBACK (cheese_prefs_burst_spinbox_value_changed),
                    self);
}

static void
cheese_prefs_burst_spinbox_set_property (GObject *object, guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec)
{
  CheesePrefsBurstSpinboxPrivate *priv = CHEESE_PREFS_BURST_SPINBOX_GET_PRIVATE (object);

  switch (prop_id)
  {
    case PROP_GCONF_KEY:
      priv->gconf_key = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
cheese_prefs_burst_spinbox_get_property (GObject *object, guint prop_id,
                                         GValue *value, GParamSpec *pspec)
{
  CheesePrefsBurstSpinboxPrivate *priv = CHEESE_PREFS_BURST_SPINBOX_GET_PRIVATE (object);

  g_return_if_fail (CHEESE_IS_PREFS_BURST_SPINBOX (object));

  switch (prop_id)
  {
    case PROP_GCONF_KEY:
      g_value_set_string (value, priv->gconf_key);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
cheese_prefs_burst_spinbox_class_init (CheesePrefsBurstSpinboxClass *klass)
{
  GObjectClass           *object_class = G_OBJECT_CLASS (klass);
  CheesePrefsWidgetClass *parent_class = CHEESE_PREFS_WIDGET_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CheesePrefsBurstSpinboxPrivate));

  object_class->finalize     = cheese_prefs_burst_spinbox_finalize;
  object_class->set_property = cheese_prefs_burst_spinbox_set_property;
  object_class->get_property = cheese_prefs_burst_spinbox_get_property;
  parent_class->synchronize  = cheese_prefs_burst_spinbox_synchronize;

  g_object_class_install_property (object_class,
                                   PROP_GCONF_KEY,
                                   g_param_spec_string ("gconf_key",
                                                        "",
                                                        "GConf key for burst mode",
                                                        "",
                                                        G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

CheesePrefsBurstSpinbox *
cheese_prefs_burst_spinbox_new (GtkWidget   *spinbox,
                                const gchar *gconf_key)
{
  CheesePrefsBurstSpinbox        *self;
  CheesePrefsBurstSpinboxPrivate *priv;

  self = g_object_new (CHEESE_TYPE_PREFS_BURST_SPINBOX,
                       "widget", spinbox,
                       "gconf_key", gconf_key,
                       NULL);

  priv = CHEESE_PREFS_BURST_SPINBOX_GET_PRIVATE (self);

  return self;
}
