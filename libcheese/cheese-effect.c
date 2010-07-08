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

struct _CheeseEffectPrivate {
  char *name;
  char *pipeline_desc;
  GstElement *control_valve;
};

static void
cheese_effect_get_property (GObject *object, guint property_id,
			    GValue *value, GParamSpec *pspec)
{
  CheeseEffectPrivate *priv = CHEESE_EFFECT_GET_PRIVATE (object);
  
  switch (property_id) {
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
  
  switch (property_id) {
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

CheeseEffect*
cheese_effect_new (void)
{
  return g_object_new (CHEESE_TYPE_EFFECT, NULL);
}
