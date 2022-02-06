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
  #include <config.h>
#endif

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>

#include "cheese-camera-device.h"

/**
 * SECTION:cheese-camera-device
 * @short_description: Object to represent a video capture device
 * @stability: Unstable
 * @include: cheese/cheese-camera-device.h
 *
 * #CheeseCameraDevice provides an abstraction of a video capture device.
 */

static void     cheese_camera_device_initable_iface_init (GInitableIface *iface);
static gboolean cheese_camera_device_initable_init (GInitable    *initable,
                                                    GCancellable *cancellable,
                                                    GError      **error);

#define CHEESE_CAMERA_DEVICE_ERROR cheese_camera_device_error_quark ()

/*
 * CheeseCameraDeviceError:
 * @CHEESE_CAMERA_DEVICE_ERROR_UNKNOWN: unknown error
 * @CHEESE_CAMERA_DEVICE_ERROR_NOT_SUPPORTED: cancellation of device
 * initialisation was requested, but is not supported
 * @CHEESE_CAMERA_DEVICE_ERROR_UNSUPPORTED_CAPS: unsupported GStreamer
 * capabilities
 * @CHEESE_CAMERA_DEVICE_ERROR_FAILED_INITIALIZATION: the device failed to
 * initialize for capability probing
 *
 * Errors that can occur during device initialization.
 */
enum CheeseCameraDeviceError
{
  CHEESE_CAMERA_DEVICE_ERROR_UNKNOWN,
  CHEESE_CAMERA_DEVICE_ERROR_NOT_SUPPORTED,
  CHEESE_CAMERA_DEVICE_ERROR_UNSUPPORTED_CAPS,
  CHEESE_CAMERA_DEVICE_ERROR_FAILED_INITIALIZATION
};

GST_DEBUG_CATEGORY (cheese_camera_device_cat);
#define GST_CAT_DEFAULT cheese_camera_device_cat

static const gchar * const supported_formats[] = {
    "video/x-raw",
    "image/jpeg",
    NULL
};

/* FIXME: make this configurable */
/*
 * CHEESE_MAXIMUM_RATE:
 *
 * The maximum framerate, in frames per second.
 */
static const guint CHEESE_MAXIMUM_RATE = 30;

enum
{
  PROP_0,
  PROP_NAME,
  PROP_PATH,
  PROP_DEVICE,
  PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

typedef struct
{
  GstDevice *device;
  gchar     *name;
  GstCaps   *caps;
  GList     *formats; /* list members are CheeseVideoFormatFull structs. */
  gchar     *path;

  GError    *construct_error;
} CheeseCameraDevicePrivate;

G_DEFINE_TYPE_WITH_CODE (CheeseCameraDevice, cheese_camera_device,
                         G_TYPE_OBJECT,
                         G_ADD_PRIVATE (CheeseCameraDevice)
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
                                                cheese_camera_device_initable_iface_init))

/*
 * This is our private version of CheeseVideoFormat, with extra fields added
 * at the end. IMPORTANT the first fields *must* be kept in sync with the
 * public CheeseVideoFormat, since in various places we cast pointers to
 * CheeseVideoFormatFull to CheeseVideoFormat.
 */
typedef struct
{
  /* CheeseVideoFormat members keep synced with cheese-camera-device.h! */
  gint width;
  gint height;
  /*< private >*/
  gint fr_numerator;
  gint fr_denominator;
} CheeseVideoFormatFull;

GQuark cheese_camera_device_error_quark (void);

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

G_DEFINE_BOXED_TYPE (CheeseVideoFormat, cheese_video_format,
    cheese_video_format_copy, cheese_video_format_free)

/* the rest */

static gint
compare_formats (gconstpointer a, gconstpointer b)
{
  const CheeseVideoFormatFull *c = a;
  const CheeseVideoFormatFull *d = b;

  /* descending sort for rectangle area */
  return (d->width * d->height - c->width * c->height);
}

