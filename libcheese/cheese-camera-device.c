/*
 * Copyright © 2009 Filippo Argiolas <filippo.argiolas@gmail.com>
 * Copyright © 2007,2008 Jaap Haitsma <jaap@haitsma.org>
 * Copyright © 2007-2009 daniel g. siegel <dgsiegel@gnome.org>
 * Copyright © 2008 Ryan Zeigler <zeiglerr@gmail.com>
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
#include <gio/gio.h>

#include "cheese-camera-device.h"

static void     cheese_camera_device_initable_iface_init (GInitableIface  *iface);
static gboolean cheese_camera_device_initable_init       (GInitable       *initable,
                                                          GCancellable    *cancellable,
                                                          GError         **error);

G_DEFINE_TYPE_WITH_CODE (CheeseCameraDevice, cheese_camera_device, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
                                                cheese_camera_device_initable_iface_init))

#define CHEESE_CAMERA_DEVICE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHEESE_TYPE_CAMERA_DEVICE, \
                                                                          CheeseCameraDevicePrivate))

#define CHEESE_CAMERA_DEVICE_ERROR cheese_camera_device_error_quark ()

enum CheeseCameraDeviceError
{
  CHEESE_CAMERA_DEVICE_ERROR_UNKNOWN,
  CHEESE_CAMERA_DEVICE_ERROR_NOT_SUPPORTED,
  CHEESE_CAMERA_DEVICE_ERROR_UNSUPPORTED_CAPS,
  CHEESE_CAMERA_DEVICE_ERROR_FAILED_INITIALIZATION
};

GST_DEBUG_CATEGORY (cheese_camera_device_cat);
#define GST_CAT_DEFAULT cheese_camera_device_cat

static gchar *supported_formats[] = {
  "video/x-raw-rgb",
  "video/x-raw-yuv",
  NULL
};

/* FIXME: make this configurable */
#define CHEESE_MAXIMUM_RATE 30 /* fps */

enum
{
  PROP_0,
  PROP_NAME,
  PROP_FILE,
  PROP_ID,
  PROP_API
};

typedef struct
{
  gchar *device;
  gchar *id;
  const gchar *src;
  gchar *name;
  gint api;
  GstCaps *caps;
  GList *formats;

  GError *construct_error;
} CheeseCameraDevicePrivate;

GQuark
cheese_camera_device_error_quark (void)
{
  return g_quark_from_static_string ("cheese-camera-device-error-quark");
}

/* CheeseVideoFormat */

static CheeseVideoFormat *
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
  return (d->width * d->height - c->width * c->height);
}

static
GstCaps *
cheese_webcam_device_filter_caps (CheeseCameraDevice *device, const GstCaps *caps, GStrv formats)
{
/*  CheeseCameraDevicePrivate *priv =
 *  CHEESE_CAMERA_DEVICE_GET_PRIVATE (device); */
  GstCaps *filter;
  GstCaps *allowed;
  gint i;

  filter = gst_caps_new_simple (formats[0],
                                "framerate", GST_TYPE_FRACTION_RANGE,
                                0, 1, CHEESE_MAXIMUM_RATE, 1,
                                NULL);

  for (i = 1; i < g_strv_length (formats); i++)
  {
    gst_caps_append (filter,
                     gst_caps_new_simple (formats[i],
                                          "framerate", GST_TYPE_FRACTION_RANGE,
                                          0, 1, CHEESE_MAXIMUM_RATE, 1,
                                          NULL));
  }

  allowed = gst_caps_intersect (caps, filter);

  GST_DEBUG ("Supported caps %" GST_PTR_FORMAT, caps);
  GST_DEBUG ("Filter caps %" GST_PTR_FORMAT, filter);
  GST_DEBUG ("Filtered caps %" GST_PTR_FORMAT, allowed);

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

  GST_INFO ("%dx%d", format->width, format->height);

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

        format->width  = cur_width;
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

        format->width  = cur_width;
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
cheese_camera_device_get_caps (CheeseCameraDevice *device)
{
  CheeseCameraDevicePrivate *priv =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device);

  gchar               *pipeline_desc;
  GstElement          *pipeline;
  GstStateChangeReturn ret;
  GstMessage          *msg;
  GstBus              *bus;
  GError              *err = NULL;

  pipeline_desc = g_strdup_printf ("%s name=source device=%s ! fakesink",
                                   priv->src, priv->device);
  pipeline = gst_parse_launch (pipeline_desc, &err);
  if ((pipeline != NULL) && (err == NULL))
  {
    /* Start the pipeline and wait for max. 10 seconds for it to start up */
    gst_element_set_state (pipeline, GST_STATE_READY);
    ret = gst_element_get_state (pipeline, NULL, NULL, 10 * GST_SECOND);

    /* Check if any error messages were posted on the bus */
    bus = gst_element_get_bus (pipeline);
    msg = gst_bus_pop_filtered (bus, GST_MESSAGE_ERROR);
    gst_object_unref (bus);

    if ((msg == NULL) && (ret == GST_STATE_CHANGE_SUCCESS))
    {
      GstElement *src;
      GstPad     *pad;
      GstCaps    *caps;

      src = gst_bin_get_by_name (GST_BIN (pipeline), "source");

      GST_LOG ("Device: %s (%s)\n", priv->name, priv->device);
      pad        = gst_element_get_pad (src, "src");
      caps       = gst_pad_get_caps (pad);
      priv->caps = cheese_webcam_device_filter_caps (device, caps, supported_formats);
      if (!gst_caps_is_empty (priv->caps))
        cheese_webcam_device_update_format_table (device);
      else {
        g_set_error_literal (&priv->construct_error,
                             CHEESE_CAMERA_DEVICE_ERROR,
                             CHEESE_CAMERA_DEVICE_ERROR_UNSUPPORTED_CAPS,
                             _("Device capabilities not supported"));
      }

      gst_object_unref (pad);
      gst_caps_unref (caps);
    } else {
      if (msg) {
        gchar *dbg_info = NULL;
        gst_message_parse_error (msg, &err, &dbg_info);
        GST_WARNING ("Failed to start the capability probing pipeline");
        GST_WARNING ("Error from element %s: %s, %s",
                     GST_OBJECT_NAME (msg->src),
                     err->message,
                     (dbg_info) ? dbg_info : "no extra debug detail");
        g_error_free (err);
        err = NULL;
        /* construct_error is meant to be displayed in the UI
           (although it currently isn't displayed in cheese),
           err->message from gstreamer is too technical for this
           purpose, the idea is warn the user about an error and point
           him to the logs for more info */
        g_set_error (&priv->construct_error,
                     CHEESE_CAMERA_DEVICE_ERROR,
                     CHEESE_CAMERA_DEVICE_ERROR_FAILED_INITIALIZATION,
                     "Failed to initialize device %s for capability probing",
                     priv->device);
      }
    }
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);
  }

  if (err)
    g_error_free (err);

  g_free (pipeline_desc);
}

