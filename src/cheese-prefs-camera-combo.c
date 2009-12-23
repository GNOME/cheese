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

#include <cheese-camera.h>
#include "cheese-prefs-widget.h"
#include "cheese-prefs-camera-combo.h"

enum
{
  PRODUCT_NAME,
  DEVICE_NAME,
  NUM_COLS
};

enum
{
  PROP_0,
  PROP_CAMERA_DEVICE_KEY,
  PROP_CAMERA
};

typedef struct CheesePrefsCameraComboPrivate
{
  gchar *camera_device_key;
  GtkListStore *list_store;
  CheeseCamera *camera;
  gboolean has_been_synchronized;  /* Make sure we don't synchronize if client
                                    * sets camera on construction. */
} CheesePrefsCameraComboPrivate;

#define CHEESE_PREFS_CAMERA_COMBO_GET_PRIVATE(o)                     \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHEESE_TYPE_PREFS_CAMERA_COMBO, \
                                CheesePrefsCameraComboPrivate))

G_DEFINE_TYPE (CheesePrefsCameraCombo, cheese_prefs_camera_combo, CHEESE_TYPE_PREFS_WIDGET);

static void
cheese_prefs_camera_combo_init (CheesePrefsCameraCombo *self)
{
  CheesePrefsCameraComboPrivate *priv = CHEESE_PREFS_CAMERA_COMBO_GET_PRIVATE (self);

  priv->has_been_synchronized = FALSE;
  priv->camera                = NULL;
  priv->camera_device_key     = NULL;
  priv->list_store            = gtk_list_store_new (NUM_COLS, G_TYPE_STRING, G_TYPE_STRING);
}

static void
cheese_prefs_camera_combo_finalize (GObject *object)
{
  CheesePrefsCameraCombo        *self = CHEESE_PREFS_CAMERA_COMBO (object);
  CheesePrefsCameraComboPrivate *priv = CHEESE_PREFS_CAMERA_COMBO_GET_PRIVATE (self);

  g_free (priv->camera_device_key);
  g_object_unref (priv->list_store);

  G_OBJECT_CLASS (cheese_prefs_camera_combo_parent_class)->finalize (object);
}

static void
cheese_prefs_camera_combo_selection_changed (GtkComboBox *combo_box, CheesePrefsCameraCombo *self)
{
  CheesePrefsCameraComboPrivate *priv = CHEESE_PREFS_CAMERA_COMBO_GET_PRIVATE (self);

  /* Put it into gconf */
  char *new_device = cheese_prefs_camera_combo_get_selected_camera (self);

  g_object_set (CHEESE_PREFS_WIDGET (self)->gconf, priv->camera_device_key, new_device, NULL);
  g_free (new_device);

  cheese_prefs_widget_notify_changed (CHEESE_PREFS_WIDGET (self));
}

static void
cheese_prefs_camera_combo_synchronize (CheesePrefsWidget *prefs_widget)
{
  CheesePrefsCameraCombo        *self = CHEESE_PREFS_CAMERA_COMBO (prefs_widget);
  CheesePrefsCameraComboPrivate *priv = CHEESE_PREFS_CAMERA_COMBO_GET_PRIVATE (self);

  GtkWidget          *combo_box;
  GPtrArray          *camera_devices;
  int                 num_devices;
  CheeseCameraDevice *selected_device;
  char               *gconf_device_name;
  char               *product_name;
  char               *device_name;
  CheeseCameraDevice *device_ptr;
  GtkTreeIter         iter;
  GtkTreeIter         active_iter;
  int                 i;

  gboolean found_same_device = FALSE;

  g_object_get (prefs_widget, "widget", &combo_box, NULL);

  /* Disconnect to prevent a whole bunch of changed notifications */
  g_signal_handlers_disconnect_by_func (combo_box, cheese_prefs_camera_combo_selection_changed, prefs_widget);

  g_object_ref (priv->list_store);

  gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box), NULL);

  camera_devices  = cheese_camera_get_camera_devices (priv->camera);
  num_devices     = cheese_camera_get_num_camera_devices (priv->camera);
  selected_device = cheese_camera_get_selected_device (priv->camera);

  /* If the selected device is not the same device as the one in gconf, the
   * selected device isn't available or was set by --hal-device. Set it now.
   * Not sure if this is desired behavior */
  if (num_devices > 0)
  {
    const gchar *devpath = cheese_camera_device_get_device_file (selected_device);
    g_object_get (prefs_widget->gconf, priv->camera_device_key, &gconf_device_name, NULL);
    if (!gconf_device_name || strcmp (devpath, gconf_device_name) != 0)
    {
      g_object_set (prefs_widget->gconf, priv->camera_device_key, devpath, NULL);
    }
    g_free (gconf_device_name);
  }
  gtk_list_store_clear (priv->list_store);

  for (i = 0; i < num_devices; i++)
  {
    device_ptr = g_ptr_array_index (camera_devices, i);
    const gchar *devpath = cheese_camera_device_get_device_file (device_ptr);
    product_name = g_strdup_printf ("%s (%s)",
                                    cheese_camera_device_get_name (device_ptr),
                                    devpath);
    device_name = g_strdup (devpath);

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
                    G_CALLBACK (cheese_prefs_camera_combo_selection_changed),
                    self);

  /* Set sensitive or not depending on whether or not there are camera devices
   * available */
  gtk_widget_set_sensitive (combo_box, num_devices > 1);

  g_ptr_array_unref (camera_devices);
}

