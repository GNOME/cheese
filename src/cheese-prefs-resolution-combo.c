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

#include "cheese-prefs-resolution-combo.h"

typedef struct
{
  gchar *x_resolution_key;
  gchar *y_resolution_key;
  unsigned int max_x_resolution;
  unsigned int max_y_resolution;
  GtkListStore *list_store;
  CheeseWebcam *webcam;
  CheeseVideoFormat *selected_format;
  gboolean has_been_synchronized;  /* Make sure we don't synchronize if client
                                    * sets webcam on construction. */
} CheesePrefsResolutionComboPrivate;

#define CHEESE_PREFS_RESOLUTION_COMBO_GET_PRIVATE(o)                     \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHEESE_TYPE_PREFS_RESOLUTION_COMBO, \
                                CheesePrefsResolutionComboPrivate))

enum
{
  PROP_0,

  PROP_X_RESOLUTION_KEY,
  PROP_Y_RESOLUTION_KEY,
  PROP_MAX_X_RESOLUTION,
  PROP_MAX_Y_RESOLUTION,
  PROP_WEBCAM
};

enum
{
  COL_NAME,
  COL_FORMAT,
  NUM_COLS
};

G_DEFINE_TYPE (CheesePrefsResolutionCombo, cheese_prefs_resolution_combo, CHEESE_TYPE_PREFS_WIDGET);

static void
cheese_prefs_resolution_combo_init (CheesePrefsResolutionCombo *self)
{
  CheesePrefsResolutionComboPrivate *priv = CHEESE_PREFS_RESOLUTION_COMBO_GET_PRIVATE (self);

  priv->x_resolution_key      = NULL;
  priv->y_resolution_key      = NULL;
  priv->max_x_resolution      = G_MAXUINT;
  priv->max_y_resolution      = G_MAXUINT;
  priv->list_store            = gtk_list_store_new (NUM_COLS, G_TYPE_STRING, G_TYPE_POINTER);
  priv->webcam                = NULL;
  priv->selected_format       = NULL;
  priv->has_been_synchronized = FALSE;
}

static void
cheese_prefs_resolution_combo_finalize (GObject *object)
{
  CheesePrefsResolutionCombo        *self = CHEESE_PREFS_RESOLUTION_COMBO (object);
  CheesePrefsResolutionComboPrivate *priv = CHEESE_PREFS_RESOLUTION_COMBO_GET_PRIVATE (self);

  g_free (priv->x_resolution_key);
  g_free (priv->y_resolution_key);
  g_object_unref (priv->list_store);

  G_OBJECT_CLASS (cheese_prefs_resolution_combo_parent_class)->finalize (object);
}

static void
combo_selection_changed (GtkComboBox                *combo_box,
                         CheesePrefsResolutionCombo *self)
{
  CheesePrefsResolutionComboPrivate *priv = CHEESE_PREFS_RESOLUTION_COMBO_GET_PRIVATE (self);

  GtkTreeIter        iter;
  CheeseVideoFormat *format;

  gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo_box), &iter);
  gtk_tree_model_get (GTK_TREE_MODEL (priv->list_store), &iter, COL_FORMAT,
                      &format, -1);

  g_object_set (CHEESE_PREFS_WIDGET (self)->gconf, priv->x_resolution_key,
                format->width, priv->y_resolution_key, format->height, NULL);

  priv->selected_format = format;

  cheese_prefs_widget_notify_changed (CHEESE_PREFS_WIDGET (self));
}