static void
cheese_camera_device_constructed (GObject *object)
{
  CheeseCameraDevice *device = CHEESE_CAMERA_DEVICE (object);
  CheeseCameraDevicePrivate *priv   =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device);

  priv->src = (priv->api == 2) ? "v4l2src" : "v4lsrc";

  cheese_camera_device_get_caps (device);

  if (G_OBJECT_CLASS (cheese_camera_device_parent_class)->constructed)
    G_OBJECT_CLASS (cheese_camera_device_parent_class)->constructed (object);
}

static void
cheese_camera_device_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  CheeseCameraDevice        *device = CHEESE_CAMERA_DEVICE (object);
  CheeseCameraDevicePrivate *priv   =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device);

  switch (prop_id)
  {
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;
    case PROP_FILE:
      g_value_set_string (value, priv->device);
      break;
    case PROP_ID:
      g_value_set_string (value, priv->id);
      break;
    case PROP_API:
      g_value_set_int (value, priv->api);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
cheese_camera_device_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  CheeseCameraDevice        *device = CHEESE_CAMERA_DEVICE (object);
  CheeseCameraDevicePrivate *priv   =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device);

  switch (prop_id)
  {
    case PROP_NAME:
      if (priv->name)
        g_free (priv->name);
      priv->name = g_value_dup_string (value);
      break;
    case PROP_ID:
      if (priv->id)
        g_free (priv->id);
      priv->id = g_value_dup_string (value);
      break;
    case PROP_FILE:
      if (priv->device)
        g_free (priv->device);
      priv->device = g_value_dup_string (value);
      break;
    case PROP_API:
      priv->api = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
cheese_camera_device_finalize (GObject *object)
{
  CheeseCameraDevice        *device = CHEESE_CAMERA_DEVICE (object);
  CheeseCameraDevicePrivate *priv   =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device);

  g_free (priv->device);
  g_free (priv->id);
  g_free (priv->name);

  gst_caps_unref (priv->caps);
  free_format_list (device);

  G_OBJECT_CLASS (cheese_camera_device_parent_class)->finalize (object);
}

