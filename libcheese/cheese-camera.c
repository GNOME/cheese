/*
 * Copyright © 2007,2008 Jaap Haitsma <jaap@haitsma.org>
 * Copyright © 2007-2009 daniel g. siegel <dgsiegel@gnome.org>
 * Copyright © 2008 Ryan Zeigler <zeiglerr@gmail.com>
 * Copyright © 2010 Yuvaraj Pandian T <yuvipanda@yuvi.in>
 * Copyright © 2011 Luciana Fujii Pontello <luciana@fujii.eti.br>
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

#include <string.h>
#include <glib-object.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <clutter/clutter.h>
#include <clutter-gst/clutter-gst.h>
#include <gst/gst.h>
#include <X11/Xlib.h>

#include "cheese-camera.h"
#include "cheese-camera-device.h"
#include "cheese-camera-device-monitor.h"

G_DEFINE_TYPE (CheeseCamera, cheese_camera, G_TYPE_OBJECT)

#define CHEESE_CAMERA_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHEESE_TYPE_CAMERA, CheeseCameraPrivate))

#define CHEESE_CAMERA_ERROR cheese_camera_error_quark ()

typedef enum
{
  MODE_IMAGE = 0,
  MODE_VIDEO
} GstCameraBinMode;

typedef enum {
  GST_CAMERABIN_FLAG_SOURCE_RESIZE               = (1 << 0),
  GST_CAMERABIN_FLAG_SOURCE_COLOR_CONVERSION     = (1 << 1),
  GST_CAMERABIN_FLAG_VIEWFINDER_COLOR_CONVERSION = (1 << 2),
  GST_CAMERABIN_FLAG_VIEWFINDER_SCALE            = (1 << 3),
  GST_CAMERABIN_FLAG_AUDIO_CONVERSION            = (1 << 4),
  GST_CAMERABIN_FLAG_DISABLE_AUDIO               = (1 << 5),
  GST_CAMERABIN_FLAG_IMAGE_COLOR_CONVERSION      = (1 << 6),
  GST_CAMERABIN_FLAG_VIDEO_COLOR_CONVERSION      = (1 << 7)
} GstCameraBinFlags;


typedef struct
{
  GstBus *bus;

  GstElement *camerabin;
  GstElement *video_filter_bin;
  GstElement *effects_preview_bin;

  GstElement *video_source;
  GstElement *video_file_sink;
  GstElement *audio_source;
  GstElement *audio_enc;
  GstElement *video_enc;

  ClutterTexture *video_texture;

  GstElement *effect_filter;
  GstElement *video_balance, *csp_post_balance;
  GstElement *camera_tee, *effects_tee;
  GstElement *effects_downscaler;
  GstElement *main_valve, *effects_valve;

  GstCaps *preview_caps;

  gulong photo_handler_signal_id;

  gboolean is_recording;
  gboolean pipeline_is_playing;
  char *photo_filename;

  int num_camera_devices;
  char *device_name;

  /* an array of CheeseCameraDevices */
  GPtrArray *camera_devices;
  int x_resolution;
  int y_resolution;
  int selected_device;
  CheeseVideoFormat *current_format;

  guint eos_timeout_id;
} CheeseCameraPrivate;

enum
{
  PROP_0,
  PROP_VIDEO_TEXTURE,
  PROP_DEVICE_NAME,
  PROP_FORMAT,
};

enum
{
  PHOTO_SAVED,
  PHOTO_TAKEN,
  VIDEO_SAVED,
  STATE_FLAGS_CHANGED,
  LAST_SIGNAL
};

static guint camera_signals[LAST_SIGNAL];

GST_DEBUG_CATEGORY (cheese_camera_cat);
#define GST_CAT_DEFAULT cheese_camera_cat

GQuark
cheese_camera_error_quark (void)
{
  return g_quark_from_static_string ("cheese-camera-error-quark");
}

static void
cheese_camera_photo_data (CheeseCamera *camera, GstBuffer *buffer)
{
  GstCaps            *caps;
  const GstStructure *structure;
  int                 width, height, stride;
  GdkPixbuf          *pixbuf;
  const int           bits_per_pixel = 8;
  guchar             *data = NULL;
  CheeseCameraPrivate *priv  = CHEESE_CAMERA_GET_PRIVATE (camera);

  caps = gst_buffer_get_caps (buffer);
  structure = gst_caps_get_structure (caps, 0);
  gst_structure_get_int (structure, "width", &width);
  gst_structure_get_int (structure, "height", &height);

  stride = buffer->size / height;

  data = g_memdup (GST_BUFFER_DATA (buffer), buffer->size);
  pixbuf = gdk_pixbuf_new_from_data (data ? data : GST_BUFFER_DATA (buffer),
                                     GDK_COLORSPACE_RGB,
                                     FALSE, bits_per_pixel, width, height, stride,
                                     data ? (GdkPixbufDestroyNotify) g_free : NULL, NULL);

  g_object_set (G_OBJECT (priv->camerabin), "preview-caps", NULL, NULL);
  g_signal_emit (camera, camera_signals[PHOTO_TAKEN], 0, pixbuf);
  g_object_unref (pixbuf);
}

