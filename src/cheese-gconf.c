/*
 * Copyright © 2007,2008 daniel g. siegel <dgsiegel@gnome.org>
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

#ifdef HAVE_CONFIG_H
  #include <cheese-config.h>
#endif

#include <glib.h>
#include <stdlib.h>

#include <gconf/gconf.h>
#include <gconf/gconf-client.h>

#include "cheese-gconf.h"

#define CHEESE_GCONF_PREFIX "/apps/cheese"

G_DEFINE_TYPE (CheeseGConf, cheese_gconf, G_TYPE_OBJECT)

#define CHEESE_GCONF_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHEESE_TYPE_GCONF, CheeseGConfPrivate))

typedef struct
{
  GConfClient *client;
} CheeseGConfPrivate;

static void
cheese_gconf_get_property (GObject *object, guint prop_id, GValue *value,
                           GParamSpec *pspec)
{
  CheeseGConf *self;

  self = CHEESE_GCONF (object);
  CheeseGConfPrivate *priv = CHEESE_GCONF_GET_PRIVATE (self);

  char   *effects;
  GSList *list, *tmp;

  switch (prop_id)
  {
    case GCONF_PROP_COUNTDOWN:
      g_value_set_boolean (value, gconf_client_get_bool (priv->client,
                                                         CHEESE_GCONF_PREFIX "/countdown",
                                                         NULL));
      break;
    case GCONF_PROP_WEBCAM:
      g_value_set_string (value, gconf_client_get_string (priv->client,
                                                          CHEESE_GCONF_PREFIX "/webcam",
                                                          NULL));
      break;
    case GCONF_PROP_SELECTED_EFFECTS:
      effects = NULL;
      list    = gconf_client_get_list (priv->client,
                                       CHEESE_GCONF_PREFIX "/selected_effects",
                                       GCONF_VALUE_STRING,
                                       NULL);
      tmp = list;
      while (tmp != NULL)
      {
        char *str = tmp->data;
        int   j;
        str[0] = g_ascii_toupper (str[0]);
        for (j = 1; j < g_utf8_strlen (str, -1); j++)
        {
          if (str[j] == '-')
          {
            str[j]     = ' ';
            str[j + 1] = g_ascii_toupper (str[j + 1]);
          }
          else if (str[j] == '_')
          {
            str[j]     = '/';
            str[j + 1] = g_ascii_toupper (str[j + 1]);
          }
        }
        if (effects == NULL)
          effects = g_strdup (str);
        else
        {
          gchar *dummy = effects;
          effects = g_strjoin (",", effects, str, NULL);
          g_free (dummy);
        }

        g_free (tmp->data);
        tmp = g_slist_next (tmp);
      }
      g_value_set_string (value, effects);

      g_slist_free (list);
      g_slist_free (tmp);
      break;
    case GCONF_PROP_X_RESOLUTION:
      g_value_set_int (value, gconf_client_get_int (priv->client,
                                                    CHEESE_GCONF_PREFIX "/x_resolution",
                                                    NULL));
      break;
    case GCONF_PROP_Y_RESOLUTION:
      g_value_set_int (value, gconf_client_get_int (priv->client,
                                                    CHEESE_GCONF_PREFIX "/y_resolution",
                                                    NULL));
      break;
    case GCONF_PROP_BRIGHTNESS:
      if (!gconf_client_get (priv->client,
                             CHEESE_GCONF_PREFIX "/brightness",
                             NULL))
        g_value_set_double (value, G_PARAM_SPEC_DOUBLE (pspec)->default_value);
      else
        g_value_set_double (value, gconf_client_get_float (priv->client,
                                                           CHEESE_GCONF_PREFIX "/brightness",
                                                           NULL));
      break;
    case GCONF_PROP_CONTRAST:
      if (!gconf_client_get (priv->client,
                             CHEESE_GCONF_PREFIX "/contrast",
                             NULL))
        g_value_set_double (value, G_PARAM_SPEC_DOUBLE (pspec)->default_value);
      else
        g_value_set_double (value, gconf_client_get_float (priv->client,
                                                           CHEESE_GCONF_PREFIX "/contrast",
                                                           NULL));
      break;
    case GCONF_PROP_SATURATION:
      if (!gconf_client_get (priv->client,
                             CHEESE_GCONF_PREFIX "/saturation",
                             NULL))
        g_value_set_double (value, G_PARAM_SPEC_DOUBLE (pspec)->default_value);
      else
        g_value_set_double (value, gconf_client_get_float (priv->client,
                                                           CHEESE_GCONF_PREFIX "/saturation",
                                                           NULL));
      break;
    case GCONF_PROP_HUE:
      if (!gconf_client_get (priv->client,
                             CHEESE_GCONF_PREFIX "/hue",
                             NULL))
        g_value_set_double (value, G_PARAM_SPEC_DOUBLE (pspec)->default_value);
      else
        g_value_set_double (value, gconf_client_get_float (priv->client,
                                                           CHEESE_GCONF_PREFIX "/hue",
                                                           NULL));
      break;
    case GCONF_PROP_VIDEO_PATH:
      g_value_set_string (value, gconf_client_get_string (priv->client,
                                                          CHEESE_GCONF_PREFIX "/video_path",
                                                          NULL));
      break;
    case GCONF_PROP_PHOTO_PATH:
      g_value_set_string (value, gconf_client_get_string (priv->client,
                                                          CHEESE_GCONF_PREFIX "/photo_path",
                                                          NULL));
      break;
    case GCONF_PROP_ENABLE_DELETE:
      g_value_set_boolean (value, gconf_client_get_bool (priv->client,
                                                         CHEESE_GCONF_PREFIX "/enable_delete",
                                                         NULL));
      break;
    case GCONF_PROP_WIDE_MODE:
      g_value_set_boolean (value, gconf_client_get_bool (priv->client,
                                                         CHEESE_GCONF_PREFIX "/wide_mode",
                                                         NULL));
      break;
    case GCONF_PROP_BURST_DELAY:
      g_value_set_int (value, gconf_client_get_int (priv->client,
                                                    CHEESE_GCONF_PREFIX "/burst_delay",
                                                    NULL));
      break;
    case GCONF_PROP_BURST_REPEAT:
      g_value_set_int (value, gconf_client_get_int (priv->client,
                                                    CHEESE_GCONF_PREFIX "/burst_repeat",
                                                    NULL));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
cheese_gconf_set_property (GObject *object, guint prop_id, const GValue *value,
                           GParamSpec *pspec)
{
  CheeseGConf *self;

  self = CHEESE_GCONF (object);
  CheeseGConfPrivate *priv = CHEESE_GCONF_GET_PRIVATE (self);

  gchar **effects = NULL;
  GSList *list    = NULL;
  int     i;

  switch (prop_id)
  {
    case GCONF_PROP_COUNTDOWN:
      gconf_client_set_bool (priv->client,
                             CHEESE_GCONF_PREFIX "/countdown",
                             g_value_get_boolean (value),
                             NULL);
      break;
    case GCONF_PROP_WEBCAM:
      gconf_client_set_string (priv->client,
                               CHEESE_GCONF_PREFIX "/webcam",
                               g_value_get_string (value),
                               NULL);
      break;
    case GCONF_PROP_SELECTED_EFFECTS:

      if (g_value_get_string (value) == NULL)
      {
        list = NULL;
      }
      else
      {
        char *str = g_value_dup_string (value);
        int   j;
        for (j = 0; j < g_utf8_strlen (str, -1); j++)
        {
          if (g_ascii_isupper (str[j]))
          {
            str[j] = g_ascii_tolower (str[j]);
          }
          else if (str[j] == ' ')
          {
            str[j] = '-';
          }
          else if (str[j] == '/')
          {
            str[j] = '_';
          }
        }
        effects = g_strsplit (str, ",", 12);
        for (i = 0; effects[i] != NULL; i++)
        {
          list = g_slist_append (list, effects[i]);
        }
        g_free (str);
      }
      gconf_client_set_list (priv->client,
                             CHEESE_GCONF_PREFIX "/selected_effects",
                             GCONF_VALUE_STRING,
                             list,
                             NULL);
      g_slist_free (list);
      g_strfreev (effects);
      break;
    case GCONF_PROP_X_RESOLUTION:
      gconf_client_set_int (priv->client,
                            CHEESE_GCONF_PREFIX "/x_resolution",
                            g_value_get_int (value),
                            NULL);
      break;
    case GCONF_PROP_Y_RESOLUTION:
      gconf_client_set_int (priv->client,
                            CHEESE_GCONF_PREFIX "/y_resolution",
                            g_value_get_int (value),
                            NULL);
      break;
    case GCONF_PROP_BRIGHTNESS:
      gconf_client_set_float (priv->client,
                              CHEESE_GCONF_PREFIX "/brightness",
                              g_value_get_double (value),
                              NULL);
      break;
    case GCONF_PROP_CONTRAST:
      gconf_client_set_float (priv->client,
                              CHEESE_GCONF_PREFIX "/contrast",
                              g_value_get_double (value),
                              NULL);
      break;
    case GCONF_PROP_SATURATION:
      gconf_client_set_float (priv->client,
                              CHEESE_GCONF_PREFIX "/saturation",
                              g_value_get_double (value),
                              NULL);
      break;
    case GCONF_PROP_HUE:
      gconf_client_set_float (priv->client,
                              CHEESE_GCONF_PREFIX "/hue",
                              g_value_get_double (value),
                              NULL);
      break;
    case GCONF_PROP_VIDEO_PATH:
      gconf_client_set_string (priv->client,
                               CHEESE_GCONF_PREFIX "/video_path",
                               g_value_get_string (value),
                               NULL);
      break;
    case GCONF_PROP_PHOTO_PATH:
      gconf_client_set_string (priv->client,
                               CHEESE_GCONF_PREFIX "/photo_path",
                               g_value_get_string (value),
                               NULL);
      break;
    case GCONF_PROP_ENABLE_DELETE:
      gconf_client_set_bool (priv->client,
                             CHEESE_GCONF_PREFIX "/enable_delete",
                             g_value_get_boolean (value),
                             NULL);
      break;
    case GCONF_PROP_WIDE_MODE:
      gconf_client_set_bool (priv->client,
                             CHEESE_GCONF_PREFIX "/wide_mode",
                             g_value_get_boolean (value),
                             NULL);
      break;
    case GCONF_PROP_BURST_DELAY:
      gconf_client_set_int (priv->client,
                            CHEESE_GCONF_PREFIX "/burst_delay",
                            g_value_get_int (value),
                            NULL);
      break;
    case GCONF_PROP_BURST_REPEAT:
      gconf_client_set_int (priv->client,
                            CHEESE_GCONF_PREFIX "/burst_repeat",
                            g_value_get_int (value),
                            NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
cheese_gconf_finalize (GObject *object)
{
  CheeseGConf *gconf;

  gconf = CHEESE_GCONF (object);
  CheeseGConfPrivate *priv = CHEESE_GCONF_GET_PRIVATE (gconf);

  g_object_unref (priv->client);
  G_OBJECT_CLASS (cheese_gconf_parent_class)->finalize (object);
}

static void
cheese_gconf_class_init (CheeseGConfClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = cheese_gconf_finalize;

  object_class->get_property = cheese_gconf_get_property;
  object_class->set_property = cheese_gconf_set_property;

  g_object_class_install_property (object_class, GCONF_PROP_COUNTDOWN,
                                   g_param_spec_boolean ("gconf_prop_countdown",
                                                         NULL,
                                                         NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE));
  g_object_class_install_property (object_class, GCONF_PROP_WEBCAM,
                                   g_param_spec_string ("gconf_prop_webcam",
                                                        NULL,
                                                        NULL,
                                                        "",
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class, GCONF_PROP_SELECTED_EFFECTS,
                                   g_param_spec_string ("gconf_prop_selected_effects",
                                                        NULL,
                                                        NULL,
                                                        "",
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (object_class, GCONF_PROP_X_RESOLUTION,
                                   g_param_spec_int ("gconf_prop_x_resolution",
                                                     NULL,
                                                     NULL,
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (object_class, GCONF_PROP_Y_RESOLUTION,
                                   g_param_spec_int ("gconf_prop_y_resolution",
                                                     NULL,
                                                     NULL,
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (object_class, GCONF_PROP_BRIGHTNESS,
                                   g_param_spec_double ("gconf_prop_brightness",
                                                        NULL,
                                                        NULL,
                                                        -G_MAXFLOAT,
                                                        G_MAXFLOAT,
                                                        0.0,
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (object_class, GCONF_PROP_CONTRAST,
                                   g_param_spec_double ("gconf_prop_contrast",
                                                        NULL,
                                                        NULL,
                                                        0,
                                                        G_MAXFLOAT,
                                                        1.0,
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (object_class, GCONF_PROP_SATURATION,
                                   g_param_spec_double ("gconf_prop_saturation",
                                                        NULL,
                                                        NULL,
                                                        0,
                                                        G_MAXFLOAT,
                                                        1.0,
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (object_class, GCONF_PROP_HUE,
                                   g_param_spec_double ("gconf_prop_hue",
                                                        NULL,
                                                        NULL,
                                                        -G_MAXFLOAT,
                                                        G_MAXFLOAT,
                                                        0.0,
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (object_class, GCONF_PROP_VIDEO_PATH,
                                   g_param_spec_string ("gconf_prop_video_path",
                                                        NULL,
                                                        NULL,
                                                        "",
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (object_class, GCONF_PROP_PHOTO_PATH,
                                   g_param_spec_string ("gconf_prop_photo_path",
                                                        NULL,
                                                        NULL,
                                                        "",
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (object_class, GCONF_PROP_ENABLE_DELETE,
                                   g_param_spec_boolean ("gconf_prop_enable_delete",
                                                         NULL,
                                                         NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (object_class, GCONF_PROP_WIDE_MODE,
                                   g_param_spec_boolean ("gconf_prop_wide_mode",
                                                         NULL,
                                                         NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (object_class, GCONF_PROP_BURST_DELAY,
                                   g_param_spec_int ("gconf_prop_burst_delay",
                                                     NULL,
                                                     NULL,
                                                     200,  /* based on some experiments */
                                                     G_MAXINT,
                                                     1000,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (object_class, GCONF_PROP_BURST_REPEAT,
                                   g_param_spec_int ("gconf_prop_burst_repeat",
                                                     NULL,
                                                     NULL,
                                                     1,
                                                     G_MAXINT,
                                                     4,
                                                     G_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (CheeseGConfPrivate));
}

static void
cheese_gconf_init (CheeseGConf *gconf)
{
  CheeseGConfPrivate *priv = CHEESE_GCONF_GET_PRIVATE (gconf);

  priv->client = gconf_client_get_default ();
}

CheeseGConf *
cheese_gconf_new ()
{
  CheeseGConf *gconf;

  gconf = g_object_new (CHEESE_TYPE_GCONF, NULL);
  return gconf;
}
