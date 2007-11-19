/*
 * Copyright (C) 2007 daniel g. siegel <dgsiegel@gmail.com>
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
#include <config.h>
#endif

#include <glib.h>
#include <stdlib.h>

#include <gconf/gconf.h>
#include <gconf/gconf-client.h>

#include "cheese-gconf.h"

#define CHEESE_GCONF_PREFIX   "/apps/cheese"

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

  switch (prop_id) 
  {
    case GCONF_PROP_PATH:
      g_value_set_string (value, gconf_client_get_string (priv->client,
                                                          CHEESE_GCONF_PREFIX "/path",
                                                          NULL));
      break;
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

  switch (prop_id) 
  {
    case GCONF_PROP_PATH:
      gconf_client_set_string (priv->client,
                               CHEESE_GCONF_PREFIX "/path",
                               g_value_get_string (value),
                               NULL);
      break;
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

  g_object_class_install_property (object_class, GCONF_PROP_PATH,
                                   g_param_spec_string ("gconf_prop_path",
                                                        NULL,
                                                        NULL,
                                                        "",
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class, GCONF_PROP_WEBCAM,
                                   g_param_spec_string ("gconf_prop_webcam",
                                                        NULL,
                                                        NULL,
                                                        "",
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class, GCONF_PROP_COUNTDOWN,
                                   g_param_spec_boolean ("gconf_prop_countdown",
                                                         NULL,
                                                         NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (CheeseGConfPrivate));
}

static void
cheese_gconf_init (CheeseGConf *gconf)
{
  CheeseGConfPrivate* priv = CHEESE_GCONF_GET_PRIVATE (gconf);
  priv->client = gconf_client_get_default();
  char *gconf_prop_path;
  char *gconf_prop_webcam;
  gboolean gconf_prop_countdown;

  // FIXME: add notification
  //gconf_client_add_dir(client, CHEESE_GCONF_PREFIX, GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);

  gconf_prop_path = gconf_client_get_string (priv->client,
                                             CHEESE_GCONF_PREFIX "/path",
                                             NULL);
  gconf_prop_webcam = gconf_client_get_string (priv->client,
                                               CHEESE_GCONF_PREFIX "/webcam",
                                               NULL);
  gconf_prop_countdown = gconf_client_get_bool (priv->client,
                                                CHEESE_GCONF_PREFIX "/countdown",
                                                NULL);

  // FIXME: provide a schema file
  if (gconf_prop_path == NULL || gconf_prop_webcam == NULL)
  {
    g_print("\n ***************************************************************************\n");
    g_print("  Remember that to run cheese you must set all those GConf keys; \n");
    g_print("    '" CHEESE_GCONF_PREFIX "/path' \n");
    g_print("    '" CHEESE_GCONF_PREFIX "/countdown' \n");
    g_print("    '" CHEESE_GCONF_PREFIX "/webcam' \n");
    g_print("\n  To set the keys, run;\n\n");
    g_print("    gconftool-2 -s -t string " CHEESE_GCONF_PREFIX "/path \'.gnome2/cheese/media\'\n");
    g_print("    gconftool-2 -s -t bool " CHEESE_GCONF_PREFIX "/countdown \'false\'\n");
    g_print("    gconftool-2 -s -t string " CHEESE_GCONF_PREFIX "/webcam \'/dev/video0\'\n");
    g_print("\n ***************************************************************************\n\n");

  }
}

CheeseGConf * 
cheese_gconf_new ()
{
  CheeseGConf *gconf;

  gconf = g_object_new (CHEESE_TYPE_GCONF, NULL);  
  return gconf;
}