static void
cheese_camera_bus_message_cb (GstBus *bus, GstMessage *message, CheeseCamera *camera)
{
  switch (GST_MESSAGE_TYPE (message))
  {
    case GST_MESSAGE_WARNING:
    {
      GError *err = NULL;
      gchar *debug = NULL;
      gst_message_parse_warning (message, &err, &debug);

      if (err && err->message) {
        g_warning ("%s\n", err->message);
        g_error_free (err);
      } else {
        g_warning ("Unparsable GST_MESSAGE_WARNING message.\n");
      }

      g_free (debug);
      break;
    }
    case GST_MESSAGE_ERROR:
    {
      GError *err = NULL;
      gchar *debug = NULL;
      gst_message_parse_error (message, &err, &debug);

      if (err && err->message) {
        g_warning ("%s\n", err->message);
        g_error_free (err);
      } else {
        g_warning ("Unparsable GST_MESSAGE_ERROR message.\n");
      }

      g_free (debug);
      break;
    }
    case GST_MESSAGE_STATE_CHANGED:
    {
      if (strcmp (GST_MESSAGE_SRC_NAME (message), "camerabin") == 0)
      {
        GstState old, new;
        gst_message_parse_state_changed (message, &old, &new, NULL);
        if (new == GST_STATE_PLAYING)
          g_signal_emit (camera, camera_signals[STATE_FLAGS_CHANGED], 0, new);
      }
      break;
    }
    case GST_MESSAGE_ELEMENT:
    {
      const GstStructure *structure;
      GstBuffer *buffer;
      const GValue *image;
      if (strcmp (GST_MESSAGE_SRC_NAME (message), "camerabin") == 0)
      {
        structure = gst_message_get_structure (message);
        if (strcmp (gst_structure_get_name (structure), "preview-image") == 0)
        {
          if (gst_structure_has_field_typed (structure, "buffer", GST_TYPE_BUFFER))
          {
            image = gst_structure_get_value (structure, "buffer");
            if (image)
            {
              buffer = gst_value_get_buffer (image);
              cheese_camera_photo_data (camera, buffer);
            }
            else
            {
              g_warning ("Could not get buffer from bus message");
            }
          }
        }
      }
      break;
    }
    default:
    {
      break;
    }
  }
}

static void
cheese_camera_add_device (CheeseCameraDeviceMonitor *monitor,
                          const gchar               *id,
                          const gchar               *device_file,
                          const gchar               *product_name,
                          gint                       api_version,
                          CheeseCamera              *camera)
{
  CheeseCameraPrivate *priv  = CHEESE_CAMERA_GET_PRIVATE (camera);
  GError              *error = NULL;

  CheeseCameraDevice *device = cheese_camera_device_new (id,
                                                         device_file,
                                                         product_name,
                                                         api_version,
                                                         &error);

  if (device == NULL)
    GST_WARNING ("Device initialization for %s failed: %s ",
                 device_file,
                 (error != NULL) ? error->message : "Unknown reason");
  else
  {
    g_ptr_array_add (priv->camera_devices, device);
    priv->num_camera_devices++;
  }
}

static void
cheese_camera_detect_camera_devices (CheeseCamera *camera)
{
  CheeseCameraPrivate       *priv = CHEESE_CAMERA_GET_PRIVATE (camera);
  CheeseCameraDeviceMonitor *monitor;

  priv->num_camera_devices = 0;
  priv->camera_devices     = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);

  monitor = cheese_camera_device_monitor_new ();
  g_signal_connect (G_OBJECT (monitor), "added",
                    G_CALLBACK (cheese_camera_add_device), camera);
  cheese_camera_device_monitor_coldplug (monitor);
  g_object_unref (monitor);
}

static gboolean
cheese_camera_set_camera_source (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  GError *err = NULL;
  char   *camera_input;

  int                 i;
  CheeseCameraDevice *selected_camera;

  /* If we have a matching video device use that one, otherwise use the first */
  priv->selected_device = 0;
  selected_camera       = g_ptr_array_index (priv->camera_devices, 0);

  for (i = 1; i < priv->num_camera_devices; i++)
  {
    CheeseCameraDevice *device = g_ptr_array_index (priv->camera_devices, i);
    if (g_strcmp0 (cheese_camera_device_get_device_file (device),
                   priv->device_name) == 0)
    {
      selected_camera       = device;
      priv->selected_device = i;
      break;
    }
  }

  camera_input = g_strdup_printf (
    "%s name=video_source device=%s",
    cheese_camera_device_get_src (selected_camera),
    cheese_camera_device_get_device_file (selected_camera));

  priv->video_source = gst_parse_bin_from_description (camera_input, TRUE, &err);
  g_free (camera_input);

  if (priv->video_source == NULL)
  {
    if (err != NULL)
    {
      g_error_free (err);
      err = NULL;
    }
    return FALSE;
  }
  g_object_set (priv->camerabin, "video-source", priv->video_source, NULL);

  return TRUE;
}

static void
cheese_camera_set_error_element_not_found (GError **error, const char *factoryname)
{
  if (error == NULL)
    return;

  if (*error == NULL)
  {
    g_set_error (error, CHEESE_CAMERA_ERROR, CHEESE_CAMERA_ERROR_ELEMENT_NOT_FOUND, "%s.", factoryname);
  }
  else
  {
    /* Ensure that what is found is not a substring of an element; all strings
     * should have a ' ' or nothing to the left and a '.' or ',' to the right */
    gchar *found = g_strrstr ((*error)->message, factoryname);
    gchar  prev  = 0;
    gchar  next  = 0;

    if (found != NULL)
    {
      prev = *(found - 1);
      next = *(found + strlen (factoryname));
    }

    if (found == NULL ||
        ((found != (*error)->message && prev != ' ') && /* Prefix check */
         (next != ',' && next != '.'))) /* Postfix check */
    {
      g_prefix_error (error, "%s, ", factoryname);
    }
  }
}

