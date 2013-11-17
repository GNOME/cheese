/*
 * Copyright © 2010 Yuvaraj Pandian T <yuvipanda@yuvi.in>
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

#include <gst/gst.h>

#include "cheese-effect.h"

/**
 * SECTION:cheese-effect
 * @short_description: An effect to apply to a video capture stream
 * @stability: Unstable
 * @include: cheese/cheese-effect.h
 *
 * #CheeseEffect provides an abstraction of an effect to apply to a stream
 * from a video capture device.
 */

enum
{
  PROP_O,
  PROP_NAME,
  PROP_PIPELINE_DESC,
  PROP_CONTROL_VALVE,
  PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

struct _CheeseEffectPrivate
{
  gchar *name;
  gchar *pipeline_desc;
  GstElement *control_valve;
};

G_DEFINE_TYPE_WITH_PRIVATE (CheeseEffect, cheese_effect, G_TYPE_OBJECT)

static void
cheese_effect_get_property (GObject *object, guint property_id,
                            GValue *value, GParamSpec *pspec)
{
  CheeseEffectPrivate *priv = CHEESE_EFFECT (object)->priv;

  switch (property_id)
  {
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;
    case PROP_PIPELINE_DESC:
      g_value_set_string (value, priv->pipeline_desc);
      break;
    case PROP_CONTROL_VALVE:
      g_value_set_object (value, priv->control_valve);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
cheese_effect_set_property (GObject *object, guint property_id,
                            const GValue *value, GParamSpec *pspec)
{
  CheeseEffectPrivate *priv = CHEESE_EFFECT (object)->priv;

  switch (property_id)
  {
    case PROP_NAME:
      g_free (priv->name);
      priv->name = g_value_dup_string (value);
      break;
    case PROP_PIPELINE_DESC:
      g_free (priv->pipeline_desc);
      priv->pipeline_desc = g_value_dup_string (value);
      break;
    case PROP_CONTROL_VALVE:
      if (priv->control_valve != NULL)
        g_object_unref (G_OBJECT (priv->control_valve));
      priv->control_valve = GST_ELEMENT (g_value_get_object (value));
      g_object_ref (G_OBJECT (priv->control_valve));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
cheese_effect_finalize (GObject *object)
{
    CheeseEffect *effect;
    CheeseEffectPrivate *priv;

    effect = CHEESE_EFFECT (object);
    priv = effect->priv;

    g_clear_pointer (&priv->name, g_free);
    g_clear_pointer (&priv->pipeline_desc, g_free);
    g_clear_pointer (&priv->control_valve, gst_object_unref);

    G_OBJECT_CLASS (cheese_effect_parent_class)->finalize (object);
}

static void
cheese_effect_class_init (CheeseEffectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = cheese_effect_get_property;
  object_class->set_property = cheese_effect_set_property;
  object_class->finalize = cheese_effect_finalize;

  /**
   * CheeseEffect:name:
   *
   * Name of the effect, for display in a UI.
   */
  properties[PROP_NAME] = g_param_spec_string ("name",
                                               "Name",
                                               "Name of the effect",
                                               "",
                                               G_PARAM_READWRITE |
                                               G_PARAM_CONSTRUCT_ONLY |
                                               G_PARAM_STATIC_STRINGS);

  /**
   * CheeseEffect:pipeline-desc:
   *
   * Description of the GStreamer pipeline associated with the effect.
   */
  properties[PROP_PIPELINE_DESC] = g_param_spec_string ("pipeline-desc",
                                                        "Pipeline description",
                                                        "Description of the GStreamer pipeline associated with the effect",
                                                        "",
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS);

  /**
   * CheeseEffect:control-valve:
   *
   * If the control valve is active, then the effect is currently connected to
   * a video stream, for previews.
   */
  properties[PROP_CONTROL_VALVE] = g_param_spec_object ("control-valve",
                                                        "Control valve",
                                                        "If the control valve is active, the effect is connected to a video stream",
                                                        GST_TYPE_ELEMENT,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST, properties);
}


/**
 * cheese_effect_get_name:
 * @effect: a #CheeseEffect
 *
 * Get the human-readable name of the @effect.
 *
 * Returns: (transfer none): the human-readable name of the effect.
 */
const gchar *
cheese_effect_get_name (CheeseEffect *effect)
{
  g_return_val_if_fail (CHEESE_IS_EFFECT (effect), NULL);

  return effect->priv->name;
}

/**
 * cheese_effect_get_pipeline_desc:
 * @effect: a #CheeseEffect
 *
 * Get the Gstreamer pipeline description of the @effect.
 *
 * Returns: (transfer none): the Gstreamer pipeline description of the effect.
 */
const gchar *
cheese_effect_get_pipeline_desc (CheeseEffect *effect)
{
  g_return_val_if_fail (CHEESE_IS_EFFECT (effect), NULL);

  return effect->priv->pipeline_desc;
}

/**
 * cheese_effect_is_preview_connected:
 * @effect: a #CheeseEffect
 *
 * Get whether the @effect is connected to a video stream, for previews.
 *
 * Returns: %TRUE if the preview is connected, %FALSE otherwise.
 */
gboolean
cheese_effect_is_preview_connected (CheeseEffect *effect)
{
  g_return_val_if_fail (CHEESE_IS_EFFECT (effect), FALSE);

  return effect->priv->control_valve != NULL;
}

/**
 * cheese_effect_enable_preview:
 * @effect: the #CheeseEffect to enable the preview of
 *
 * Enable the preview of a #CheeseEffect.
 */
void
cheese_effect_enable_preview (CheeseEffect *effect)
{
  g_return_if_fail (CHEESE_IS_EFFECT (effect));

  g_object_set (G_OBJECT (effect->priv->control_valve), "drop", FALSE, NULL);
}

/**
 * cheese_effect_disable_preview:
 * @effect: the #CheeseEffect to disable the preview of
 *
 * Disable the preview of a #CheeseEffect.
 */
void
cheese_effect_disable_preview (CheeseEffect *effect)
{
  g_return_if_fail (CHEESE_IS_EFFECT (effect));

  g_object_set (G_OBJECT (effect->priv->control_valve), "drop", TRUE, NULL);
}

static void
cheese_effect_init (CheeseEffect *self)
{
  self->priv = cheese_effect_get_instance_private (self);
}

/**
 * cheese_effect_new:
 * @name: name of the effect
 * @pipeline_desc: GStreamer pipeline of the new effect
 *
 * Create a new #CheeseEffect.
 *
 * Returns: (transfer full): a new #CheeseEffect
 */
CheeseEffect *
cheese_effect_new (const gchar *name, const gchar *pipeline_desc)
{
  return g_object_new (CHEESE_TYPE_EFFECT,
                       "name", name,
                       "pipeline-desc", pipeline_desc,
                       NULL);
}

/**
 * cheese_effect_load_from_file:
 * @filename: (type filename): name of the file containing the effect
 * specification
 *
 * Load effect from file.
 *
 * Returns: (transfer full): a #CheeseEffect, or %NULL on error
 */
CheeseEffect*
cheese_effect_load_from_file (const gchar *filename)
{
  const gchar GROUP_NAME[] = "Effect";
  gchar        *name, *desc;
  GError       *err = NULL;
  CheeseEffect *effect = NULL;
  GKeyFile     *keyfile = g_key_file_new ();

  g_key_file_load_from_file (keyfile, filename, G_KEY_FILE_NONE, &err);
  if (err != NULL)
    goto err_keyfile_load;

  name = g_key_file_get_locale_string (keyfile, GROUP_NAME, "Name", NULL, &err);
  if (err != NULL)
    goto err_name;

  desc = g_key_file_get_string (keyfile, GROUP_NAME, "PipelineDescription", &err);
  if (err != NULL)
    goto err_desc;

  g_key_file_free (keyfile);

  effect = cheese_effect_new (name, desc);
  g_free (name);
  g_free (desc);

  return effect;

err_desc:
    g_free (name);
err_name:
err_keyfile_load:
    g_key_file_free (keyfile);
    g_warning ("CheeseEffect: couldn't load file %s: %s", filename, err->message);
    g_clear_error (&err);
    return NULL;
}

/*
 * cheese_effect_load_effects_from_directory:
 * @directory: the directory in which to search for effects
 *
 * Only parses files ending with the '.effects' extension.
 *
 * Returns: (element-type Cheese.Effect) (transfer full): list of effects
 * loaded from files from @directory, or %NULL if any errors were encountered
 */
static GList*
cheese_effect_load_effects_from_directory (const gchar* directory)
{
  gboolean is_dir;
  GError  *err = NULL;
  GDir    *dir = NULL;
  GList   *list = NULL;

  is_dir = g_file_test (directory, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR);
  if (!is_dir)
    return NULL;

  dir = g_dir_open (directory, (guint) 0, &err);
  if (err != NULL)
  {
    g_warning ("CheeseEffect: g_dir_open: %s\n", err->message);
    g_clear_error (&err);
    return NULL;
  }

  while (TRUE) {
    CheeseEffect *effect;
    gchar        *abs_path;
    const gchar  *filename = g_dir_read_name (dir);

    /* no more files */
    if (filename == NULL)
      break;

    if (!g_str_has_suffix (filename, ".effect"))
      continue;

    abs_path = g_build_filename (directory, filename, NULL);
    effect = cheese_effect_load_from_file (abs_path);
    if (effect != NULL)
      list = g_list_prepend (list, effect);
    g_free (abs_path);
  }
  g_dir_close (dir);

  return g_list_reverse (list);
}

/*
 * cheese_effect_load_effects_from_subdirectory:
 * @directory: directory from under which to load effects
 *
 * Get a list of effects from the gnome-video-effects subdirectory under
 * @directory.
 *
 * Returns: (element-type Cheese.Effect) (transfer full): a list of
 * #CheeseEffect, or %NULL upon failure
 */
static GList*
cheese_effect_load_effects_from_subdirectory (const gchar* directory)
{
  GList *list;
  gchar *path = g_build_filename (directory, "gnome-video-effects", NULL);
  list = cheese_effect_load_effects_from_directory (path);
  g_free (path);
  return list;
}

/**
 * cheese_effect_load_effects:
 *
 * Load effects from standard directories, including the user's data directory.
 *
 * Returns: (element-type Cheese.Effect) (transfer full): a list of
 * #CheeseEffect, or %NULL if no effects could be found
 */
GList*
cheese_effect_load_effects ()
{
  const gchar * const *data_dirs, *dir;
  GList *effect_list = NULL, *l;

  dir = g_get_user_data_dir (); /* value returned owned by GLib */
  l = cheese_effect_load_effects_from_subdirectory (dir);
  effect_list = g_list_concat (effect_list, l);

  data_dirs = g_get_system_data_dirs (); /* value returned owned by GLib */
  if (!data_dirs)
    return effect_list;

  while (*data_dirs)
  {
    dir = *data_dirs;
    l = cheese_effect_load_effects_from_subdirectory (dir);
    effect_list = g_list_concat (effect_list, l);
    data_dirs++;
  }

  return effect_list;
}
