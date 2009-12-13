/*
 * Copyright © 2007,2008 Jaap Haitsma <jaap@haitsma.org>
 * Copyright © 2007-2009 daniel g. siegel <dgsiegel@gnome.org>
 * Copyright © 2008 Ryan Zeigler <zeiglerr@gmail.com>
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

#ifdef HAVE_CONFIG_H
  #include <cheese-config.h>
#endif

#include <glib.h>
#include <glib/gi18n.h>

#include "cheese-camera-device.h"

G_DEFINE_TYPE (CheeseCameraDevice, cheese_camera_device, G_TYPE_OBJECT)

#define CHEESE_CAMERA_DEVICE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHEESE_TYPE_CAMERA_DEVICE, CheeseCameraDevicePrivate))

static gchar *supported_formats[] =
{
  "video/x-raw-rgb",
  "video/x-raw-yuv",
  NULL
};

enum
{
  PROP_0,
  PROP_NAME,
  PROP_FILE,
  PROP_ID,
  PROP_SRC,
  PROP_CAPS
};

typedef struct
{
  gchar *device;
  gchar *id;
  gchar *src;
  gchar *name;

  GstCaps *caps;

  GList *formats;
} CheeseCameraDevicePrivate;


/* CheeseVideoFormat */

static CheeseVideoFormat*
cheese_video_format_copy (const CheeseVideoFormat *format)
{
  return g_slice_dup (CheeseVideoFormat, format);
}

static void
cheese_video_format_free (CheeseVideoFormat *format)
{
  if (G_LIKELY (format != NULL))
    g_slice_free (CheeseVideoFormat, format);
}

GType
cheese_video_format_get_type (void)
{
  static GType our_type = 0;

  if (G_UNLIKELY (our_type == 0))
    our_type =
      g_boxed_type_register_static ("CheeseVideoFormat",
                                    (GBoxedCopyFunc) cheese_video_format_copy,
                                    (GBoxedFreeFunc) cheese_video_format_free);
  return our_type;
}

/* the rest */

static gint
compare_formats (gconstpointer a, gconstpointer b)
{
  const CheeseVideoFormat *c = a;
  const CheeseVideoFormat *d = b;

  /* descending sort for rectangle area */
  return (d->width*d->height - c->width*c->height);
}

static
GstCaps *cheese_webcam_device_filter_caps (CheeseCameraDevice *device, const GstCaps *caps, GStrv formats)
{
/*  CheeseCameraDevicePrivate *priv =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device); */
  gchar *formats_string = g_strjoinv ("; ", formats);
  GstCaps *filter = gst_caps_from_string (formats_string);
  GstCaps *allowed = gst_caps_intersect (caps, filter);

  g_free (formats_string);
  gst_caps_unref (filter);

  return allowed;
}

static void
cheese_camera_device_add_format (CheeseCameraDevice *device, CheeseVideoFormat *format)
{
  CheeseCameraDevicePrivate *priv =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device);
  GList *l;
  for (l = priv->formats; l != NULL; l = l->next)
  {
    CheeseVideoFormat *item = l->data;
    if ((item != NULL) &&
        (item->width == format->width) &&
        (item->height == format->height)) return;
  }

  priv->formats = g_list_append (priv->formats, format);
}

static void
free_format_list (CheeseCameraDevice *device)
{
  CheeseCameraDevicePrivate *priv =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device);

  GList *l;
  for (l = priv->formats; l != NULL; l = l->next)
    g_free (l->data);
  g_list_free (priv->formats);
  priv->formats = NULL;
}

static void
cheese_webcam_device_update_format_table (CheeseCameraDevice *device)
{
  CheeseCameraDevicePrivate *priv =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device);

  int i;
  int num_structures;

  /* FIXME: limit maximum framerate somehow, we don't want those
   * stupid 100Hz webcams that slow down the whole pipeline for a
   * merely perceivable refresh reate gain. I'd say let's throw away
   * everything over 30/1 */

  free_format_list (device);

  num_structures = gst_caps_get_size (priv->caps);
  for (i = 0; i < num_structures; i++)
  {
    GstStructure *structure;
    const GValue *width, *height;
    structure = gst_caps_get_structure (priv->caps, i);

    width  = gst_structure_get_value (structure, "width");
    height = gst_structure_get_value (structure, "height");

    if (G_VALUE_HOLDS_INT (width))
    {
      CheeseVideoFormat *format = g_new0 (CheeseVideoFormat, 1);

      gst_structure_get_int (structure, "width", &(format->width));
      gst_structure_get_int (structure, "height", &(format->height));
      cheese_camera_device_add_format (device, format);
    }
    else if (GST_VALUE_HOLDS_INT_RANGE (width))
    {
      int min_width, max_width, min_height, max_height;
      int cur_width, cur_height;

      min_width  = gst_value_get_int_range_min (width);
      max_width  = gst_value_get_int_range_max (width);
      min_height = gst_value_get_int_range_min (height);
      max_height = gst_value_get_int_range_max (height);

      cur_width  = min_width;
      cur_height = min_height;

      /* Gstreamer will sometimes give us a range with min_xxx == max_xxx,
       * we use <= here (and not below) to make this work */
      while (cur_width <= max_width && cur_height <= max_height)
      {
        CheeseVideoFormat *format = g_new0 (CheeseVideoFormat, 1);

        format->width = cur_width;
        format->height = cur_height;

        cheese_camera_device_add_format (device, format);

        cur_width  *= 2;
        cur_height *= 2;
      }

      cur_width  = max_width;
      cur_height = max_height;
      while (cur_width > min_width && cur_height > min_height)
      {
        CheeseVideoFormat *format = g_new0 (CheeseVideoFormat, 1);

        format->width = cur_width;
        format->height = cur_height;

        cheese_camera_device_add_format (device, format);

        cur_width  /= 2;
        cur_height /= 2;
      }
    }
    else
    {
      g_critical ("GValue type %s, cannot be handled for resolution width", G_VALUE_TYPE_NAME (width));
    }
  }
}