static void
cheese_prefs_resolution_combo_synchronize (CheesePrefsWidget *prefs_widget)
{
  CheesePrefsResolutionCombo        *self = CHEESE_PREFS_RESOLUTION_COMBO (prefs_widget);
  CheesePrefsResolutionComboPrivate *priv = CHEESE_PREFS_RESOLUTION_COMBO_GET_PRIVATE (self);

  GtkWidget         *combo_box;
  CheeseVideoFormat *current_format;
  GArray            *formats;
  int                i;
  CheeseVideoFormat *format;
  gchar             *format_name;
  GtkTreeIter        iter;
  GtkTreeIter        active_iter;
  gboolean           found_resolution;

  priv->has_been_synchronized = TRUE;
  found_resolution            = FALSE;

  /* Don't generate spurious changed events when we set the active resolution */
  g_object_get (prefs_widget, "widget", &combo_box, NULL);
  g_signal_handlers_disconnect_by_func (combo_box, combo_selection_changed,
                                        prefs_widget);

  g_object_ref (priv->list_store);
  gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box), NULL);

  gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box), NULL);
  current_format = cheese_webcam_get_current_video_format (priv->webcam);

  gtk_list_store_clear (priv->list_store);
  formats = cheese_webcam_get_video_formats (priv->webcam);

  for (i = 0; i < formats->len; i++)
  {
    format      = &g_array_index (formats, CheeseVideoFormat, i);
    format_name = g_strdup_printf ("%i x %i", format->width, format->height);


    if (format->width <= priv->max_x_resolution &&
        format->height <= priv->max_y_resolution)
    {
      gtk_list_store_append (priv->list_store, &iter);
      gtk_list_store_set (priv->list_store, &iter,
                          COL_NAME, format_name,
                          COL_FORMAT, format,
                          -1);

      g_free (format_name);

      if (format == current_format)
      {
        active_iter      = iter;
        found_resolution = TRUE;
      }
    }
  }

  gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box),
                           GTK_TREE_MODEL (priv->list_store));
  g_object_unref (priv->list_store);

  if (found_resolution)
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo_box), &active_iter);
  else
    gtk_combo_box_set_active (GTK_COMBO_BOX (combo_box), 0);

  g_signal_connect (G_OBJECT (combo_box), "changed",
                    G_CALLBACK (combo_selection_changed),
                    self);

  gtk_widget_set_sensitive (combo_box, formats->len > 1);
}