static void
cheese_camera_set_video_recording (CheeseCamera *camera, GError **error)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);
  GstElement          *video_enc;
  GstElement          *mux;

  /* Setup video-encoder explicitly to be able to set its properties*/

  if ((video_enc = gst_element_factory_make ("theoraenc", "theoraenc")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "theoraenc");
    return;
  }
  g_object_set (priv->camerabin, "video-encoder", video_enc, NULL);
  g_object_set (G_OBJECT (video_enc), "speed-level", 2, NULL);

  if ((mux = gst_element_factory_make ("oggmux", "oggmux")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "oggmux");
    return;
  }
  g_object_set (priv->camerabin, "video-muxer", mux, NULL);
  g_object_set (G_OBJECT (mux),
                "max-delay", (guint64) 10000000,
                "max-page-delay", (guint64) 10000000, NULL);
}

static gboolean
cheese_camera_create_effects_preview_bin (CheeseCamera *camera, GError **error)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  gboolean ok = TRUE;
  GstPad  *pad;
  GError  *err = NULL;

  priv->effects_preview_bin = gst_bin_new ("effects_preview_bin");

  if ((priv->effects_tee = gst_element_factory_make ("tee", "effects_tee")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "tee");
  }
  if ((priv->effects_valve = gst_element_factory_make ("valve", "effects_valve")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "effects_valve");
  }
  priv->effects_downscaler = gst_parse_bin_from_description (
    "videoscale ! video/x-raw-yuv,width=160,height=120 ! ffmpegcolorspace",
    TRUE,
    &err);
  if (priv->effects_downscaler == NULL || err != NULL)
  {
    cheese_camera_set_error_element_not_found (error, "effects_downscaler");
  }

  if (error != NULL && *error != NULL)
    return FALSE;

  gst_bin_add_many (GST_BIN (priv->effects_preview_bin),
                    priv->effects_downscaler, priv->effects_tee,
                    priv->effects_valve, NULL);

  ok &= gst_element_link_many (priv->effects_valve, priv->effects_downscaler,
                               priv->effects_tee, NULL);

  /* add ghostpads */

  pad = gst_element_get_static_pad (priv->effects_valve, "sink");
  gst_element_add_pad (priv->effects_preview_bin, gst_ghost_pad_new ("sink", pad));
  gst_object_unref (GST_OBJECT (pad));

  if (!ok)
    g_error ("Unable to create effects preview bin");

  return TRUE;
}

static gboolean
cheese_camera_create_video_filter_bin (CheeseCamera *camera, GError **error)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  gboolean ok = TRUE;
  GstPad  *pad;

  cheese_camera_create_effects_preview_bin (camera, error);

  priv->video_filter_bin = gst_bin_new ("video_filter_bin");

  if ((priv->camera_tee = gst_element_factory_make ("tee", "camera_tee")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "tee");
  }
  if ((priv->main_valve = gst_element_factory_make ("valve", "main_valve")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "main_valve");
  }
  if ((priv->effect_filter = gst_element_factory_make ("identity", "effect")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "identity");
  }
  if ((priv->video_balance = gst_element_factory_make ("videobalance", "video_balance")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "videobalance");
    return FALSE;
  }
  if ((priv->csp_post_balance = gst_element_factory_make ("ffmpegcolorspace", "csp_post_balance")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "ffmpegcolorspace");
    return FALSE;
  }

  if (error != NULL && *error != NULL)
    return FALSE;

  gst_bin_add_many (GST_BIN (priv->video_filter_bin), priv->camera_tee,
                    priv->main_valve, priv->effect_filter,
                    priv->video_balance, priv->csp_post_balance,
                    priv->effects_preview_bin, NULL);

  ok &= gst_element_link_many (priv->camera_tee, priv->main_valve,
                               priv->effect_filter, priv->video_balance,
                               priv->csp_post_balance, NULL);
  gst_pad_link (gst_element_get_request_pad (priv->camera_tee, "src%d"),
                gst_element_get_static_pad (priv->effects_preview_bin, "sink"));

  /* add ghostpads */

  pad = gst_element_get_static_pad (priv->csp_post_balance, "src");
  gst_element_add_pad (priv->video_filter_bin, gst_ghost_pad_new ("src", pad));
  gst_object_unref (GST_OBJECT (pad));

  pad = gst_element_get_static_pad (priv->camera_tee, "sink");
  gst_element_add_pad (priv->video_filter_bin, gst_ghost_pad_new ("sink", pad));
  gst_object_unref (GST_OBJECT (pad));

  if (!ok)
    g_error ("Unable to create filter bin");

  return TRUE;
}

static int
cheese_camera_get_num_camera_devices (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  return priv->num_camera_devices;
}

/**
 * cheese_camera_get_selected_device:
 * @camera: a #CheeseCamera
 *
 * Returns: (transfer none): a #CheeseCameraDevice or NULL
 */