static void
cheese_camera_device_class_init (CheeseCameraDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  if (cheese_camera_device_cat == NULL)
    GST_DEBUG_CATEGORY_INIT (cheese_camera_device_cat,
			     "cheese-camera-device",
			     0, "Cheese Camera Device");

  object_class->finalize     = cheese_camera_device_finalize;
  object_class->get_property = cheese_camera_device_get_property;
  object_class->set_property = cheese_camera_device_set_property;
  object_class->constructed  = cheese_camera_device_constructed;

  g_object_class_install_property (object_class, PROP_NAME,
                                   g_param_spec_string ("name",
                                                        NULL, NULL, NULL,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_FILE,
                                   g_param_spec_string ("device-file",
                                                        NULL, NULL, NULL,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_ID,
                                   g_param_spec_string ("device-id",
                                                        NULL, NULL, NULL,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_API,
                                   g_param_spec_int ("api", NULL, NULL,
                                                     1, 2, 2,
                                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_type_class_add_private (klass, sizeof (CheeseCameraDevicePrivate));
}

static void
cheese_camera_device_initable_iface_init (GInitableIface *iface)
{
  iface->init = cheese_camera_device_initable_init;
}

static void
cheese_camera_device_init (CheeseCameraDevice *device)
{
  CheeseCameraDevicePrivate *priv =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device);

  priv->device = NULL;
  priv->id     = NULL;
  priv->src    = NULL;
  priv->name   = g_strdup (_("Unknown device"));
  priv->caps   = gst_caps_new_empty ();

  priv->formats = NULL;

  priv->construct_error = NULL;
}

static gboolean
cheese_camera_device_initable_init (GInitable *initable,
                                    GCancellable *cancellable,
                                    GError **error)
{
  CheeseCameraDevice *device = CHEESE_CAMERA_DEVICE (initable);
  CheeseCameraDevicePrivate *priv =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device);

  g_return_val_if_fail (CHEESE_IS_CAMERA_DEVICE (initable), FALSE);

  if (cancellable != NULL)
  {
    g_set_error_literal (error,
                         CHEESE_CAMERA_DEVICE_ERROR,
                         CHEESE_CAMERA_DEVICE_ERROR_NOT_SUPPORTED,
                         _("Cancellable initialization not supported"));
    return FALSE;
  }

  if (priv->construct_error)
  {
    if (error)
      *error = g_error_copy (priv->construct_error);
    return FALSE;
  }

  return TRUE;
}

CheeseCameraDevice *
cheese_camera_device_new (const gchar *device_id,
                          const gchar *device_file,
                          const gchar *product_name,
                          gint         api_version,
                          GError **error)
{
  return CHEESE_CAMERA_DEVICE (g_initable_new (CHEESE_TYPE_CAMERA_DEVICE,
                                               NULL, error,
                                               "device-id", device_id,
                                               "device-file", device_file,
                                               "name", product_name,
                                               "api", api_version,
                                               NULL));
}

/* public methods */

GList *
cheese_camera_device_get_format_list (CheeseCameraDevice *device)
{
  CheeseCameraDevicePrivate *priv =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device);

  return g_list_sort (g_list_copy (priv->formats), compare_formats);
}

const gchar *
cheese_camera_device_get_name (CheeseCameraDevice *device)
{
  CheeseCameraDevicePrivate *priv =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device);

  return priv->name;
}

const gchar *
cheese_camera_device_get_id (CheeseCameraDevice *device)
{
  CheeseCameraDevicePrivate *priv =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device);

  return priv->id;
}

const gchar *
cheese_camera_device_get_src (CheeseCameraDevice *device)
{
  CheeseCameraDevicePrivate *priv =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device);

  return priv->src;
}

const gchar *
cheese_camera_device_get_device_file (CheeseCameraDevice *device)
{
  CheeseCameraDevicePrivate *priv =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device);

  return priv->device;
}

CheeseVideoFormat *
cheese_camera_device_get_best_format (CheeseCameraDevice *device)
{
  CheeseVideoFormat *format = g_boxed_copy (CHEESE_TYPE_VIDEO_FORMAT,
                                            cheese_camera_device_get_format_list (device)->data);

  GST_INFO ("%dx%d", format->width, format->height);
  return format;
}

GstCaps *
cheese_camera_device_get_caps_for_format (CheeseCameraDevice *device,
                                          CheeseVideoFormat  *format)
{
  CheeseCameraDevicePrivate *priv =
    CHEESE_CAMERA_DEVICE_GET_PRIVATE (device);
  GstCaps *desired_caps;
  GstCaps *subset_caps;
  gint     i;

  GST_INFO ("Getting caps for %dx%d", format->width, format->height);

  desired_caps = gst_caps_new_simple (supported_formats[0],
                                      "width", G_TYPE_INT,
                                      format->width,
                                      "height", G_TYPE_INT,
                                      format->height,
                                      NULL);

  for (i = 1; i < g_strv_length (supported_formats); i++)
  {
    gst_caps_append (desired_caps,
                     gst_caps_new_simple (supported_formats[i],
                                          "width", G_TYPE_INT,
                                          format->width,
                                          "height", G_TYPE_INT,
                                          format->height,
                                          NULL));
  }

  subset_caps = gst_caps_intersect (desired_caps, priv->caps);
  gst_caps_unref (desired_caps);

  GST_INFO ("Got %" GST_PTR_FORMAT, subset_caps);

  return subset_caps;
}
