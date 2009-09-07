/*
 * Copyright © 2008 James Liggett <jrliggett@cox.net>
 * Copyright © 2008 Ryan Zeigler <zeiglerr@gmail.com>
 * Copyright © 2008 daniel g. siegel <dgsiegel@gnome.org>
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
#include "cheese-prefs-webcam-combo.h"

enum
{
  PRODUCT_NAME,
  DEVICE_NAME,
  NUM_COLS
};

enum
{
  PROP_0,
  PROP_WEBCAM_DEVICE_KEY,
  PROP_WEBCAM
};

typedef struct CheesePrefsWebcamComboPrivate
{
  gchar *webcam_device_key;
  GtkListStore *list_store;
  CheeseWebcam *webcam;
  gboolean has_been_synchronized;  /* Make sure we don't synchronize if client
                                    * sets webcam on construction. */
} CheesePrefsWebcamComboPrivate;

#define CHEESE_PREFS_WEBCAM_COMBO_GET_PRIVATE(o)                     \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHEESE_TYPE_PREFS_WEBCAM_COMBO, \
                                CheesePrefsWebcamComboPrivate))

G_DEFINE_TYPE (CheesePrefsWebcamCombo, cheese_prefs_webcam_combo, CHEESE_TYPE_PREFS_WIDGET);

static void
cheese_prefs_webcam_combo_init (CheesePrefsWebcamCombo *self)
{
  CheesePrefsWebcamComboPrivate *priv = CHEESE_PREFS_WEBCAM_COMBO_GET_PRIVATE (self);

  priv->has_been_synchronized = FALSE;
  priv->webcam                = NULL;
  priv->webcam_device_key     = NULL;
  priv->list_store            = gtk_list_store_new (NUM_COLS, G_TYPE_STRING, G_TYPE_STRING);
}

static void
cheese_prefs_webcam_combo_finalize (GObject *object)
{
  CheesePrefsWebcamCombo        *self = CHEESE_PREFS_WEBCAM_COMBO (object);
  CheesePrefsWebcamComboPrivate *priv = CHEESE_PREFS_WEBCAM_COMBO_GET_PRIVATE (self);

  g_free (priv->webcam_device_key);
  g_object_unref (priv->list_store);

  G_OBJECT_CLASS (cheese_prefs_webcam_combo_parent_class)->finalize (object);
}

static void
cheese_prefs_webcam_combo_selection_changed (GtkComboBox *combo_box, CheesePrefsWebcamCombo *self)
{
  CheesePrefsWebcamComboPrivate *priv = CHEESE_PREFS_WEBCAM_COMBO_GET_PRIVATE (self);

  /* Put it into gconf */
  char *new_device = cheese_prefs_webcam_combo_get_selected_webcam (self);

  g_object_set (CHEESE_PREFS_WIDGET (self)->gconf, priv->webcam_device_key, new_device, NULL);
  g_free (new_device);

  cheese_prefs_widget_notify_changed (CHEESE_PREFS_WIDGET (self));
}

static void
cheese_prefs_webcam_combo_synchronize (CheesePrefsWidget *prefs_widget)
{
  CheesePrefsWebcamCombo        *self = CHEESE_PREFS_WEBCAM_COMBO (prefs_widget);
  CheesePrefsWebcamComboPrivate *priv = CHEESE_PREFS_WEBCAM_COMBO_GET_PRIVATE (self);

  GtkWidget          *combo_box;
  GArray             *webcam_devices;
  int                 selected_device_ind;
  int                 num_devices;
  CheeseWebcamDevice *selected_device;
  char               *gconf_device_name;
  char               *product_name;
  char               *device_name;
  CheeseWebcamDevice *device_ptr;
  GtkTreeIter         iter;
  GtkTreeIter         active_iter;
  int                 i;

  gboolean found_same_device = FALSE;

  g_object_get (prefs_widget, "widget", &combo_box, NULL);

  /* Disconnect to prevent a whole bunch of changed notifications */
  g_signal_handlers_disconnect_by_func (combo_box, cheese_prefs_webcam_combo_selection_changed, prefs_widget);

  g_object_ref (priv->list_store);

  gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box), NULL);

  webcam_devices      = cheese_webcam_get_webcam_devices (priv->webcam);
  selected_device_ind = cheese_webcam_get_selected_device_index (priv->webcam);
  num_devices         = cheese_webcam_get_num_webcam_devices (priv->webcam);

  selected_device = &g_array_index (webcam_devices, CheeseWebcamDevice, selected_device_ind);

  /* If the selected device is not the same device as the one in gconf, the
   * selected device isn't available or was set by --hal-device. Set it now.
   * Not sure if this is desired behavior */
  if (num_devices > 0)
  {
    g_object_get (prefs_widget->gconf, priv->webcam_device_key, &gconf_device_name, NULL);
    if (!gconf_device_name || strcmp (selected_device->video_device, gconf_device_name) != 0)
    {
      g_object_set (prefs_widget->gconf, priv->webcam_device_key, selected_device->video_device, NULL);
    }
    g_free (gconf_device_name);
  }
  gtk_list_store_clear (priv->list_store);

  for (i = 0; i < num_devices; i++)
  {
    device_ptr   = &g_array_index (webcam_devices, CheeseWebcamDevice, i);
    product_name = g_strdup_printf ("%s (%s)", device_ptr->product_name, device_ptr->video_device);
    device_name  = g_strdup (device_ptr->video_device);

    gtk_list_store_append (priv->list_store, &iter);
    gtk_list_store_set (priv->list_store, &iter, PRODUCT_NAME, product_name,
                        DEVICE_NAME, device_name,
                        -1);
    g_free (product_name);
    g_free (device_name);
    if (device_ptr == selected_device)
    {
      active_iter       = iter;
      found_same_device = TRUE;
    }
  }

  gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box),
                           GTK_TREE_MODEL (priv->list_store));

  g_object_unref (priv->list_store);

  if (found_same_device)
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo_box), &active_iter);
  else
    gtk_combo_box_set_active (GTK_COMBO_BOX (combo_box), 0);

  g_signal_connect (G_OBJECT (combo_box), "changed",
                    G_CALLBACK (cheese_prefs_webcam_combo_selection_changed),
                    self);

  /* Set sensitive or not depending on whether or not there are webcam devices
   * available */
  gtk_widget_set_sensitive (combo_box, num_devices > 1);

  g_array_free (webcam_devices, TRUE);
}