CheeseCameraDevice *
cheese_camera_get_selected_device (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  if (cheese_camera_get_num_camera_devices (camera) > 0)
    return CHEESE_CAMERA_DEVICE (
             g_ptr_array_index (priv->camera_devices, priv->selected_device));
  else
    return NULL;
}

void
cheese_camera_switch_camera_device (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  gboolean was_recording        = FALSE;
  gboolean pipeline_was_playing = FALSE;

  if (priv->is_recording)
  {
    cheese_camera_stop_video_recording (camera);
    was_recording = TRUE;
  }

  if (priv->pipeline_is_playing)
  {
    cheese_camera_stop (camera);
    pipeline_was_playing = TRUE;
  }

  cheese_camera_set_camera_source (camera);

  if (pipeline_was_playing)
  {
    cheese_camera_play (camera);
  }

  /* if (was_recording)
   * {
   * Restart recording... ?
   * } */
}

/**
 * cheese_camera_play:
 * @camera: a #CheeseCamera
 */

void
cheese_camera_play (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv   = CHEESE_CAMERA_GET_PRIVATE (camera);
  CheeseCameraDevice  *device = g_ptr_array_index (priv->camera_devices, priv->selected_device);
  GstCaps             *caps;

  caps = cheese_camera_device_get_caps_for_format (device, priv->current_format);

  if (gst_caps_is_empty (caps))
  {
    gst_caps_unref (caps);
    g_boxed_free (CHEESE_TYPE_VIDEO_FORMAT, priv->current_format);
    priv->current_format = cheese_camera_device_get_best_format (device);
    g_object_notify (G_OBJECT (camera), "format");
    caps = cheese_camera_device_get_caps_for_format (device, priv->current_format);
  }

  if (!gst_caps_is_empty (caps))
  {
    g_signal_emit_by_name (priv->camerabin, "set-video-resolution-fps",
                           priv->current_format->width,
                           priv->current_format->height, 0, 1, 0);
  }
  gst_caps_unref (caps);

  gst_element_set_state (priv->camerabin, GST_STATE_PLAYING);
  priv->pipeline_is_playing = TRUE;
}

/**
 * cheese_camera_stop:
 * @camera: a #CheeseCamera
 */

void
cheese_camera_stop (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  if (priv->camerabin != NULL)
    gst_element_set_state (priv->camerabin, GST_STATE_NULL);
  priv->pipeline_is_playing = FALSE;
}

static void
cheese_camera_change_effect_filter (CheeseCamera *camera, GstElement *new_filter)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);
  gboolean             ok;

  g_object_set (G_OBJECT (priv->main_valve), "drop", TRUE, NULL);

  gst_element_unlink_many (priv->main_valve, priv->effect_filter,
                           priv->video_balance, NULL);

  g_object_ref (priv->effect_filter);
  gst_bin_remove (GST_BIN (priv->video_filter_bin), priv->effect_filter);
  gst_element_set_state (priv->effect_filter, GST_STATE_NULL);
  g_object_unref (priv->effect_filter);

  gst_bin_add (GST_BIN (priv->video_filter_bin), new_filter);
  ok = gst_element_link_many (priv->main_valve, new_filter,
                              priv->video_balance, NULL);
  gst_element_set_state (new_filter, GST_STATE_PAUSED);

  g_return_if_fail (ok);

  g_object_set (G_OBJECT (priv->main_valve), "drop", FALSE, NULL);

  priv->effect_filter = new_filter;
}

static GstElement *
cheese_camera_element_from_effect (CheeseCamera *camera, CheeseEffect *effect)
{
  char       *effects_pipeline_desc;
  char       *name;
  GstElement *effect_filter;
  GError     *err = NULL;
  char       *effect_desc;
  GstElement *colorspace1;
  GstElement *colorspace2;
  GstPad     *pad;

  g_object_get (G_OBJECT (effect),
                "pipeline_desc", &effect_desc,
                "name", &name, NULL);

  effects_pipeline_desc = g_strconcat ("ffmpegcolorspace name=colorspace1 ! ",
                                       effect_desc,
                                       " ! ffmpegcolorspace name=colorspace2",
                                       NULL);
  effect_filter = gst_parse_bin_from_description (effects_pipeline_desc, FALSE, &err);
  if (!effect_filter || (err != NULL))
  {
    g_error_free (err);
    g_warning ("Error with effect filter %s. Ignored", name);
    return NULL;
  }
  g_free (effects_pipeline_desc);

  /* Add ghost pads to effect_filter bin */
  colorspace1 = gst_bin_get_by_name (GST_BIN (effect_filter), "colorspace1");
  colorspace2 = gst_bin_get_by_name (GST_BIN (effect_filter), "colorspace2");

  pad = gst_element_get_static_pad (colorspace1, "sink");
  gst_element_add_pad (effect_filter, gst_ghost_pad_new ("sink", pad));
  gst_object_unref (GST_OBJECT (pad));

  pad = gst_element_get_static_pad (colorspace2, "src");
  gst_element_add_pad (effect_filter, gst_ghost_pad_new ("src", pad));
  gst_object_unref (GST_OBJECT (pad));

  return effect_filter;
}

/**
 * cheese_camera_set_effect:
 * @camera: a #CheeseCamera
 * @effect: a #CheeseEffect
 */

