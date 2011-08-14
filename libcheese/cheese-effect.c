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

enum
{
  PROP_O,
  PROP_NAME,
  PROP_PIPELINE_DESC,
  PROP_CONTROL_VALVE
};

G_DEFINE_TYPE (CheeseEffect, cheese_effect, G_TYPE_OBJECT)

#define CHEESE_EFFECT_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHEESE_TYPE_EFFECT, CheeseEffectPrivate))

typedef struct _CheeseEffectPrivate CheeseEffectPrivate;

struct _CheeseEffectPrivate
{
  char *name;
  char *pipeline_desc;
  GstElement *control_valve;
};

static void
cheese_effect_get_property (GObject *object, guint property_id,
                            GValue *value, GParamSpec *pspec)
{
  CheeseEffectPrivate *priv = CHEESE_EFFECT_GET_PRIVATE (object);

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
  CheeseEffectPrivate *priv = CHEESE_EFFECT_GET_PRIVATE (object);

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
cheese_effect_class_init (CheeseEffectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CheeseEffectPrivate));

  object_class->get_property = cheese_effect_get_property;
  object_class->set_property = cheese_effect_set_property;

  g_object_class_install_property (object_class, PROP_NAME,
                                   g_param_spec_string ("name",
                                                        NULL,
                                                        NULL,
                                                        "",
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_PIPELINE_DESC,
                                   g_param_spec_string ("pipeline_desc",
                                                        NULL,
                                                        NULL,
                                                        "",
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_CONTROL_VALVE,
                                   g_param_spec_object ("control_valve",
                                                        NULL,
                                                        NULL,
                                                        GST_TYPE_ELEMENT,
                                                        G_PARAM_READWRITE));
}

gboolean
cheese_effect_is_preview_connected (CheeseEffect *self)
{
  CheeseEffectPrivate *priv = CHEESE_EFFECT_GET_PRIVATE (self);

  return priv->control_valve != NULL;
}

void
cheese_effect_enable_preview (CheeseEffect *self)
{
  CheeseEffectPrivate *priv = CHEESE_EFFECT_GET_PRIVATE (self);

  g_object_set (G_OBJECT (priv->control_valve), "drop", FALSE, NULL);
}

void
cheese_effect_disable_preview (CheeseEffect *self)
{
  CheeseEffectPrivate *priv = CHEESE_EFFECT_GET_PRIVATE (self);

  g_object_set (G_OBJECT (priv->control_valve), "drop", TRUE, NULL);
}

static void
cheese_effect_init (CheeseEffect *self)
{
}

CheeseEffect *
cheese_effect_new (void)
{
  return g_object_new (CHEESE_TYPE_EFFECT, NULL);
}

/**
 * cheese_effect_load_from_file: load effect from file
 * @fname: (type filename): name of the file containing effect specification
 *
 * Returns: (transfer full): a #CheeseEffect
 */
CheeseEffect*
cheese_effect_load_from_file (const gchar *fname)
{
  const gchar  *GROUP_NAME = "Effect";
  gchar        *name, *desc;
  GError       *err = NULL;
  CheeseEffect *eff = NULL;
  GKeyFile     *kf = g_key_file_new ();

  g_key_file_load_from_file (kf, fname, G_KEY_FILE_NONE, &err);
  if (err != NULL)
    goto err_kf_load;

  name = g_key_file_get_locale_string (kf, GROUP_NAME, "Name", NULL, &err);
  if (err != NULL)
    goto err_name;

  desc = g_key_file_get_string (kf, GROUP_NAME, "PipelineDescription", &err);
  if (err != NULL)
    goto err_desc;

  g_key_file_free (kf);

  eff = cheese_effect_new ();
  g_object_set (eff, "name", name, NULL);
  g_object_set (eff, "pipeline-desc", desc, NULL);
  g_free (name);
  g_free (desc);

  return eff;

err_desc:
    g_free (name);
err_name:
err_kf_load:
    g_key_file_free (kf);
    g_warning ("CheeseEffect: couldn't load file %s: %s", fname, err->message);
    g_clear_error (&err);
    return NULL;
}

/* Returns list of effects loaded from files from @directory.
   Only parses files ending with the '.effects' extension */
static GList*
cheese_effect_load_effects_from_directory (const gchar* directory)
{
  gboolean retval;
  GError   *err = NULL;
  GDir     *dir = NULL;
  GList    *list = NULL;

  retval = g_file_test (directory, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR);
  if (!retval)
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
    const gchar  *fname = g_dir_read_name (dir);

    /* no more files */
    if (fname == NULL)
      break;

    if (!g_str_has_suffix (fname, ".effect"))
      continue;

    abs_path = g_build_filename (directory, fname, NULL);
    effect = cheese_effect_load_from_file (abs_path);
    if (effect != NULL)
      list = g_list_append (list, effect);
    g_free (abs_path);
  }
  g_dir_close (dir);

  return list;
}

static GList*
cheese_effect_load_effects_from_subdirectory (const gchar* directory,
                                              const gchar* subdirectory)
{
  GList *list;
  gchar *path = g_build_filename (directory, subdirectory, NULL);
  list = cheese_effect_load_effects_from_directory (path);
  g_free (path);
  return list;
}

/**
 * cheese_effect_load_effects: load effects from known standard directories.
 *
 * Returns: (element-type Cheese.Effect) (transfer full): List of #CheeseEffect
 */
GList*
cheese_effect_load_effects ()
{
  const gchar *const*data_dirs, *dir;
  GList *ret = NULL, *l;

  dir = g_get_user_data_dir (); /* value returned owned by GLib */
  l = cheese_effect_load_effects_from_subdirectory (dir, "gnome-video-effects");
  ret = g_list_concat (ret, l);

  data_dirs = g_get_system_data_dirs (); /* value returned owned by GLib */
  if (!data_dirs)
    return ret;

  for (; *data_dirs; data_dirs++)
  {
    dir = *data_dirs;
    l = cheese_effect_load_effects_from_subdirectory (dir, "gnome-video-effects");
    ret = g_list_concat (ret, l);
  }

  return ret;
}