static GstCaps *
format_caps (const gchar * const formats[])
{
    GstCaps *filter;
    gsize i;

    filter = gst_caps_new_empty ();

    for (i = 0; formats[i] != NULL; i++)
    {
        gst_caps_append (filter,
                         gst_caps_new_simple (formats[i],
                                              "framerate",
                                              GST_TYPE_FRACTION_RANGE,
                                              0, 1, CHEESE_MAXIMUM_RATE, 1,
                                              NULL));
    }

    return filter;
}

/*
 * cheese_camera_device_filter_caps:
 * @device: the #CheeseCameraDevice
 * @caps: the #GstCaps that the device supports
 * @formats: an array of strings of video formats, in the form axb, where a and
 * b are in units of pixels
 *
 * Filter the supplied @caps with %CHEESE_MAXIMUM_RATE to only allow @formats
 * which can reach the desired framerate.
 *
 * Returns: the filtered #GstCaps
 */
static GstCaps *
cheese_camera_device_filter_caps (CheeseCameraDevice *device,
                                  GstCaps *caps,
                                  const gchar * const formats[])
{
  GstCaps *filter;
  GstCaps *allowed;

    filter = format_caps (formats);

  allowed = gst_caps_intersect (caps, filter);

  GST_DEBUG ("Supported caps %" GST_PTR_FORMAT, caps);
  GST_DEBUG ("Filter caps %" GST_PTR_FORMAT, filter);
  GST_DEBUG ("Filtered caps %" GST_PTR_FORMAT, allowed);

  gst_caps_unref (filter);

  return allowed;
}

/*
 * cheese_camera_device_get_highest_framerate:
 * @framerate: a #GValue holding a framerate cap
 * @numerator: destination to store the numerator of the highest rate
 * @denominator: destination to store the denominator of the highest rate
 *
 * Get the numerator and denominator for the highest framerate stored in
 * a framerate cap.
 *
 * Note this function does not handle framerate ranges, if @framerate
 * contains a range it will return 0/0 as framerate
 */
static void
cheese_camera_device_get_highest_framerate (const GValue *framerate,
                                            gint *numerator, gint *denominator)
{
  *numerator = 0;
  *denominator = 0;

  if (GST_VALUE_HOLDS_FRACTION (framerate))
  {
    *numerator = gst_value_get_fraction_numerator (framerate);
    *denominator = gst_value_get_fraction_denominator (framerate);
  }
  else if (GST_VALUE_HOLDS_ARRAY (framerate))
  {
    float curr, highest = 0;
    guint i, size = gst_value_array_get_size (framerate);

    for (i = 0; i < size; i++)
    {
      const GValue *val = gst_value_array_get_value (framerate, i);

      if (!GST_VALUE_HOLDS_FRACTION (val) ||
          gst_value_get_fraction_denominator (val) == 0) {
        continue;
      }

      curr = (float)gst_value_get_fraction_numerator (val) /
             (float)gst_value_get_fraction_denominator (val);

      if (curr > highest && curr <= CHEESE_MAXIMUM_RATE)
      {
        highest = curr;
        *numerator = gst_value_get_fraction_numerator (val);
        *denominator = gst_value_get_fraction_denominator (val);
      }
    }
  }
  else if (GST_VALUE_HOLDS_LIST (framerate))
  {
    float curr, highest = 0;
    guint i, size = gst_value_list_get_size (framerate);

    for (i = 0; i < size; i++)
    {
      const GValue *val = gst_value_list_get_value(framerate, i);

      if (!GST_VALUE_HOLDS_FRACTION (val) ||
          gst_value_get_fraction_denominator (val) == 0)
      {
        continue;
      }

      curr = (float)gst_value_get_fraction_numerator (val) /
             (float)gst_value_get_fraction_denominator (val);

      if (curr > highest && curr <= CHEESE_MAXIMUM_RATE)
      {
        highest = curr;
        *numerator = gst_value_get_fraction_numerator (val);
        *denominator = gst_value_get_fraction_denominator (val);
      }
    }
  }
  else if (GST_VALUE_HOLDS_FRACTION_RANGE (framerate))
  {
    const GValue *val = gst_value_get_fraction_range_max (framerate);

    if (GST_VALUE_HOLDS_FRACTION (val))
    {
      *numerator = gst_value_get_fraction_numerator (val);
      *denominator = gst_value_get_fraction_denominator (val);
    }
  }
}