void
cheese_camera_set_effect (CheeseCamera *camera, CheeseEffect *effect)
{
  GstElement *effect_filter;

  effect_filter = cheese_camera_element_from_effect (camera, effect);
  if (effect_filter != NULL)
    cheese_camera_change_effect_filter (camera, effect_filter);
}

/**
 * cheese_camera_toggle_effects_pipeline:
 * @camera: a #CheeseCamera
 * @active: TRUE is effects pipeline is active, FALSE otherwise
 */

void
cheese_camera_toggle_effects_pipeline (CheeseCamera *camera, gboolean active)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  if (active)
  {
    g_object_set (G_OBJECT (priv->effects_valve), "drop", FALSE, NULL);
    g_object_set (G_OBJECT (priv->main_valve), "drop", TRUE, NULL);
  }
  else
  {
    g_object_set (G_OBJECT (priv->effects_valve), "drop", TRUE, NULL);
    g_object_set (G_OBJECT (priv->main_valve), "drop", FALSE, NULL);
  }
}

/**
 * cheese_camera_connect_effect_texture:
 * @camera: a #CheeseCamera
 * @effect: a #CheeseEffect
 * @texture: a #ClutterTexture
 */

void
cheese_camera_connect_effect_texture (CheeseCamera *camera, CheeseEffect *effect, ClutterTexture *texture)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  GstElement *effect_filter;
  GstElement *display_element;
  GstElement *display_queue;
  GstElement *control_valve;
  gboolean    ok = TRUE;

  g_object_set (G_OBJECT (priv->effects_valve), "drop", TRUE, NULL);

  control_valve = gst_element_factory_make ("valve", NULL);
  g_object_set (G_OBJECT (effect), "control_valve", control_valve, NULL);

  display_queue = gst_element_factory_make ("queue", NULL);

  effect_filter = cheese_camera_element_from_effect (camera, effect);

  display_element = clutter_gst_video_sink_new (texture);
  g_object_set (G_OBJECT (display_element), "async", FALSE, NULL);

  gst_bin_add_many (GST_BIN (priv->video_filter_bin), control_valve, effect_filter, display_queue, display_element, NULL);

  ok = gst_element_link_many (priv->effects_tee, control_valve, effect_filter, display_queue, display_element, NULL);
  g_return_if_fail (ok);

  /* HACK: I don't understand GStreamer enough to know why this works. */
  gst_element_set_state (control_valve, GST_STATE_PLAYING);
  gst_element_set_state (effect_filter, GST_STATE_PLAYING);
  gst_element_set_state (display_queue, GST_STATE_PLAYING);
  gst_element_set_state (display_element, GST_STATE_PLAYING);

  if (!ok)
      g_warning ("Could not create effects pipeline");

  g_object_set (G_OBJECT (priv->effects_valve), "drop", FALSE, NULL);
}

/**
 * cheese_camera_start_video_recording:
 * @camera: a #CheeseCamera
 * @filename: the name of the video file that should be recorded
 */

void
cheese_camera_start_video_recording (CheeseCamera *camera, const char *filename)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  g_object_set (priv->camerabin, "mode", MODE_VIDEO, NULL);
  gst_element_set_state (priv->camerabin, GST_STATE_READY);
  g_object_set (priv->camerabin, "filename", filename, NULL);
  g_signal_emit_by_name (priv->camerabin, "capture-start", 0);
  gst_element_set_state (priv->camerabin, GST_STATE_PLAYING);
  priv->is_recording = TRUE;
}

static gboolean
cheese_camera_force_stop_video_recording (gpointer data)
{
  CheeseCamera        *camera = CHEESE_CAMERA (data);
  CheeseCameraPrivate *priv   = CHEESE_CAMERA_GET_PRIVATE (camera);

  if (priv->is_recording)
  {
    GST_WARNING ("Cannot cleanly shutdown recording pipeline, forcing");
    g_signal_emit (camera, camera_signals[VIDEO_SAVED], 0);

    cheese_camera_stop (camera);
    g_object_set (priv->camerabin, "mode", MODE_IMAGE, NULL);
    priv->is_recording = FALSE;
  }

  return FALSE;
}

/**
 * cheese_camera_stop_video_recording:
 * @camera: a #CheeseCamera
 */

void
cheese_camera_stop_video_recording (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);
  GstState             state;

  gst_element_get_state (priv->camerabin, &state, NULL, 0);

  if (state == GST_STATE_PLAYING)
  {
    g_signal_emit_by_name (priv->camerabin, "capture-stop", 0);
    g_object_set (priv->camerabin, "mode", MODE_IMAGE, NULL);
    priv->is_recording = FALSE;
  }
  else
  {
    cheese_camera_force_stop_video_recording (camera);
  }
}

static void
cheese_camera_image_done_cb (GstElement *camerabin, gchar *filename,
                             CheeseCamera *camera)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);
  g_signal_handler_disconnect (G_OBJECT (priv->camerabin),
                               priv->photo_handler_signal_id);
  priv->photo_handler_signal_id = 0;
  if (priv->photo_filename != NULL)
    g_signal_emit (camera, camera_signals[PHOTO_SAVED], 0);
}

/**
 * cheese_camera_take_photo:
 * @camera: a #CheeseCamera
 * @filename: name of the file to save photo to
 *
 * Returns: TRUE on success, FALSE if an error occurred
 */