static void
cheese_prefs_resolution_combo_set_property (GObject *object, guint prop_id,
                                            const GValue *value,
                                            GParamSpec *pspec)
{
  CheesePrefsResolutionComboPrivate *priv = CHEESE_PREFS_RESOLUTION_COMBO_GET_PRIVATE (object);

  g_return_if_fail (CHEESE_IS_PREFS_RESOLUTION_COMBO (object));

  switch (prop_id)
  {
    case PROP_X_RESOLUTION_KEY:
      priv->x_resolution_key = g_value_dup_string (value);
      break;
    case PROP_Y_RESOLUTION_KEY:
      priv->y_resolution_key = g_value_dup_string (value);
      break;
    case PROP_MAX_X_RESOLUTION:
      priv->max_x_resolution = g_value_get_uint (value);
      break;
    case PROP_MAX_Y_RESOLUTION:
      priv->max_y_resolution = g_value_get_uint (value);
      break;
    case PROP_WEBCAM:
      priv->webcam = CHEESE_WEBCAM (g_value_get_object (value));

      /* If the webcam changes the resolutions change too. But only change the
       * data if we've been synchronized once already. If this property is set
       * on construction, we would synchronize twice--once when the property is
       * set, and again when the dialog syncs when it's created. */
      if (priv->has_been_synchronized)
        cheese_prefs_resolution_combo_synchronize (CHEESE_PREFS_WIDGET (object));

      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
cheese_prefs_resolution_combo_get_property (GObject *object, guint prop_id,
                                            GValue *value, GParamSpec *pspec)
{
  CheesePrefsResolutionComboPrivate *priv = CHEESE_PREFS_RESOLUTION_COMBO_GET_PRIVATE (object);

  g_return_if_fail (CHEESE_IS_PREFS_RESOLUTION_COMBO (object));

  switch (prop_id)
  {
    case PROP_X_RESOLUTION_KEY:
      g_value_set_string (value, priv->x_resolution_key);
      break;
    case PROP_Y_RESOLUTION_KEY:
      g_value_set_string (value, priv->y_resolution_key);
      break;
    case PROP_MAX_X_RESOLUTION:
      g_value_set_uint (value, priv->max_x_resolution);
      break;
    case PROP_MAX_Y_RESOLUTION:
      g_value_set_uint (value, priv->max_y_resolution);
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
cheese_prefs_resolution_combo_class_init (CheesePrefsResolutionComboClass *klass)
{
  GObjectClass           *object_class = G_OBJECT_CLASS (klass);
  CheesePrefsWidgetClass *parent_class = CHEESE_PREFS_WIDGET_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CheesePrefsResolutionComboPrivate));

  object_class->finalize     = cheese_prefs_resolution_combo_finalize;
  object_class->set_property = cheese_prefs_resolution_combo_set_property;
  object_class->get_property = cheese_prefs_resolution_combo_get_property;
  parent_class->synchronize  = cheese_prefs_resolution_combo_synchronize;

  g_object_class_install_property (object_class,
                                   PROP_X_RESOLUTION_KEY,
                                   g_param_spec_string ("x_resolution_key",
                                                        "",
                                                        "GConf key for X resolution",
                                                        "",
                                                        G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_Y_RESOLUTION_KEY,
                                   g_param_spec_string ("y_resolution_key",
                                                        "",
                                                        "GConf key for Y resolution",
                                                        "",
                                                        G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_MAX_X_RESOLUTION,
                                   g_param_spec_uint ("max_x_resolution",
                                                      "",
                                                      "Maximum supported X resolution",
                                                      0,
                                                      G_MAXUINT,
                                                      G_MAXUINT,
                                                      G_PARAM_READABLE | G_PARAM_WRITABLE));

  g_object_class_install_property (object_class,
                                   PROP_MAX_Y_RESOLUTION,
                                   g_param_spec_uint ("max_y_resolution",
                                                      "",
                                                      "Maximum supported Y resolution",
                                                      0,
                                                      G_MAXUINT,
                                                      G_MAXUINT,
                                                      G_PARAM_READABLE | G_PARAM_WRITABLE));

  g_object_class_install_property (object_class,
                                   PROP_WEBCAM,
                                   g_param_spec_object ("webcam",
                                                        "",
                                                        "Webcam object",
                                                        CHEESE_TYPE_WEBCAM,
                                                        G_PARAM_READABLE | G_PARAM_WRITABLE));
}

CheesePrefsResolutionCombo *
cheese_prefs_resolution_combo_new (GtkWidget *combo_box, CheeseWebcam *webcam,
                                   const gchar *x_resolution_key,
                                   const gchar *y_resolution_key,
                                   unsigned int max_x_resolution,
                                   unsigned int max_y_resolution)
{
  CheesePrefsResolutionCombo        *self;
  GtkCellRenderer                   *renderer;
  CheesePrefsResolutionComboPrivate *priv;

  self = g_object_new (CHEESE_TYPE_PREFS_RESOLUTION_COMBO,
                       "widget", combo_box,
                       "webcam", webcam,
                       "x_resolution_key", x_resolution_key,
                       "y_resolution_key", y_resolution_key,
                       NULL);

  if (max_x_resolution > 0 &&
      max_y_resolution > 0)
  {
    g_object_set (self,
                  "max_x_resolution", max_x_resolution,
                  "max_y_resolution", max_y_resolution,
                  NULL);
  }

  priv = CHEESE_PREFS_RESOLUTION_COMBO_GET_PRIVATE (self);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), renderer, FALSE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo_box), renderer, "text",
                                 COL_NAME);
  gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box),
                           GTK_TREE_MODEL (priv->list_store));

  return self;
}

CheeseVideoFormat *
cheese_prefs_resolution_combo_get_selected_format (CheesePrefsResolutionCombo *resolution_combo)
{
  CheesePrefsResolutionComboPrivate *priv = CHEESE_PREFS_RESOLUTION_COMBO_GET_PRIVATE (resolution_combo);

  return priv->selected_format;
}