/*
 * cheese_camera_device_format_update_framerate:
 * @format: the #CheeseVideoFormatFull to update the framerate of
 * @framerate: a #GValue holding a framerate cap
 *
 * This function updates the framerate in @format with the highest framerate
 * from @framerate, if @framerate contains a framerate higher then the
 * framerate currently stored in @format.
 */
static void
cheese_camera_device_format_update_framerate (CheeseVideoFormatFull *format,
                                              const GValue *framerate)
{
  float high, curr = (float)format->fr_numerator / format->fr_denominator;
  gint high_numerator, high_denominator;

  cheese_camera_device_get_highest_framerate (framerate, &high_numerator,
                                              &high_denominator);
  if (high_denominator == 0)
    return;

  high = (float)high_numerator / (float)high_denominator;

  if (high > curr) {
    format->fr_numerator = high_numerator;
    format->fr_denominator = high_denominator;
    GST_INFO ("%dx%d new framerate %d/%d", format->width, format->height,
              format->fr_numerator, format->fr_denominator);
  }
}

/*
 * cheese_camera_device_find_full_format:
 * @device: a #CheeseCameraDevice
 * @format: #CheeseVideoFormat to find the matching #CheeseVideoFormatFull for
 *
 * Find a #CheeseVideoFormatFull matching the passed in #CheeseVideoFormat.
 */
static CheeseVideoFormatFull *
cheese_camera_device_find_full_format (CheeseCameraDevice *device,
                                       CheeseVideoFormat* format)
{
    CheeseCameraDevicePrivate *priv;
  GList *l;

    priv = cheese_camera_device_get_instance_private (device);

    for (l = priv->formats; l != NULL; l = g_list_next (l))
    {
        CheeseVideoFormatFull *item = l->data;

        if ((item != NULL) &&
            (item->width == format->width) && (item->height == format->height))
        {
            return item;
        }
    }

  return NULL;
}

/*
 * cheese_camera_device_add_format:
 * @device: a #CheeseCameraDevice
 * @format: the #CheeseVideoFormatFull to add
 *
 * Add the supplied @format to the list of formats supported by the @device.
 */
static void
cheese_camera_device_add_format (CheeseCameraDevice *device,
                                 CheeseVideoFormatFull *format,
                                 const GValue *framerate)
{
    CheeseCameraDevicePrivate *priv;
  CheeseVideoFormatFull *existing;

    priv = cheese_camera_device_get_instance_private (device);
  existing = cheese_camera_device_find_full_format (device,
                                                    (CheeseVideoFormat *)format);

  if (existing)
  {
    g_slice_free (CheeseVideoFormatFull, format);
    cheese_camera_device_format_update_framerate (existing, framerate);
    return;
  }

  cheese_camera_device_get_highest_framerate (framerate, &format->fr_numerator,
                                              &format->fr_denominator);
  GST_INFO ("%dx%d framerate %d/%d", format->width, format->height,
            format->fr_numerator, format->fr_denominator);

    priv->formats = g_list_insert_sorted (priv->formats, format,
                                          compare_formats);
}

/*
 * free_format_list_foreach:
 * @data: the #CheeseVideoFormatFull to free
 *
 * Free the individual #CheeseVideoFormatFull.
 */
static void
free_format_list_foreach (gpointer data)
{
  g_slice_free (CheeseVideoFormatFull, data);
}

/*
 * free_format_list:
 * @device: a #CheeseCameraDevice
 *
 * Free the list of video formats for the @device.
 */
static void
free_format_list (CheeseCameraDevice *device)
{
    CheeseCameraDevicePrivate *priv;

    priv = cheese_camera_device_get_instance_private (device);

  g_list_free_full (priv->formats, free_format_list_foreach);
  priv->formats = NULL;
}

/*
 * cheese_camera_device_update_format_table:
 * @device: a #CheeseCameraDevice
 *
 * Clear the current list of video formats supported by the @device and create
 * it anew.
 */