gboolean
cheese_camera_take_photo (CheeseCamera *camera, const char *filename)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  if (priv->photo_handler_signal_id != 0)
  {
    GST_WARNING ("Still waiting for previous photo data, ignoring new request");
    return FALSE;
  }
  priv->photo_handler_signal_id = g_signal_connect (G_OBJECT (priv->camerabin),
                                                    "image-done",
                                                    G_CALLBACK (cheese_camera_image_done_cb),
                                                    camera);

  g_free (priv->photo_filename);
  priv->photo_filename = g_strdup (filename);

  /* Take the photo*/

  /* Only copy the data if we're giving away a pixbuf,
   * not if we're throwing everything away straight away */

  if (priv->photo_filename != NULL)
  {
    g_object_set (priv->camerabin, "filename", priv->photo_filename, NULL);
    g_object_set (priv->camerabin, "mode", MODE_IMAGE, NULL);
    g_signal_emit_by_name (priv->camerabin, "capture-start", 0);
  }
  else
  {
    g_signal_handler_disconnect (G_OBJECT (priv->camerabin),
                                 priv->photo_handler_signal_id);
    priv->photo_handler_signal_id = 0;
    return FALSE;
  }

  return TRUE;
}

/**
 * cheese_camera_take_photo_pixbuf:
 * @camera: a #CheeseCamera
 */

gboolean
cheese_camera_take_photo_pixbuf (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);
  GstCaps             *caps;

  if (priv->photo_handler_signal_id != 0)
  {
    GST_WARNING ("Still waiting for previous photo data, ignoring new request");
    return FALSE;
  }
  priv->photo_handler_signal_id = g_signal_connect (G_OBJECT (priv->camerabin),
                                                    "image-done",
                                                    G_CALLBACK (cheese_camera_image_done_cb),
                                                    camera);
  caps = gst_caps_new_simple ("video/x-raw-rgb",
                              "bpp", G_TYPE_INT, 24,
                              "depth", G_TYPE_INT, 24,
                              NULL);
  g_object_set (G_OBJECT (priv->camerabin), "preview-caps", caps, NULL);
  gst_caps_unref (caps);

  if (priv->photo_filename)
    g_free (priv->photo_filename);
  priv->photo_filename = NULL;

  /* Take the photo */

  g_object_set (priv->camerabin, "filename", "/dev/null", NULL);
  g_object_set (priv->camerabin, "mode", MODE_IMAGE, NULL);
  g_signal_emit_by_name (priv->camerabin, "capture-start", 0);

  return TRUE;
}

static void
cheese_camera_finalize (GObject *object)
{
  CheeseCamera *camera;

  camera = CHEESE_CAMERA (object);
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  cheese_camera_stop (camera);

  if (priv->camerabin != NULL)
    gst_object_unref (priv->camerabin);

  if (priv->photo_filename)
    g_free (priv->photo_filename);
  g_free (priv->device_name);
  g_boxed_free (CHEESE_TYPE_VIDEO_FORMAT, priv->current_format);

  /* Free CheeseCameraDevice array */
  g_ptr_array_free (priv->camera_devices, TRUE);

  G_OBJECT_CLASS (cheese_camera_parent_class)->finalize (object);
}