static void
cheese_prefs_webcam_combo_set_property (GObject *object, guint prop_id,
                                        const GValue *value,
                                        GParamSpec *pspec)
{
  CheesePrefsWebcamComboPrivate *priv = CHEESE_PREFS_WEBCAM_COMBO_GET_PRIVATE (object);

  switch (prop_id)
  {
    case PROP_WEBCAM_DEVICE_KEY:
      priv->webcam_device_key = g_value_dup_string (value);
      break;
    case PROP_WEBCAM:
      priv->webcam = CHEESE_WEBCAM (g_value_get_object (value));
      if (priv->has_been_synchronized)
        cheese_prefs_webcam_combo_synchronize (CHEESE_PREFS_WIDGET (object));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
cheese_prefs_webcam_combo_get_property (GObject *object, guint prop_id,
                                        GValue *value, GParamSpec *pspec)
{
  CheesePrefsWebcamComboPrivate *priv = CHEESE_PREFS_WEBCAM_COMBO_GET_PRIVATE (object);

  g_return_if_fail (CHEESE_IS_PREFS_WEBCAM_COMBO (object));

  switch (prop_id)
  {
    case PROP_WEBCAM_DEVICE_KEY:
      g_value_set_string (value, priv->webcam_device_key);
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
cheese_prefs_webcam_combo_class_init (CheesePrefsWebcamComboClass *klass)
{
  GObjectClass           *object_class = G_OBJECT_CLASS (klass);
  CheesePrefsWidgetClass *parent_class = CHEESE_PREFS_WIDGET_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CheesePrefsWebcamComboPrivate));

  object_class->finalize     = cheese_prefs_webcam_combo_finalize;
  object_class->set_property = cheese_prefs_webcam_combo_set_property;
  object_class->get_property = cheese_prefs_webcam_combo_get_property;
  parent_class->synchronize  = cheese_prefs_webcam_combo_synchronize;

  g_object_class_install_property (object_class,
                                   PROP_WEBCAM_DEVICE_KEY,
                                   g_param_spec_string ("webcam_device_key",
                                                        "",
                                                        "Webcam device gconf key",
                                                        "",
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_WEBCAM,
                                   g_param_spec_object ("webcam",
                                                        "",
                                                        "Webcam object",
                                                        CHEESE_TYPE_WEBCAM,
                                                        G_PARAM_READWRITE));
}

CheesePrefsWebcamCombo *
cheese_prefs_webcam_combo_new (GtkWidget *combo_box, CheeseWebcam *webcam,
                               const gchar *webcam_device_key)
{
  CheesePrefsWebcamCombo        *self;
  GtkCellRenderer               *renderer;
  CheesePrefsWebcamComboPrivate *priv;

  self = g_object_new (CHEESE_TYPE_PREFS_WEBCAM_COMBO,
                       "widget", combo_box,
                       "webcam", webcam,
                       "webcam_device_key", webcam_device_key,
                       NULL);

  priv = CHEESE_PREFS_WEBCAM_COMBO_GET_PRIVATE (self);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), renderer, FALSE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo_box), renderer, "text",
                                 PRODUCT_NAME);
  gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box),
                           GTK_TREE_MODEL (priv->list_store));

  return self;
}

char *
cheese_prefs_webcam_combo_get_selected_webcam (CheesePrefsWebcamCombo *webcam)
{
  CheesePrefsWebcamComboPrivate *priv = CHEESE_PREFS_WEBCAM_COMBO_GET_PRIVATE (webcam);

  GtkTreeIter active_iter;
  GtkWidget  *combo_box;
  char       *device;

  g_object_get (webcam, "widget", &combo_box, NULL);

  gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo_box), &active_iter);
  gtk_tree_model_get (GTK_TREE_MODEL (priv->list_store), &active_iter, DEVICE_NAME, &device, -1);

  return device;
}