static void
cheese_camera_device_update_format_table (CheeseCameraDevice *device)
{
    CheeseCameraDevicePrivate *priv;

  guint i;
  guint num_structures;

  free_format_list (device);

    priv = cheese_camera_device_get_instance_private (device);
  num_structures = gst_caps_get_size (priv->caps);
  for (i = 0; i < num_structures; i++)
  {
    GstStructure *structure;
    const GValue *width, *height, *framerate;
    structure = gst_caps_get_structure (priv->caps, i);

    width  = gst_structure_get_value (structure, "width");
    height = gst_structure_get_value (structure, "height");
    framerate = gst_structure_get_value (structure, "framerate");

    if (G_VALUE_HOLDS_INT (width) && G_VALUE_HOLDS_INT (height))
    {
      CheeseVideoFormatFull *format;
      gint width = 0, height = 0;

      gst_structure_get_int (structure, "width", &width);
      gst_structure_get_int (structure, "height", &height);

      if (width > 0 && height > 0) {
        format = g_slice_new0 (CheeseVideoFormatFull);

	format->width = width;
	format->height = height;

        cheese_camera_device_add_format (device, format, framerate);
      }
    }
    else if (GST_VALUE_HOLDS_INT_RANGE (width) &&
             GST_VALUE_HOLDS_INT_RANGE (height))
    {
      gint min_width, max_width, min_height, max_height;
      gint cur_width, cur_height;

      min_width  = gst_value_get_int_range_min (width);
      max_width  = gst_value_get_int_range_max (width);
      min_height = gst_value_get_int_range_min (height);
      max_height = gst_value_get_int_range_max (height);

      /* Some devices report a very small min_width / height down to reporting
       * 0x0 as minimum resolution, which causes an infinte loop below, limit
       * these to something reasonable. */
      if (min_width < 160)
        min_width = 160;
      if (min_height < 120)
        min_height = 120;

      if (max_width > 5120)
        max_width = 5120;
      if (max_height > 3840)
        max_height = 3840;

      cur_width  = min_width;
      cur_height = min_height;

      /* Gstreamer will sometimes give us a range with min_xxx == max_xxx,
       * we use <= here (and not below) to make this work */
      while (cur_width <= max_width && cur_height <= max_height)
      {
        CheeseVideoFormatFull *format = g_slice_new0 (CheeseVideoFormatFull);

        /* Gstreamer wants resolutions for YUV formats where the width is
         * a multiple of 8, and the height is a multiple of 2 */
        format->width  = cur_width & ~7;
        format->height = cur_height & ~1;

        cheese_camera_device_add_format (device, format, framerate);

        cur_width  *= 2;
        cur_height *= 2;
      }

      cur_width  = max_width;
      cur_height = max_height;
      while (cur_width > min_width && cur_height > min_height)
      {
        CheeseVideoFormatFull *format = g_slice_new0 (CheeseVideoFormatFull);

        /* Gstreamer wants resolutions for YUV formats where the width is
         * a multiple of 8, and the height is a multiple of 2 */
        format->width  = cur_width & ~7;
        format->height = cur_height & ~1;

        cheese_camera_device_add_format (device, format, framerate);

        cur_width  /= 2;
        cur_height /= 2;
      }
    }
    else
    {
      g_critical ("GValue type %s x %s, cannot be handled for resolution",
		      G_VALUE_TYPE_NAME (width), G_VALUE_TYPE_NAME (height));
    }
  }
}

/*
 * cheese_camera_device_get_caps:
 * @device: a #CheeseCameraDevice
 *
 * Probe the #GstCaps that the @device supports.
 */
static void
cheese_camera_device_get_caps (CheeseCameraDevice *device)
{
  CheeseCameraDevicePrivate *priv;
  GstCaps *caps;

  priv = cheese_camera_device_get_instance_private (device);

  caps = gst_device_get_caps (priv->device);
  if (caps == NULL)
    caps = gst_caps_new_empty_simple ("video/x-raw");

  gst_caps_unref (priv->caps);
  priv->caps = cheese_camera_device_filter_caps (device, caps, supported_formats);

  if (!gst_caps_is_empty (priv->caps))
    cheese_camera_device_update_format_table (device);
  else
  {
    g_set_error_literal (&priv->construct_error,
                         CHEESE_CAMERA_DEVICE_ERROR,
                         CHEESE_CAMERA_DEVICE_ERROR_UNSUPPORTED_CAPS,
                         _("Device capabilities not supported"));
  }
  gst_caps_unref (caps);
}