static void
cheese_camera_get_property (GObject *object, guint prop_id, GValue *value,
                            GParamSpec *pspec)
{
  CheeseCamera *self;

  self = CHEESE_CAMERA (object);
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (self);

  switch (prop_id)
  {
    case PROP_VIDEO_TEXTURE:
      g_value_set_pointer (value, priv->video_texture);
      break;
    case PROP_DEVICE_NAME:
      g_value_set_string (value, priv->device_name);
      break;
    case PROP_FORMAT:
      g_value_set_boxed (value, priv->current_format);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
cheese_camera_set_property (GObject *object, guint prop_id, const GValue *value,
                            GParamSpec *pspec)
{
  CheeseCamera *self;

  self = CHEESE_CAMERA (object);
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (self);

  switch (prop_id)
  {
    case PROP_VIDEO_TEXTURE:
      priv->video_texture = g_value_get_pointer (value);
      break;
    case PROP_DEVICE_NAME:
      g_free (priv->device_name);
      priv->device_name = g_value_dup_string (value);
      break;
    case PROP_FORMAT:
      if (priv->current_format != NULL)
        g_boxed_free (CHEESE_TYPE_VIDEO_FORMAT, priv->current_format);
      priv->current_format = g_value_dup_boxed (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
cheese_camera_class_init (CheeseCameraClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  if (cheese_camera_cat == NULL)
    GST_DEBUG_CATEGORY_INIT (cheese_camera_cat,
                             "cheese-camera",
                             0, "Cheese Camera");

  object_class->finalize     = cheese_camera_finalize;
  object_class->get_property = cheese_camera_get_property;
  object_class->set_property = cheese_camera_set_property;

  camera_signals[PHOTO_SAVED] = g_signal_new ("photo-saved", G_OBJECT_CLASS_TYPE (klass),
                                              G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                              G_STRUCT_OFFSET (CheeseCameraClass, photo_saved),
                                              NULL, NULL,
                                              g_cclosure_marshal_VOID__VOID,
                                              G_TYPE_NONE, 0);

  camera_signals[PHOTO_TAKEN] = g_signal_new ("photo-taken", G_OBJECT_CLASS_TYPE (klass),
                                              G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                              G_STRUCT_OFFSET (CheeseCameraClass, photo_taken),
                                              NULL, NULL,
                                              g_cclosure_marshal_VOID__OBJECT,
                                              G_TYPE_NONE, 1, GDK_TYPE_PIXBUF);

  camera_signals[VIDEO_SAVED] = g_signal_new ("video-saved", G_OBJECT_CLASS_TYPE (klass),
                                              G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                              G_STRUCT_OFFSET (CheeseCameraClass, video_saved),
                                              NULL, NULL,
                                              g_cclosure_marshal_VOID__VOID,
                                              G_TYPE_NONE, 0);

  camera_signals[STATE_FLAGS_CHANGED] = g_signal_new ("state-flags-changed", G_OBJECT_CLASS_TYPE (klass),
                                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                                G_STRUCT_OFFSET (CheeseCameraClass, state_flags_changed),
                                                NULL, NULL,
                                                g_cclosure_marshal_VOID__INT,
                                                G_TYPE_NONE, 1, G_TYPE_INT);


  g_object_class_install_property (object_class, PROP_VIDEO_TEXTURE,
                                   g_param_spec_pointer ("video-texture",
                                                         NULL,
                                                         NULL,
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_DEVICE_NAME,
                                   g_param_spec_string ("device-name",
                                                        NULL,
                                                        NULL,
                                                        "",
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_FORMAT,
                                   g_param_spec_boxed ("format",
                                                       NULL,
                                                       NULL,
                                                       CHEESE_TYPE_VIDEO_FORMAT,
                                                       G_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (CheeseCameraPrivate));
}

static void
cheese_camera_init (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  priv->is_recording            = FALSE;
  priv->pipeline_is_playing     = FALSE;
  priv->photo_filename          = NULL;
  priv->camera_devices          = NULL;
  priv->device_name             = NULL;
  priv->photo_handler_signal_id = 0;
  priv->current_format          = NULL;
}

CheeseCamera *
cheese_camera_new (ClutterTexture *video_texture, char *camera_device_name,
                   int x_resolution, int y_resolution)
{
  CheeseCamera      *camera;
  CheeseVideoFormat *format = g_slice_new (CheeseVideoFormat);

  format->width  = x_resolution;
  format->height = y_resolution;

  if (camera_device_name)
  {
    camera = g_object_new (CHEESE_TYPE_CAMERA, "video-texture", video_texture,
                           "device_name", camera_device_name,
                           "format", format, NULL);
  }
  else
  {
    camera = g_object_new (CHEESE_TYPE_CAMERA, "video-texture", video_texture,
                           "format", format, NULL);
  }

  return camera;
}

/**
 * cheese_camera_set_device_by_dev_file:
 * @camera: a #CheeseCamera
 * @file: the filename
 */

void
cheese_camera_set_device_by_dev_file (CheeseCamera *camera, const gchar *file)
{
  g_return_if_fail (CHEESE_IS_CAMERA (camera));
  g_object_set (camera, "device_name", file, NULL);
}

static void
cheese_camera_set_device_by_dev_udi (CheeseCamera *camera, const gchar *udi)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);
  int                  i;

  g_return_if_fail (CHEESE_IS_CAMERA (camera));


  for (i = 0; i < priv->num_camera_devices; i++)
  {
    CheeseCameraDevice *device = g_ptr_array_index (priv->camera_devices, i);
    if (strcmp (cheese_camera_device_get_id (device), udi) == 0)
    {
      g_object_set (camera,
                    "device_name", cheese_camera_device_get_id (device),
                    NULL);
      break;
    }
  }
}

/**
 * cheese_camera_setup:
 * @camera: a #CheeseCamera
 * @id: the device id
 * @error: return location for a GError or NULL
 */

void
cheese_camera_setup (CheeseCamera *camera, const char *id, GError **error)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  GError  *tmp_error = NULL;
  GstElement *video_sink;
  GstCaps *caps;

  cheese_camera_detect_camera_devices (camera);

  if (priv->num_camera_devices < 1)
  {
    g_set_error (error, CHEESE_CAMERA_ERROR, CHEESE_CAMERA_ERROR_NO_DEVICE, _("No device found"));
    return;
  }

  if (id != NULL)
  {
    cheese_camera_set_device_by_dev_udi (camera, id);
  }


  if ((priv->camerabin = gst_element_factory_make ("camerabin", "camerabin")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "camerabin");
  }
  g_object_set (priv->camerabin, "video-capture-height", 0,
                "video-capture-width", 0, NULL);

  /* Create a clutter-gst sink and set it as camerabin sink*/

  if ((video_sink = clutter_gst_video_sink_new (priv->video_texture)) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "cluttervideosink");
  }
  g_object_set (G_OBJECT (video_sink), "async", FALSE, NULL);
  g_object_set (G_OBJECT (priv->camerabin), "viewfinder-sink", video_sink, NULL);

  /* Set flags to enable conversions*/

  g_object_set (G_OBJECT (priv->camerabin), "flags",
                GST_CAMERABIN_FLAG_SOURCE_RESIZE |
                GST_CAMERABIN_FLAG_SOURCE_COLOR_CONVERSION |
                GST_CAMERABIN_FLAG_VIEWFINDER_SCALE |
                GST_CAMERABIN_FLAG_AUDIO_CONVERSION |
                GST_CAMERABIN_FLAG_IMAGE_COLOR_CONVERSION |
                GST_CAMERABIN_FLAG_VIDEO_COLOR_CONVERSION,
                NULL);

  /* Set caps to filter, so it doesn't defaults to I420 format*/

  caps = gst_caps_from_string ("video/x-raw-yuv; video/x-raw-rgb");
  g_object_set (G_OBJECT (priv->camerabin), "filter-caps", caps, NULL);
  gst_caps_unref (caps);

  cheese_camera_set_camera_source (camera);
  cheese_camera_set_video_recording (camera, &tmp_error);
  cheese_camera_create_video_filter_bin (camera, &tmp_error);

  if (tmp_error != NULL || (error != NULL && *error != NULL))
  {
    g_propagate_prefixed_error (error, tmp_error,
                                _("One or more needed GStreamer elements are missing: "));
    GST_WARNING ("%s", (*error)->message);
    return;
  }
  g_object_set (G_OBJECT (priv->camerabin), "video-source-filter", priv->video_filter_bin, NULL);

  priv->bus = gst_element_get_bus (priv->camerabin);
  gst_bus_add_signal_watch (priv->bus);

  g_signal_connect (G_OBJECT (priv->bus), "message",
                    G_CALLBACK (cheese_camera_bus_message_cb), camera);
}

/**
 * cheese_camera_get_camera_devices:
 * @camera: a #CheeseCamera
 *
 * Returns: (element-type Cheese.CameraDevice) (transfer container): Array of #CheeseCameraDevice
 */

GPtrArray *
cheese_camera_get_camera_devices (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv;

  g_return_val_if_fail (CHEESE_IS_CAMERA (camera), NULL);

  priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  return g_ptr_array_ref (priv->camera_devices);
}

/**
 * cheese_camera_get_video_formats:
 * @camera: a #CheeseCamera
 *
 * Returns: (element-type Cheese.VideoFormat) (transfer container): List of #CheeseVideoFormat
 */

GList *
cheese_camera_get_video_formats (CheeseCamera *camera)
{
  CheeseCameraDevice *device;

  g_return_val_if_fail (CHEESE_IS_CAMERA (camera), NULL);

  device = cheese_camera_get_selected_device (camera);

  if (device)
    return cheese_camera_device_get_format_list (device);
  else
    return NULL;
}

static gboolean
cheese_camera_is_playing (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv;

  g_return_val_if_fail (CHEESE_IS_CAMERA (camera), FALSE);

  priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  return priv->pipeline_is_playing;
}

/**
 * cheese_camera_set_video_format:
 * @camera: a #CheeseCamera
 * format: a #CheeseVideoFormat
 */

void
cheese_camera_set_video_format (CheeseCamera *camera, CheeseVideoFormat *format)
{
  CheeseCameraPrivate *priv;

  g_return_if_fail (CHEESE_IS_CAMERA (camera));
  g_return_if_fail (format != NULL);

  priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  if (!(priv->current_format->width == format->width &&
        priv->current_format->height == format->height))
  {
    g_object_set (G_OBJECT (camera), "format", format, NULL);
    if (cheese_camera_is_playing (camera))
    {
      cheese_camera_stop (camera);
      cheese_camera_play (camera);
    }
  }
}

/**
 * cheese_camera_get_current_video_format:
 * @camera: a #CheeseCamera
 *
 * Returns: (transfer none): a #CheeseVideoFormat
 */

const CheeseVideoFormat *
cheese_camera_get_current_video_format (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  g_return_val_if_fail (CHEESE_IS_CAMERA (camera), NULL);

  return priv->current_format;
}

/**
 * cheese_camera_get_balance_property_range:
 * @camera: a #CheeseCamera
 * @property: name of the balance property
 * @min: (out): minimun value
 * @max: (out): maximum value
 * @def: (out): default value
 */

gboolean
cheese_camera_get_balance_property_range (CheeseCamera *camera,
                                          const gchar *property,
                                          gdouble *min, gdouble *max, gdouble *def)
{
  CheeseCameraPrivate *priv;
  GParamSpec          *pspec;

  g_return_val_if_fail (CHEESE_IS_CAMERA (camera), FALSE);

  priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  *min = 0.0;
  *max = 1.0;
  *def = 0.5;

  if (!GST_IS_ELEMENT (priv->video_balance)) return FALSE;

  pspec = g_object_class_find_property (
    G_OBJECT_GET_CLASS (G_OBJECT (priv->video_balance)), property);

  g_return_val_if_fail (G_IS_PARAM_SPEC_DOUBLE (pspec), FALSE);

  *min = G_PARAM_SPEC_DOUBLE (pspec)->minimum;
  *max = G_PARAM_SPEC_DOUBLE (pspec)->maximum;
  *def = G_PARAM_SPEC_DOUBLE (pspec)->default_value;

  return TRUE;
}

/**
 * cheese_camera_set_balance_property:
 * @camera: A #CheeseCamera
 * @property: name of the balance property
 * @value: value to be set
 */

void
cheese_camera_set_balance_property (CheeseCamera *camera, const gchar *property, gdouble value)
{
  CheeseCameraPrivate *priv;

  g_return_if_fail (CHEESE_IS_CAMERA (camera));

  priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  g_object_set (G_OBJECT (priv->video_balance), property, value, NULL);
}