static void
cheese_prefs_camera_combo_set_property (GObject *object, guint prop_id,
                                        const GValue *value,
                                        GParamSpec *pspec)
{
  CheesePrefsCameraComboPrivate *priv = CHEESE_PREFS_CAMERA_COMBO_GET_PRIVATE (object);

  switch (prop_id)
  {
    case PROP_CAMERA_DEVICE_KEY:
      priv->camera_device_key = g_value_dup_string (value);
      break;
    case PROP_CAMERA:
      priv->camera = CHEESE_CAMERA (g_value_get_object (value));
      if (priv->has_been_synchronized)
        cheese_prefs_camera_combo_synchronize (CHEESE_PREFS_WIDGET (object));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
cheese_prefs_camera_combo_get_property (GObject *object, guint prop_id,
                                        GValue *value, GParamSpec *pspec)
{
  CheesePrefsCameraComboPrivate *priv = CHEESE_PREFS_CAMERA_COMBO_GET_PRIVATE (object);

  g_return_if_fail (CHEESE_IS_PREFS_CAMERA_COMBO (object));

  switch (prop_id)
  {
    case PROP_CAMERA_DEVICE_KEY:
      g_value_set_string (value, priv->camera_device_key);
      break;
    case PROP_CAMERA:
      g_value_set_object (value, priv->camera);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
cheese_prefs_camera_combo_class_init (CheesePrefsCameraComboClass *klass)
{
  GObjectClass           *object_class = G_OBJECT_CLASS (klass);
  CheesePrefsWidgetClass *parent_class = CHEESE_PREFS_WIDGET_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CheesePrefsCameraComboPrivate));

  object_class->finalize     = cheese_prefs_camera_combo_finalize;
  object_class->set_property = cheese_prefs_camera_combo_set_property;
  object_class->get_property = cheese_prefs_camera_combo_get_property;
  parent_class->synchronize  = cheese_prefs_camera_combo_synchronize;

  g_object_class_install_property (object_class,
                                   PROP_CAMERA_DEVICE_KEY,
                                   g_param_spec_string ("camera_device_key",
                                                        "",
                                                        "Camera device gconf key",
                                                        "",
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_CAMERA,
                                   g_param_spec_object ("camera",
                                                        "",
                                                        "Camera object",
                                                        CHEESE_TYPE_CAMERA,
                                                        G_PARAM_READWRITE));
}

CheesePrefsCameraCombo *
cheese_prefs_camera_combo_new (GtkWidget *combo_box, CheeseCamera *camera,
                               const gchar *camera_device_key)
{
  CheesePrefsCameraCombo        *self;
  GtkCellRenderer               *renderer;
  CheesePrefsCameraComboPrivate *priv;

  self = g_object_new (CHEESE_TYPE_PREFS_CAMERA_COMBO,
                       "widget", combo_box,
                       "camera", camera,
                       "camera_device_key", camera_device_key,
                       NULL);

  priv = CHEESE_PREFS_CAMERA_COMBO_GET_PRIVATE (self);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), renderer, FALSE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo_box), renderer, "text",
                                 PRODUCT_NAME);
  gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box),
                           GTK_TREE_MODEL (priv->list_store));

  return self;
}

char *
cheese_prefs_camera_combo_get_selected_camera (CheesePrefsCameraCombo *camera)
{
  CheesePrefsCameraComboPrivate *priv = CHEESE_PREFS_CAMERA_COMBO_GET_PRIVATE (camera);

  GtkTreeIter active_iter;
  GtkWidget  *combo_box;
  char       *device;

  g_object_get (camera, "widget", &combo_box, NULL);

  gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo_box), &active_iter);
  gtk_tree_model_get (GTK_TREE_MODEL (priv->list_store), &active_iter, DEVICE_NAME, &device, -1);

  return device;
}