static void
cheese_camera_device_constructed (GObject *object)
{
  CheeseCameraDevice        *device = CHEESE_CAMERA_DEVICE (object);

  cheese_camera_device_get_caps (device);

  if (G_OBJECT_CLASS (cheese_camera_device_parent_class)->constructed)
    G_OBJECT_CLASS (cheese_camera_device_parent_class)->constructed (object);
}

static void
cheese_camera_device_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  CheeseCameraDevice        *device = CHEESE_CAMERA_DEVICE (object);
    CheeseCameraDevicePrivate *priv = cheese_camera_device_get_instance_private (device);

  switch (prop_id)
  {
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;
    case PROP_DEVICE:
      g_value_set_object (value, priv->device);
      break;
    case PROP_PATH:
      g_value_set_string (value, priv->path);
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
  CheeseCameraDevicePrivate *priv = cheese_camera_device_get_instance_private (device);
  const GValue *tmp;

  switch (prop_id)
  {
    case PROP_NAME:
      g_free (priv->name);
      priv->name = g_value_dup_string (value);
      break;
    case PROP_DEVICE:
      g_clear_object (&priv->device);
      priv->device = g_value_dup_object (value);
      g_free (priv->name);
      priv->name = gst_device_get_display_name (priv->device);
      tmp = gst_structure_get_value (gst_device_get_properties (priv->device), "api.v4l2.path");
      if (tmp) {
        g_clear_pointer (&priv->path, g_free);
        priv->path = g_value_dup_string (tmp);
      }
      break;
    case PROP_PATH:
      g_free (priv->path);
      priv->path = g_value_dup_string (value);
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
    CheeseCameraDevicePrivate *priv = cheese_camera_device_get_instance_private (device);

  g_object_unref (priv->device);
  g_free (priv->name);
  g_free (priv->path);

  gst_caps_unref (priv->caps);
  free_format_list (device);

  G_OBJECT_CLASS (cheese_camera_device_parent_class)->finalize (object);
}

static void
cheese_camera_device_class_init (CheeseCameraDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

#ifndef GST_DISABLE_GST_DEBUG
  if (cheese_camera_device_cat == NULL)
    GST_DEBUG_CATEGORY_INIT (cheese_camera_device_cat,
                             "cheese-camera-device",
                             0, "Cheese Camera Device");
#endif

  object_class->finalize     = cheese_camera_device_finalize;
  object_class->get_property = cheese_camera_device_get_property;
  object_class->set_property = cheese_camera_device_set_property;
  object_class->constructed  = cheese_camera_device_constructed;

  /**
   * CheeseCameraDevice:name:
   *
   * Human-readable name of the video capture device, for display to the user.
   */
  properties[PROP_NAME] = g_param_spec_string ("name",
                                               "Name of the device",
                                               "Human-readable name of the video capture device",
                                               NULL,
                                               G_PARAM_READWRITE |
                                               G_PARAM_CONSTRUCT_ONLY |
                                               G_PARAM_STATIC_STRINGS);

  /**
   * CheeseCameraDevice:path:
   *
   * Path of the video capture device.
   */
  properties[PROP_PATH] = g_param_spec_string ("path",
                                               "Device path",
                                               "Path of the video capture device",
                                               NULL,
                                               G_PARAM_READWRITE |
                                               G_PARAM_CONSTRUCT_ONLY |
                                               G_PARAM_STATIC_STRINGS);
  /**
   * CheeseCameraDevice:device:
   *
   * GStreamer device object of the video capture device.
   */
  properties[PROP_DEVICE] = g_param_spec_object ("device",
                                                 "Device",
                                                 "The GStreamer device object of the video capture device",
                                                 GST_TYPE_DEVICE,
                                                 G_PARAM_READWRITE |
                                                 G_PARAM_CONSTRUCT_ONLY |
                                                 G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST, properties);
}

static void
cheese_camera_device_initable_iface_init (GInitableIface *iface)
{
  iface->init = cheese_camera_device_initable_init;
}

static void
cheese_camera_device_init (CheeseCameraDevice *device)
{
    CheeseCameraDevicePrivate *priv = cheese_camera_device_get_instance_private (device);

  priv->name = g_strdup (_("Unknown device"));
  priv->caps = gst_caps_new_empty ();
}

static gboolean
cheese_camera_device_initable_init (GInitable    *initable,
                                    GCancellable *cancellable,
                                    GError      **error)
{
  CheeseCameraDevice        *device = CHEESE_CAMERA_DEVICE (initable);
    CheeseCameraDevicePrivate *priv = cheese_camera_device_get_instance_private (device);

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

/* public methods */

/**
 * cheese_camera_device_new:
 * @device: The GStreamer the device, as supplied by GstDeviceMonitor
 * @error: a location to store errors
 *
 * Tries to create a new #CheeseCameraDevice with the supplied device. If
 * construction fails, %NULL is returned, and @error is set.
 *
 * Returns: a new #CheeseCameraDevice, or %NULL
 */
CheeseCameraDevice *
cheese_camera_device_new (GstDevice *device,
                          GError    **error)
{
  return CHEESE_CAMERA_DEVICE (g_initable_new (CHEESE_TYPE_CAMERA_DEVICE,
                                               NULL, error,
                                               "device", device,
                                               NULL));
}

/**
 * cheese_camera_device_get_format_list:
 * @device: a #CheeseCameraDevice
 *
 * Get the sorted list of #CheeseVideoFormat that the @device supports.
 *
 * Returns: (element-type Cheese.VideoFormat) (transfer container): list of
 * #CheeseVideoFormat
 */
GList *
cheese_camera_device_get_format_list (CheeseCameraDevice *device)
{
    CheeseCameraDevicePrivate *priv;

  g_return_val_if_fail (CHEESE_IS_CAMERA_DEVICE (device), NULL);

    priv = cheese_camera_device_get_instance_private (device);

    return g_list_copy (priv->formats);
}

/**
 * cheese_camera_device_get_path:
 * @device: a #CheeseCameraDevice
 *
 * Get path for the device, as reported by udev.
 *
 * Returns: (transfer none): the path of the video capture device
 */
const gchar *
cheese_camera_device_get_path (CheeseCameraDevice *device)
{
  CheeseCameraDevicePrivate *priv;

  g_return_val_if_fail (CHEESE_IS_CAMERA_DEVICE (device), NULL);

  priv = cheese_camera_device_get_instance_private (device);

  return priv->path;
}
/**
 * cheese_camera_device_get_name:
 * @device: a #CheeseCameraDevice
 *
 * Get a human-readable name for the device, as reported by udev, which is
 * suitable for display to a user.
 *
 * Returns: (transfer none): the human-readable name of the video capture device
 */
const gchar *
cheese_camera_device_get_name (CheeseCameraDevice *device)
{
    CheeseCameraDevicePrivate *priv;

  g_return_val_if_fail (CHEESE_IS_CAMERA_DEVICE (device), NULL);

    priv = cheese_camera_device_get_instance_private (device);

    return priv->name;
}

/**
 * cheese_camera_device_get_src:
 * @device: a #CheeseCameraDevice
 *
 * Get the source GStreamer element for the @device.
 *
 * Returns: (transfer full): the source GStreamer element
 */
GstElement *
cheese_camera_device_get_src (CheeseCameraDevice *device)
{
    CheeseCameraDevicePrivate *priv;

  g_return_val_if_fail (CHEESE_IS_CAMERA_DEVICE (device), NULL);

    priv = cheese_camera_device_get_instance_private (device);

    return gst_device_create_element (priv->device, NULL);
}

/**
 * cheese_camera_device_get_best_format:
 * @device: a #CheeseCameraDevice
 *
 * Get the #CheeseVideoFormat with the highest resolution with a width greater
 * than 640 pixels and a framerate of greater than 15 FPS for this @device. If
 * no such format is found, get the highest available resolution instead.
 *
 * Returns: (transfer full): the highest-resolution supported
 * #CheeseVideoFormat
 */
CheeseVideoFormat *
cheese_camera_device_get_best_format (CheeseCameraDevice *device)
{
    CheeseCameraDevicePrivate *priv;
  CheeseVideoFormatFull *format = NULL;
  GList *l;

  g_return_val_if_fail (CHEESE_IS_CAMERA_DEVICE (device), NULL);

    priv = cheese_camera_device_get_instance_private (device);

    /* Check for the highest resolution with width >= 640 and FPS >= 15. */
    for (l = priv->formats; l != NULL; l = g_list_next (l))
    {
        CheeseVideoFormatFull *item = l->data;
        float frame_rate = (float)item->fr_numerator
                           / (float)item->fr_denominator;

        if (item->width >= 640 && frame_rate >= 15)
        {
            format = item;
            break;
        }
    }

    /* Else simply return the highest resolution. */
    if (!format)
    {
        format = priv->formats->data;
    }

  GST_INFO ("%dx%d@%d/%d", format->width, format->height,
            format->fr_numerator, format->fr_denominator);

  return g_boxed_copy (CHEESE_TYPE_VIDEO_FORMAT, format);
}

static GstCaps *
cheese_camera_device_format_to_caps (const char *media_type,
                                     CheeseVideoFormatFull *format)
{
  if (format->fr_numerator != 0 && format->fr_denominator != 0)
  {
    return gst_caps_new_simple (media_type,
                                "framerate", GST_TYPE_FRACTION,
                                format->fr_numerator, format->fr_denominator,
                                "width", G_TYPE_INT, format->width,
                                "height", G_TYPE_INT, format->height, NULL);
  }
  else
  {
    return gst_caps_new_simple (media_type,
                                "width", G_TYPE_INT, format->width,
                                "height", G_TYPE_INT, format->height, NULL);
  }
}

/**
 * cheese_camera_device_get_caps_for_format:
 * @device: a #CheeseCameraDevice
 * @format: a #CheeseVideoFormat
 *
 * Get the #GstCaps for the given @format on the @device.
 *
 * Returns: (transfer full): the #GstCaps for the given @format
 */
GstCaps *
cheese_camera_device_get_caps_for_format (CheeseCameraDevice *device,
                                          CheeseVideoFormat  *format)
{
    CheeseCameraDevicePrivate *priv;
    CheeseVideoFormatFull *full_format;
    GstCaps *result_caps;
    gsize i;

  g_return_val_if_fail (CHEESE_IS_CAMERA_DEVICE (device), NULL);

  full_format = cheese_camera_device_find_full_format (device, format);

  if (!full_format)
  {
    GST_INFO ("Getting caps for %dx%d: no such format!",
              format->width, format->height);
    return gst_caps_new_empty ();
  }

  GST_INFO ("Getting caps for %dx%d @ %d/%d fps",
            full_format->width, full_format->height,
            full_format->fr_numerator, full_format->fr_denominator);

    priv = cheese_camera_device_get_instance_private (device);

    result_caps = gst_caps_new_empty ();

    for (i = 0; supported_formats[i] != NULL; i++)
    {
        GstCaps *desired_caps;
        GstCaps *subset_caps;

        desired_caps = cheese_camera_device_format_to_caps (supported_formats[i],
                                                            full_format);
        subset_caps = gst_caps_intersect (desired_caps, priv->caps);
        subset_caps = gst_caps_simplify (subset_caps);

        gst_caps_append (result_caps, subset_caps);

        gst_caps_unref (desired_caps);
    }

    GST_INFO ("Got %" GST_PTR_FORMAT, result_caps);

    return result_caps;
}

/**
 * cheese_camera_device_supported_format_caps:
 *
 * Get the #GstCaps that are supported for all #CheeseCameraDevice
 *
 * Returns: (transfer full): the #GstCaps
 */
GstCaps *
cheese_camera_device_supported_format_caps (void)
{
    return format_caps (supported_formats);
}