static void
cheese_camera_device_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  CheeseCameraDevice *device = CHEESE_CAMERA_DEVICE (object);
  CheeseCameraDevicePrivate *priv =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device);

  switch (prop_id) {
  case PROP_NAME:
    g_value_set_string (value, priv->name);
    break;
  case PROP_FILE:
    g_value_set_string (value, priv->device);
    break;
  case PROP_ID:
    g_value_set_string (value, priv->id);
    break;
  case PROP_SRC:
    g_value_set_string (value, priv->src);
    break;
  case PROP_CAPS:
    gst_value_set_caps (value, priv->caps);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
cheese_camera_device_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  CheeseCameraDevice *device = CHEESE_CAMERA_DEVICE (object);
  CheeseCameraDevicePrivate *priv =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device);
  const GstCaps *new_caps;

  switch (prop_id) {
  case PROP_NAME:
    g_free (priv->name);
    priv->name = g_value_dup_string (value);
    break;
  case PROP_ID:
    g_free (priv->id);
    priv->id = g_value_dup_string (value);
    break;
  case PROP_FILE:
    g_free (priv->device);
    priv->device = g_value_dup_string (value);
    break;
  case PROP_SRC:
    g_free (priv->src);
    priv->src = g_value_dup_string (value);
    break;
  case PROP_CAPS:
    new_caps = gst_value_get_caps (value);
    if (new_caps != NULL) {
      gst_caps_unref (priv->caps);
      priv->caps = cheese_webcam_device_filter_caps (device, new_caps, supported_formats);
      cheese_webcam_device_update_format_table (device);
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
cheese_camera_device_finalize (GObject *object)
{
  CheeseCameraDevice *device = CHEESE_CAMERA_DEVICE (object);
  CheeseCameraDevicePrivate *priv =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device);

  g_free (priv->device);
  g_free (priv->id);
  g_free (priv->src);
  g_free (priv->name);

  gst_caps_unref (priv->caps);
  free_format_list (device);
}

static void
cheese_camera_device_class_init (CheeseCameraDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = cheese_camera_device_finalize;
  object_class->get_property = cheese_camera_device_get_property;
  object_class->set_property = cheese_camera_device_set_property;

  g_object_class_install_property (object_class, PROP_NAME,
                                   g_param_spec_string ("name",
                                                        NULL, NULL, NULL,
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_FILE,
                                   g_param_spec_string ("device-file",
                                                        NULL, NULL, NULL,
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_ID,
                                   g_param_spec_string ("device-id",
                                                        NULL, NULL, NULL,
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SRC,
                                   g_param_spec_string ("src",
                                                        NULL, NULL, NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_CAPS,
                                   g_param_spec_boxed ("caps",
                                                       NULL, NULL,
                                                       GST_TYPE_CAPS,
                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (CheeseCameraDevicePrivate));
}

static void
cheese_camera_device_init (CheeseCameraDevice *device)
{
  CheeseCameraDevicePrivate *priv =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device);
  priv->device = NULL;
  priv->id = NULL;
  priv->src = NULL;
  priv->name = g_strdup(_("Unknown device"));
  priv->caps = gst_caps_new_empty ();

  priv->formats = NULL;
}

CheeseCameraDevice *cheese_camera_device_new (void)
{
  return g_object_new (CHEESE_TYPE_CAMERA_DEVICE, NULL);
}


/* public methods */

GList *cheese_camera_device_get_format_list (CheeseCameraDevice *device)
{
  CheeseCameraDevicePrivate *priv =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device);

  return g_list_sort (g_list_copy (priv->formats), compare_formats);
}

const gchar *cheese_camera_device_get_name (CheeseCameraDevice *device)
{
  CheeseCameraDevicePrivate *priv =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device);

  return priv->name;
}
const gchar *cheese_camera_device_get_id (CheeseCameraDevice *device)
{
  CheeseCameraDevicePrivate *priv =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device);

  return priv->id;
}
const gchar *cheese_camera_device_get_src (CheeseCameraDevice *device)
{
  CheeseCameraDevicePrivate *priv =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device);

  return priv->src;
}
const gchar *cheese_camera_device_get_device_file (CheeseCameraDevice *device)
{
  CheeseCameraDevicePrivate *priv =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device);

  return priv->device;
}

CheeseVideoFormat *cheese_camera_device_get_best_format (CheeseCameraDevice *device)
{
  return (CheeseVideoFormat *) cheese_camera_device_get_format_list (device)->data;
}

GstCaps *cheese_camera_device_get_caps_for_format (CheeseCameraDevice *device,
                                                   CheeseVideoFormat *format)
{
  CheeseCameraDevicePrivate *priv =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device);
  GstCaps *caps;
  gint i;

  caps = gst_caps_new_simple (supported_formats[0],
                              "width", G_TYPE_INT,
                              format->width,
                              "height", G_TYPE_INT,
                              format->height,
                              NULL);

  for (i = 1; i < g_strv_length (supported_formats); i++)
  {
    gst_caps_append (caps,
                     gst_caps_new_simple (supported_formats[i],
                                          "width", G_TYPE_INT,
                                          format->width,
                                          "height", G_TYPE_INT,
                                          format->height,
                                          NULL));
  }

  if (gst_caps_can_intersect (caps, priv->caps))
    return caps;
  else
    return gst_caps_new_empty ();
}
