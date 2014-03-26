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
#include <glib/gi18n-lib.h>
#include <clutter/clutter.h>
#include <clutter-gst/clutter-gst.h>
#include <gst/gst.h>
#include <gst/basecamerabinsrc/gstcamerabin-enum.h>
#include <gst/pbutils/encoding-profile.h>
#include <X11/Xlib.h>

#include "cheese-camera.h"
#include "cheese-camera-device.h"
#include "cheese-camera-device-monitor.h"

#define CHEESE_VIDEO_ENC_PRESET "Profile Realtime"
#define CHEESE_VIDEO_ENC_ALT_PRESET "Cheese Realtime"

/**
 * SECTION:cheese-camera
 * @short_description: A representation of the video capture device inside
 * #CheeseWidget
 * @stability: Unstable
 * @include: cheese/cheese-camera.h
 *
 * #CheeseCamera represents the video capture device used to drive a
 * #CheeseWidget.
 */

struct _CheeseCameraPrivate
{
  GstBus *bus;

  GstElement *camerabin;
  GstElement *video_filter_bin;
  GstElement *effects_preview_bin;

  GstElement *video_source;
  GstElement *camera_source;
  GstElement *video_file_sink;
  GstElement *audio_source;
  GstElement *audio_enc;
  GstElement *video_enc;

  ClutterTexture *video_texture;

  GstElement *effect_filter, *effects_capsfilter;
  GstElement *video_balance;
  GstElement *camera_tee, *effects_tee;
  GstElement *main_valve, *effects_valve;
  gchar *current_effect_desc;

  gboolean is_recording;
  gboolean pipeline_is_playing;
  gboolean effect_pipeline_is_playing;
  gchar *photo_filename;

  guint num_camera_devices;
  gchar *device_node;

  /* an array of CheeseCameraDevices */
  GPtrArray *camera_devices;
  gint x_resolution;
  gint y_resolution;
  guint selected_device;
  CheeseVideoFormat *current_format;

  guint eos_timeout_id;

  CheeseCameraDeviceMonitor *monitor;
};

G_DEFINE_TYPE_WITH_PRIVATE (CheeseCamera, cheese_camera, G_TYPE_OBJECT)

#define CHEESE_CAMERA_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHEESE_TYPE_CAMERA, CheeseCameraPrivate))

#define CHEESE_CAMERA_ERROR cheese_camera_error_quark ()

enum
{
  PROP_0,
  PROP_VIDEO_TEXTURE,
  PROP_DEVICE_NODE,
  PROP_FORMAT,
  PROP_NUM_CAMERA_DEVICES,
  PROP_LAST
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
static GParamSpec *properties[PROP_LAST];

GST_DEBUG_CATEGORY (cheese_camera_cat);
#define GST_CAT_DEFAULT cheese_camera_cat

GQuark cheese_camera_error_quark (void);

GQuark
cheese_camera_error_quark (void)
{
  return g_quark_from_static_string ("cheese-camera-error-quark");
}

/*
 * cheese_camera_photo_data:
 * @camera: a #CheeseCamera
 * @sample: the #GstSample containing photo data
 *
 * Create a #GdkPixbuf containing photo data captured from @camera, and emit it
 * in the ::photo-taken signal.
 */
static void
cheese_camera_photo_data (CheeseCamera *camera, GstSample *sample)
{
  GstBuffer          *buffer;
  GstCaps            *caps;
  const GstStructure *structure;
  gint                width, height, stride;
  GdkPixbuf          *pixbuf;
  const gint          bits_per_pixel = 8;
  guchar             *data = NULL;
  
  CheeseCameraPrivate *priv  = camera->priv;
  GstMapInfo         mapinfo = {0, };

  buffer = gst_sample_get_buffer (sample);
  caps = gst_sample_get_caps (sample);
  structure = gst_caps_get_structure (caps, 0);
  gst_structure_get_int (structure, "width", &width);
  gst_structure_get_int (structure, "height", &height);

  gst_buffer_map (buffer, &mapinfo, GST_MAP_READ);
  stride = mapinfo.size / height;
  data = g_memdup (mapinfo.data, mapinfo.size);
  pixbuf = gdk_pixbuf_new_from_data (data ? data : mapinfo.data,
                                     GDK_COLORSPACE_RGB,
                                     FALSE, bits_per_pixel, width, height, stride,
                                     data ? (GdkPixbufDestroyNotify) g_free : NULL, NULL);

  gst_buffer_unmap (buffer, &mapinfo);
  g_object_set (G_OBJECT (priv->camerabin), "post-previews", FALSE, NULL);
  g_signal_emit (camera, camera_signals[PHOTO_TAKEN], 0, pixbuf);
  g_object_unref (pixbuf);
}

/*
 * cheese_camera_bus_message_cb:
 * @bus: a #GstBus
 * @message: the #GstMessage
 * @camera: the #CheeseCamera
 *
 * Process messages create by the @camera on the @bus. Emit
 * ::state-flags-changed if the state of the camera has changed.
 */
static void
cheese_camera_bus_message_cb (GstBus *bus, GstMessage *message, CheeseCamera *camera)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  switch (GST_MESSAGE_TYPE (message))
  {
    case GST_MESSAGE_WARNING:
    {
      GError *err = NULL;
      gchar *debug = NULL;
      gst_message_parse_warning (message, &err, &debug);

      if (err && err->message) {
        g_warning ("%s: %s\n", err->message, debug);
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
        g_warning ("%s: %s\n", err->message, debug);
        g_error_free (err);
      } else {
        g_warning ("Unparsable GST_MESSAGE_ERROR message.\n");
      }

      cheese_camera_stop (camera);
      g_signal_emit (camera, camera_signals[STATE_FLAGS_CHANGED], 0,
                     GST_STATE_NULL);
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
        {
          g_signal_emit (camera, camera_signals[STATE_FLAGS_CHANGED], 0, new);
          cheese_camera_toggle_effects_pipeline (camera,
                                            priv->effect_pipeline_is_playing);
        }
      }
      break;
    }
    case GST_MESSAGE_ELEMENT:
    {
      const GstStructure *structure;
      GstSample *sample;
      const GValue *image;
      if (strcmp (GST_MESSAGE_SRC_NAME (message), "camera_source") == 0)
      {
        structure = gst_message_get_structure (message);
        if (strcmp (gst_structure_get_name (structure), "preview-image") == 0)
        {
          if (gst_structure_has_field_typed (structure, "sample", GST_TYPE_SAMPLE))
          {
            image = gst_structure_get_value (structure, "sample");
            if (image)
            {
              sample = gst_value_get_sample (image);
              cheese_camera_photo_data (camera, sample);
            }
            else
            {
              g_warning ("Could not get buffer from bus message");
            }
          }
        }
      }
      if (strcmp (GST_MESSAGE_SRC_NAME (message), "camerabin") == 0)
      {
        structure = gst_message_get_structure (message);
        if (strcmp (gst_structure_get_name (structure), "image-done") == 0)
        {
          const gchar *filename = gst_structure_get_string (structure, "filename");
          if (priv->photo_filename != NULL && filename != NULL &&
              (strcmp (priv->photo_filename, filename) == 0))
          {
            g_signal_emit (camera, camera_signals[PHOTO_SAVED], 0);
          }
        }
        else if (strcmp (gst_structure_get_name (structure), "video-done") == 0)
        {
          g_signal_emit (camera, camera_signals[VIDEO_SAVED], 0);
          priv->is_recording = FALSE;
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

/*
 * cheese_camera_add_device:
 * @monitor: a #CheeseCameraDeviceMonitor
 * @device: a #CheeseCameraDevice
 * @camera: a #CheeseCamera
 *
 * Handle the CheeseCameraDeviceMonitor::added signal and add the new
 * #CheeseCameraDevice to the list of current devices.
 */
static void
cheese_camera_add_device (CheeseCameraDeviceMonitor *monitor,
			  CheeseCameraDevice        *device,
                          CheeseCamera              *camera)
{
  CheeseCameraPrivate *priv  = camera->priv;

  g_ptr_array_add (priv->camera_devices, device);
  priv->num_camera_devices++;

  g_object_notify_by_pspec (G_OBJECT (camera), properties[PROP_NUM_CAMERA_DEVICES]);
}

/*
 * cheese_camera_remove_device:
 * @monitor: a #CheeseCameraDeviceMonitor
 * @uuid: UUId of a #CheeseCameraDevice
 * @camera: a #CheeseCamera
 *
 * Handle the CheeseCameraDeviceMonitor::removed signal and remove the
 * #CheeseCameraDevice associated with the UUID from the list of current
 * devices.
 */
static void
cheese_camera_remove_device (CheeseCameraDeviceMonitor *monitor,
                             const gchar               *uuid,
                             CheeseCamera              *camera)
{
  guint i;

  CheeseCameraPrivate *priv  = camera->priv;

  for (i = 0; i < priv->num_camera_devices; i++)
  {
    CheeseCameraDevice *device = (CheeseCameraDevice *) g_ptr_array_index (priv->camera_devices, i);
    const gchar *device_uuid = cheese_camera_device_get_uuid (device);

    if (strcmp (device_uuid, uuid) == 0)
    {
        g_ptr_array_remove (priv->camera_devices, (gpointer) device);
        priv->num_camera_devices--;
        g_object_notify_by_pspec (G_OBJECT (camera), properties[PROP_NUM_CAMERA_DEVICES]);
        break;
    }
  }
}

/*
 * cheese_camera_detect_camera_devices:
 * @camera: a #CheeseCamera
 *
 * Enumerate the physical camera devices present on the system, and add them to
 * the list of #CheeseCameraDevice objects.
 */
static void
cheese_camera_detect_camera_devices (CheeseCamera *camera)
{
  CheeseCameraPrivate       *priv = camera->priv;

  priv->num_camera_devices = 0;
  priv->camera_devices     = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);

  priv->monitor = cheese_camera_device_monitor_new ();
  g_signal_connect (G_OBJECT (priv->monitor), "added",
                    G_CALLBACK (cheese_camera_add_device), camera);
  g_signal_connect (G_OBJECT (priv->monitor), "removed",
                    G_CALLBACK (cheese_camera_remove_device), camera);

  cheese_camera_device_monitor_coldplug (priv->monitor);
}

/*
 * cheese_camera_set_camera_source:
 * @camera: a #CheeseCamera
 *
 * Set the currently-selected video capture device as the source for a video
 * steam.
 *
 * Returns: %TRUE if successful, %FALSE otherwise
 */
static gboolean
cheese_camera_set_camera_source (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv = camera->priv;

  GError *err = NULL;
  gchar  *camera_input;

  guint               i;
  CheeseCameraDevice *selected_camera;

  if (priv->video_source)
    gst_object_unref (priv->video_source);

  /* If we have a matching video device use that one, otherwise use the first */
  priv->selected_device = 0;
  selected_camera       = g_ptr_array_index (priv->camera_devices, 0);

  for (i = 1; i < priv->num_camera_devices; i++)
  {
    CheeseCameraDevice *device = g_ptr_array_index (priv->camera_devices, i);
    if (g_strcmp0 (cheese_camera_device_get_device_node (device),
                   priv->device_node) == 0)
    {
      selected_camera       = device;
      priv->selected_device = i;
      break;
    }
  }

  camera_input = g_strdup_printf (
    "%s name=video_source device=%s ! capsfilter name=video_source_filter",
    cheese_camera_device_get_src (selected_camera),
    cheese_camera_device_get_device_node (selected_camera));

  priv->video_source = gst_parse_bin_from_description (camera_input, TRUE, &err);
  g_free (camera_input);

  if (priv->video_source == NULL)
  {
    g_clear_error(&err);
    return FALSE;
  }

  return TRUE;
}

/*
 * cheese_camera_set_error_element_not_found:
 * @error: return location for errors, or %NULL
 * @factoryname: the name of the #GstElement which was not found
 *
 * Create a #GError to warn that a required GStreamer element was not found.
 */
static void
cheese_camera_set_error_element_not_found (GError **error, const gchar *factoryname)
{
  g_return_if_fail (error == NULL || *error == NULL);

  g_set_error (error, CHEESE_CAMERA_ERROR, CHEESE_CAMERA_ERROR_ELEMENT_NOT_FOUND, "%s%s.", _("One or more needed GStreamer elements are missing: "), factoryname);
}

/*
 * cheese_camera_set_video_recording:
 * @camera: a #CheeseCamera
 * @error: a return location for errors, or %NULL
 *
 */
static void
cheese_camera_set_video_recording (CheeseCamera *camera, GError **error)
{
  CheeseCameraPrivate *priv   = CHEESE_CAMERA_GET_PRIVATE (camera);
  GstEncodingContainerProfile *prof;
  GstEncodingVideoProfile *v_prof;
  GstCaps *caps;
  GstElement *video_enc;
  const gchar *video_preset;
  gboolean res;

  /* Check if we can use global preset for vp8enc. */
  video_enc = gst_element_factory_make ("vp8enc", "vp8enc");
  video_preset = (gchar *) &CHEESE_VIDEO_ENC_PRESET;
  res = gst_preset_load_preset (GST_PRESET (video_enc), video_preset);
  if (res == FALSE) {
    g_warning("Can't find vp8enc preset: \"%s\", using alternate preset:"
        " \"%s\". If you see this, make a bug report!",
        video_preset, CHEESE_VIDEO_ENC_ALT_PRESET);

    /* If global preset not found, then probably we use wrong preset name,
     * or old gstreamer version. In any case, we should try to control
     * keep poker face and not fail. DON'T FORGET TO MAKE A BUG REPORT!*/
    video_preset = (gchar *) &CHEESE_VIDEO_ENC_ALT_PRESET;
    res = gst_preset_load_preset (GST_PRESET (video_enc), video_preset);
    if (res == FALSE) {
      g_warning ("Can't find vp8enc preset: \"%s\", "
          "creating new userspace preset.", video_preset);

      /* Seems like we do first run and userspace preset do not exist.
       * Let us create a new one. It will be probably located some where here:
       * ~/.local/share/gstreamer-1.0/presets/GstVP8Enc.prs */
      g_object_set (G_OBJECT (video_enc), "speed", 2, NULL);
      g_object_set (G_OBJECT (video_enc), "max-latency", 1, NULL);
      gst_preset_save_preset (GST_PRESET (video_enc), video_preset);
    }
  }
  gst_object_unref(video_enc);

  /* create profile for webm encoding */
  caps = gst_caps_from_string("video/webm");
  prof = gst_encoding_container_profile_new("WebM audio/video",
      "Standard WebM/VP8/Vorbis",
      caps, NULL);
  gst_caps_unref (caps);

  caps = gst_caps_from_string("video/x-vp8");
  v_prof = gst_encoding_video_profile_new(caps, NULL, NULL, 0);
  gst_encoding_video_profile_set_variableframerate(v_prof, TRUE);
  gst_encoding_profile_set_preset((GstEncodingProfile*) v_prof, video_preset);
  gst_encoding_container_profile_add_profile(prof, (GstEncodingProfile*) v_prof);
  gst_caps_unref (caps);

  caps = gst_caps_from_string("audio/x-vorbis");
  gst_encoding_container_profile_add_profile(prof,
      (GstEncodingProfile*) gst_encoding_audio_profile_new(caps, NULL, NULL, 0));
  gst_caps_unref (caps);

  g_object_set (priv->camerabin, "video-profile", prof, NULL);
  gst_encoding_profile_unref (prof);
}

/*
 * cheese_camera_create_effects_preview_bin:
 * @camera: a #CheeseCamera
 * @error: a return location for errors
 *
 * Create the #GstBin for effect previews.
 *
 * Returns: %TRUE if the bin creation was successful, %FALSE otherwise
 */
static gboolean
cheese_camera_create_effects_preview_bin (CheeseCamera *camera, GError **error)
{
  CheeseCameraPrivate *priv = camera->priv;

  gboolean ok = TRUE;
  GstElement *scale;
  GstPad  *pad;

  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  priv->effects_preview_bin = gst_bin_new ("effects_preview_bin");

  if ((priv->effects_valve = gst_element_factory_make ("valve", "effects_valve")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "effects_valve");
    return FALSE;
  }
  g_object_set (G_OBJECT (priv->effects_valve), "drop", TRUE, NULL);
  if ((scale = gst_element_factory_make ("videoscale", "effects_scale")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "videoscale");
    return FALSE;
  }
  if ((priv->effects_capsfilter = gst_element_factory_make ("capsfilter", "effects_capsfilter")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "capsfilter");
    return FALSE;
  }
  if ((priv->effects_tee = gst_element_factory_make ("tee", "effects_tee")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "tee");
    return FALSE;
  }

  gst_bin_add_many (GST_BIN (priv->effects_preview_bin), priv->effects_valve,
                    scale, priv->effects_capsfilter, priv->effects_tee, NULL);

  ok &= gst_element_link_many (priv->effects_valve, scale,
                           priv->effects_capsfilter, priv->effects_tee, NULL);

  /* add ghostpads */

  pad = gst_element_get_static_pad (priv->effects_valve, "sink");
  gst_element_add_pad (priv->effects_preview_bin, gst_ghost_pad_new ("sink", pad));
  gst_object_unref (GST_OBJECT (pad));

  if (!ok)
    g_error ("Unable to create effects preview bin");

  return TRUE;
}

/*
 * cheese_camera_create_video_filter_bin:
 * @camera: a #CheeseCamera
 * @error: a return location for errors, or %NULL
 *
 * Create the #GstBin for video filtering.
 *
 * Returns: %TRUE if the bin creation was successful, %FALSE and sets @error
 * otherwise
 */
static gboolean
cheese_camera_create_video_filter_bin (CheeseCamera *camera, GError **error)
{
  CheeseCameraPrivate *priv = camera->priv;

  gboolean ok = TRUE;
  GstPad  *pad;

  cheese_camera_create_effects_preview_bin (camera, error);

  priv->video_filter_bin = gst_bin_new ("video_filter_bin");

  if ((priv->camera_tee = gst_element_factory_make ("tee", "camera_tee")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "tee");
    return FALSE;
  }
  if ((priv->main_valve = gst_element_factory_make ("valve", "main_valve")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "main_valve");
    return FALSE;
  }
  if ((priv->effect_filter = gst_element_factory_make ("identity", "effect")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "identity");
    return FALSE;
  }
  priv->current_effect_desc = g_strdup("identity");
  if ((priv->video_balance = gst_element_factory_make ("videobalance", "video_balance")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "videobalance");
    return FALSE;
  }

  if (error != NULL && *error != NULL)
    return FALSE;

  gst_bin_add_many (GST_BIN (priv->video_filter_bin), priv->camera_tee,
                    priv->main_valve, priv->effect_filter,
                    priv->video_balance, priv->effects_preview_bin, NULL);

  ok &= gst_element_link_many (priv->camera_tee, priv->main_valve,
                               priv->effect_filter, priv->video_balance, NULL);
  gst_pad_link (gst_element_get_request_pad (priv->camera_tee, "src_%u"),
                gst_element_get_static_pad (priv->effects_preview_bin, "sink"));

  /* add ghostpads */

  pad = gst_element_get_static_pad (priv->video_balance, "src");
  gst_element_add_pad (priv->video_filter_bin, gst_ghost_pad_new ("src", pad));
  gst_object_unref (GST_OBJECT (pad));

  pad = gst_element_get_static_pad (priv->camera_tee, "sink");
  gst_element_add_pad (priv->video_filter_bin, gst_ghost_pad_new ("sink", pad));
  gst_object_unref (GST_OBJECT (pad));

  if (!ok)
    g_error ("Unable to create filter bin");

  return TRUE;
}

/*
 * cheese_camera_get_num_camera_devices:
 * @camera: a #CheeseCamera
 *
 * Get the number of #CheeseCameraDevice found on the system managed by
 * @camera.
 *
 * Returns: the number of #CheeseCameraDevice objects on the system
 */
static guint
cheese_camera_get_num_camera_devices (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv = camera->priv;

  return priv->num_camera_devices;
}

/**
 * cheese_camera_get_selected_device:
 * @camera: a #CheeseCamera
 *
 * Get the currently-selected #CheeseCameraDevice of the @camera.
 *
 * Returns: (transfer none): a #CheeseCameraDevice, or %NULL if there is no
 * selected device
 */
CheeseCameraDevice *
cheese_camera_get_selected_device (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv;

  g_return_val_if_fail (CHEESE_IS_CAMERA (camera), NULL);

  priv = camera->priv;

  if (cheese_camera_get_num_camera_devices (camera) > 0)
    return CHEESE_CAMERA_DEVICE (
             g_ptr_array_index (priv->camera_devices, priv->selected_device));
  else
    return NULL;
}

/**
 * cheese_camera_switch_camera_device:
 * @camera: a #CheeseCamera
 *
 * Toggle the playing/recording state of the @camera.
 */
void
cheese_camera_switch_camera_device (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv;
  gboolean pipeline_was_playing;

  g_return_if_fail (CHEESE_IS_CAMERA (camera));

  priv = camera->priv;

  /* gboolean was_recording        = FALSE; */
  pipeline_was_playing = FALSE;

  if (priv->is_recording)
  {
    cheese_camera_stop_video_recording (camera);
    /* was_recording = TRUE; */
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
 *
 * Set the state of the GStreamer pipeline associated with the #CheeseCamera to
 * playing.
 */

static void
cheese_camera_set_new_caps (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv;
  CheeseCameraDevice *device;
  GstCaps *caps, *i420_caps, *video_caps;
  gchar *caps_desc;
  int width, height;

  g_return_if_fail (CHEESE_IS_CAMERA (camera));

  priv = camera->priv;
  device = g_ptr_array_index (priv->camera_devices, priv->selected_device);
  caps = cheese_camera_device_get_caps_for_format (device, priv->current_format);

  if (gst_caps_is_empty (caps))
  {
    gst_caps_unref (caps);
    g_boxed_free (CHEESE_TYPE_VIDEO_FORMAT, priv->current_format);
    priv->current_format = cheese_camera_device_get_best_format (device);
    g_object_notify_by_pspec (G_OBJECT (camera), properties[PROP_FORMAT]);
    caps = cheese_camera_device_get_caps_for_format (device, priv->current_format);
  }

  if (!gst_caps_is_empty (caps))
  {
    GST_INFO_OBJECT (camera, "SETTING caps %" GST_PTR_FORMAT, caps);
    g_object_set (gst_bin_get_by_name (GST_BIN (priv->video_source),
                  "video_source_filter"), "caps", caps, NULL);
    g_object_set (priv->camerabin, "viewfinder-caps", caps,
                  "image-capture-caps", caps, NULL);

    /* GStreamer >= 1.1.4 expects fully-specified video-capture-source caps. */
    i420_caps = gst_caps_new_simple ("video/x-raw",
                                     "format", G_TYPE_STRING, "I420", NULL);
    video_caps = gst_caps_intersect (caps, i420_caps);
    g_object_set (priv->camerabin, "video-capture-caps", video_caps, NULL);

    gst_caps_unref (i420_caps);
    gst_caps_unref (video_caps);
    gst_caps_unref (caps);

    width = priv->current_format->width;
    width = width > 640 ? 640 : width;
    height = width * priv->current_format->height
             / priv->current_format->width;
    /* GStreamer will crash if this is not a multiple of 2! */
    height = (height + 1) & ~1;
    caps_desc = g_strdup_printf ("video/x-raw, width=%d, height=%d", width,
                                 height);
    caps = gst_caps_from_string (caps_desc);
    g_free (caps_desc);
    g_object_set (priv->effects_capsfilter, "caps", caps, NULL);
  }
  gst_caps_unref (caps);
}

void
cheese_camera_play (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv   = CHEESE_CAMERA_GET_PRIVATE (camera);
  cheese_camera_set_new_caps (camera);
  g_object_set (priv->camera_source, "video-source", priv->video_source, NULL);
  g_object_set (priv->main_valve, "drop", FALSE, NULL);
  gst_element_set_state (priv->camerabin, GST_STATE_PLAYING);
  priv->pipeline_is_playing = TRUE;
}

/**
 * cheese_camera_stop:
 * @camera: a #CheeseCamera
 *
 * Set the state of the GStreamer pipeline associated with the #CheeseCamera to
 * NULL.
 */
void
cheese_camera_stop (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv;

  g_return_if_fail (CHEESE_IS_CAMERA (camera));

  priv = camera->priv;

  if (priv->camerabin != NULL)
    gst_element_set_state (priv->camerabin, GST_STATE_NULL);
  priv->pipeline_is_playing = FALSE;
}

/*
 * cheese_camera_change_effect_filter:
 * @camera: a #CheeseCamera
 * @new_filter: the new effect filter to apply
 *
 * Change the current effect to that of @element.
 */
static void
cheese_camera_change_effect_filter (CheeseCamera *camera, GstElement *new_filter)
{
  CheeseCameraPrivate *priv;
  gboolean             ok;

  g_return_if_fail (CHEESE_IS_CAMERA (camera));

  priv = camera->priv;

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

/*
 * cheese_camera_element_from_effect:
 * @camera: a #CheeseCamera
 * @effect: the #CheeseEffect to use as the template
 *
 * Create a new #GstElement based on the @effect template.
 *
 * Returns: a new #GstElement
 */
static GstElement *
cheese_camera_element_from_effect (CheeseCamera *camera, CheeseEffect *effect)
{
  gchar      *effects_pipeline_desc;
  gchar      *name;
  GstElement *effect_filter;
  GError     *err = NULL;
  gchar      *effect_desc;
  GstElement *colorspace1;
  GstElement *colorspace2;
  GstPad     *pad;

  g_object_get (G_OBJECT (effect),
                "pipeline-desc", &effect_desc,
                "name", &name, NULL);

  effects_pipeline_desc = g_strconcat ("videoconvert name=colorspace1 ! ",
                                       effect_desc,
                                       " ! videoconvert name=colorspace2",
                                       NULL);
  g_free (effect_desc);
  effect_filter = gst_parse_bin_from_description (effects_pipeline_desc, FALSE, &err);
  g_free (effects_pipeline_desc);
  if (!effect_filter || (err != NULL))
  {
    g_clear_error (&err);
    g_warning ("Error with effect filter %s. Ignored", name);
    g_free (name);
    return NULL;
  }
  g_free (name);


  /* Add ghost pads to effect_filter bin */
  colorspace1 = gst_bin_get_by_name (GST_BIN (effect_filter), "colorspace1");
  colorspace2 = gst_bin_get_by_name (GST_BIN (effect_filter), "colorspace2");

  pad = gst_element_get_static_pad (colorspace1, "sink");
  gst_element_add_pad (effect_filter, gst_ghost_pad_new ("sink", pad));
  gst_object_unref (GST_OBJECT (pad));
  gst_object_unref (GST_OBJECT (colorspace1));

  pad = gst_element_get_static_pad (colorspace2, "src");
  gst_element_add_pad (effect_filter, gst_ghost_pad_new ("src", pad));
  gst_object_unref (GST_OBJECT (pad));
  gst_object_unref (GST_OBJECT (colorspace2));

  return effect_filter;
}

/**
 * cheese_camera_set_effect:
 * @camera: a #CheeseCamera
 * @effect: a #CheeseEffect
 *
 * Set the @effect on the @camera.
 */
void
cheese_camera_set_effect (CheeseCamera *camera, CheeseEffect *effect)
{
  const gchar *effect_desc = cheese_effect_get_pipeline_desc (effect);
  GstElement *effect_filter;

  g_return_if_fail (CHEESE_IS_CAMERA (camera));

  if (strcmp (camera->priv->current_effect_desc, effect_desc) == 0)
  {
    GST_INFO_OBJECT (camera, "Effect is: \"%s\", not updating", effect_desc);
    return;
  }

  GST_INFO_OBJECT (camera, "Changing effect to: \"%s\"", effect_desc);

  if (strcmp (effect_desc, "identity") == 0)
    effect_filter = gst_element_factory_make ("identity", "effect");
  else
    effect_filter = cheese_camera_element_from_effect (camera, effect);
  if (effect_filter != NULL)
  {
    cheese_camera_change_effect_filter (camera, effect_filter);
    g_free (camera->priv->current_effect_desc);
    camera->priv->current_effect_desc = g_strdup(effect_desc);
  }
}

/**
 * cheese_camera_toggle_effects_pipeline:
 * @camera: a #CheeseCamera
 * @active: %TRUE if effects pipeline is active, %FALSE otherwise
 *
 * Control whether the effects pipeline is enabled for @camera.
 */
void
cheese_camera_toggle_effects_pipeline (CheeseCamera *camera, gboolean active)
{
  CheeseCameraPrivate *priv;

  g_return_if_fail (CHEESE_IS_CAMERA (camera));

  priv = camera->priv;

  if (active)
  {
    g_object_set (G_OBJECT (priv->effects_valve), "drop", FALSE, NULL);
    if (!priv->is_recording)
      g_object_set (G_OBJECT (priv->main_valve), "drop", TRUE, NULL);
  }
  else
  {
    g_object_set (G_OBJECT (priv->effects_valve), "drop", TRUE, NULL);
    g_object_set (G_OBJECT (priv->main_valve), "drop", FALSE, NULL);
  }
  priv->effect_pipeline_is_playing = active;
}

/**
 * cheese_camera_connect_effect_texture:
 * @camera: a #CheeseCamera
 * @effect: a #CheeseEffect
 * @texture: a #ClutterTexture
 *
 * Connect the supplied @texture to the @camera, using @effect.
 */
void
cheese_camera_connect_effect_texture (CheeseCamera *camera, CheeseEffect *effect, ClutterTexture *texture)
{
  CheeseCameraPrivate *priv;
  GstElement *effect_filter;
  GstElement *display_element;
  GstElement *display_queue;
  GstElement *control_valve;
  gboolean ok;
  g_return_if_fail (CHEESE_IS_CAMERA (camera));

  priv = camera->priv;
  ok = TRUE;

  g_object_set (G_OBJECT (priv->effects_valve), "drop", TRUE, NULL);

  control_valve = gst_element_factory_make ("valve", NULL);
  g_object_set (G_OBJECT (effect), "control-valve", control_valve, NULL);

  display_queue = gst_element_factory_make ("queue", NULL);

  effect_filter = cheese_camera_element_from_effect (camera, effect);

  display_element = gst_element_factory_make ("autocluttersink", NULL);
  if (display_element == NULL)
  {
    g_critical ("Unable to create a Clutter sink");
    return;
  }
  g_object_set (G_OBJECT (display_element), "async-handling", FALSE, "texture",
                texture, NULL);

  gst_bin_add_many (GST_BIN (priv->video_filter_bin), control_valve, effect_filter, display_queue, display_element, NULL);

  ok = gst_element_link_many (priv->effects_tee, control_valve, effect_filter, display_queue, display_element, NULL);
  g_return_if_fail (ok);

  /* HACK: I don't understand GStreamer enough to know why this works. */
  gst_element_set_state (control_valve, GST_STATE_PLAYING);
  gst_element_set_state (effect_filter, GST_STATE_PLAYING);
  gst_element_set_state (display_queue, GST_STATE_PLAYING);
  gst_element_set_state (display_element, GST_STATE_PLAYING);
  gst_element_set_locked_state (display_element, TRUE);

  if (!ok)
      g_warning ("Could not create effects pipeline");

  g_object_set (G_OBJECT (priv->effects_valve), "drop", FALSE, NULL);
}

/*
 * cheese_camera_set_tags:
 * @camera: a #CheeseCamera
 *
 * Set tags on the camerabin element, such as the stream creation time and the
 * name of the application. Call this just before starting the capture process.
 */
static void
cheese_camera_set_tags (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv;
  CheeseCameraDevice *device;
  const gchar *device_name;
  GstDateTime *datetime;
  GstTagList *taglist;

  device = cheese_camera_get_selected_device (camera);
  device_name = cheese_camera_device_get_name (device);

  datetime = gst_date_time_new_now_local_time();

  taglist = gst_tag_list_new (
      GST_TAG_APPLICATION_NAME, PACKAGE_STRING,
      GST_TAG_DATE_TIME, datetime,
      GST_TAG_DEVICE_MODEL, device_name,
      GST_TAG_KEYWORDS, PACKAGE_NAME, NULL);

  priv = camera->priv;
  gst_tag_setter_merge_tags (GST_TAG_SETTER (priv->camerabin), taglist,
        GST_TAG_MERGE_REPLACE);

  gst_date_time_unref (datetime);
  gst_tag_list_unref (taglist);
}

/**
 * cheese_camera_start_video_recording:
 * @camera: a #CheeseCamera
 * @filename: (type filename): the name of the video file to where the
 * recording will be saved
 *
 * Start a video recording with the @camera and save it to @filename.
 */
void
cheese_camera_start_video_recording (CheeseCamera *camera, const gchar *filename)
{
  CheeseCameraPrivate *priv;

  g_return_if_fail (CHEESE_IS_CAMERA (camera));

  priv = camera->priv;

  g_object_set (priv->camerabin, "mode", MODE_VIDEO, NULL);
  g_object_set (priv->camerabin, "location", filename, NULL);
  cheese_camera_set_tags (camera);
  g_signal_emit_by_name (priv->camerabin, "start-capture", 0);
  priv->is_recording = TRUE;
}

/*
 * cheese_camera_force_stop_video_recording:
 * @data: a #CheeseCamera
 *
 * Forcibly stop a #CheeseCamera from recording video.
 *
 * Returns: %FALSE
 */
static gboolean
cheese_camera_force_stop_video_recording (gpointer data)
{
  CheeseCamera        *camera = CHEESE_CAMERA (data);
  CheeseCameraPrivate *priv   = camera->priv;

  if (priv->is_recording)
  {
    GST_WARNING ("Cannot cleanly shutdown recording pipeline, forcing");
    g_signal_emit (camera, camera_signals[VIDEO_SAVED], 0);

    cheese_camera_stop (camera);
    cheese_camera_play (camera);
    priv->is_recording = FALSE;
  }

  return FALSE;
}

/**
 * cheese_camera_stop_video_recording:
 * @camera: a #CheeseCamera
 *
 * Stop recording video on the @camera.
 */
void
cheese_camera_stop_video_recording (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv;
  GstState             state;

  g_return_if_fail (CHEESE_IS_CAMERA (camera));

  priv = camera->priv;

  gst_element_get_state (priv->camerabin, &state, NULL, 0);

  if (state == GST_STATE_PLAYING)
  {
    g_signal_emit_by_name (priv->camerabin, "stop-capture", 0);
  }
  else
  {
    cheese_camera_force_stop_video_recording (camera);
  }
}

/**
 * cheese_camera_take_photo:
 * @camera: a #CheeseCamera
 * @filename: (type filename): name of the file to save a photo to
 *
 * Save a photo taken with the @camera to a new file at @filename.
 *
 * Returns: %TRUE on success, %FALSE if an error occurred
 */
gboolean
cheese_camera_take_photo (CheeseCamera *camera, const gchar *filename)
{
  CheeseCameraPrivate *priv;
  gboolean             ready;

  g_return_val_if_fail (CHEESE_IS_CAMERA (camera), FALSE);

  priv = camera->priv;

  g_object_get (priv->camera_source, "ready-for-capture", &ready, NULL);
  if (!ready)
  {
    GST_WARNING ("Still waiting for previous photo data, ignoring new request");
    return FALSE;
  }

  g_free (priv->photo_filename);
  priv->photo_filename = g_strdup (filename);

  /* Take the photo*/

  /* Only copy the data if we're giving away a pixbuf,
   * not if we're throwing everything away straight away */
  if (priv->photo_filename == NULL)
    return FALSE;

  g_object_set (priv->camerabin, "location", priv->photo_filename, NULL);
  g_object_set (priv->camerabin, "mode", MODE_IMAGE, NULL);
  cheese_camera_set_tags (camera);
  g_signal_emit_by_name (priv->camerabin, "start-capture", 0);
  return TRUE;
}

/**
 * cheese_camera_take_photo_pixbuf:
 * @camera: a #CheeseCamera
 *
 * Take a photo with the @camera and emit it in the ::capture-start signal as a
 * #GdkPixbuf.
 *
 * Returns: %TRUE if the photo was successfully captured, %FALSE otherwise
 */
gboolean
cheese_camera_take_photo_pixbuf (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv;
  GstCaps             *caps;
  gboolean             ready;

  g_return_val_if_fail (CHEESE_IS_CAMERA (camera), FALSE);

  priv = camera->priv;

  g_object_get (priv->camera_source, "ready-for-capture", &ready, NULL);
  if (!ready)
  {
    GST_WARNING ("Still waiting for previous photo data, ignoring new request");
    return FALSE;
  }

  caps = gst_caps_new_simple ("video/x-raw",
                              "format", G_TYPE_STRING, "RGB",
                              NULL);
  g_object_set (G_OBJECT (priv->camerabin), "post-previews", TRUE, NULL);
  g_object_set (G_OBJECT (priv->camerabin), "preview-caps", caps, NULL);
  gst_caps_unref (caps);

  if (priv->photo_filename)
    g_free (priv->photo_filename);
  priv->photo_filename = NULL;

  /* Take the photo */

  g_object_set (priv->camerabin, "location", NULL, NULL);
  g_object_set (priv->camerabin, "mode", MODE_IMAGE, NULL);
  g_signal_emit_by_name (priv->camerabin, "start-capture", 0);

  return TRUE;
}

static void
cheese_camera_finalize (GObject *object)
{
  CheeseCamera *camera;
  CheeseCameraPrivate *priv;

  camera = CHEESE_CAMERA (object);
  priv = camera->priv;

  cheese_camera_stop (camera);

  if (priv->camerabin != NULL)
    gst_object_unref (priv->camerabin);

  if (priv->photo_filename)
    g_free (priv->photo_filename);
  g_free (priv->current_effect_desc);
  g_free (priv->device_node);
  g_boxed_free (CHEESE_TYPE_VIDEO_FORMAT, priv->current_format);

  /* Free CheeseCameraDevice array */
  g_ptr_array_free (priv->camera_devices, TRUE);

  g_clear_object (&priv->monitor);

  G_OBJECT_CLASS (cheese_camera_parent_class)->finalize (object);
}

static void
cheese_camera_get_property (GObject *object, guint prop_id, GValue *value,
                            GParamSpec *pspec)
{
  CheeseCamera *self;
  CheeseCameraPrivate *priv;

  self = CHEESE_CAMERA (object);
  priv = self->priv;

  switch (prop_id)
  {
    case PROP_VIDEO_TEXTURE:
      g_value_set_pointer (value, priv->video_texture);
      break;
    case PROP_DEVICE_NODE:
      g_value_set_string (value, priv->device_node);
      break;
    case PROP_FORMAT:
      g_value_set_boxed (value, priv->current_format);
      break;
    case PROP_NUM_CAMERA_DEVICES:
      g_value_set_uint (value, priv->num_camera_devices);
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
  CheeseCameraPrivate *priv;

  self = CHEESE_CAMERA (object);
  priv = self->priv;

  switch (prop_id)
  {
    case PROP_VIDEO_TEXTURE:
      priv->video_texture = g_value_get_pointer (value);
      break;
    case PROP_DEVICE_NODE:
      g_free (priv->device_node);
      priv->device_node = g_value_dup_string (value);
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

  /**
   * CheeseCamera::photo-saved:
   * @camera: a #CheeseCamera
   *
   * Emitted when a photo was saved to disk.
   */
  camera_signals[PHOTO_SAVED] = g_signal_new ("photo-saved", G_OBJECT_CLASS_TYPE (klass),
                                              G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                              G_STRUCT_OFFSET (CheeseCameraClass, photo_saved),
                                              NULL, NULL,
                                              g_cclosure_marshal_VOID__VOID,
                                              G_TYPE_NONE, 0);

  /**
   * CheeseCamera::photo-taken:
   * @camera: a #CheeseCamera
   * @pixbuf: a #GdkPixbuf of the photo which was taken
   *
   * Emitted when a photo was taken.
   */
  camera_signals[PHOTO_TAKEN] = g_signal_new ("photo-taken", G_OBJECT_CLASS_TYPE (klass),
                                              G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                              G_STRUCT_OFFSET (CheeseCameraClass, photo_taken),
                                              NULL, NULL,
                                              g_cclosure_marshal_VOID__OBJECT,
                                              G_TYPE_NONE, 1, GDK_TYPE_PIXBUF);

  /**
   * CheeseCamera::video-saved:
   * @camera: a #CheeseCamera
   *
   * Emitted when a video was saved to disk.
   */
  camera_signals[VIDEO_SAVED] = g_signal_new ("video-saved", G_OBJECT_CLASS_TYPE (klass),
                                              G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                              G_STRUCT_OFFSET (CheeseCameraClass, video_saved),
                                              NULL, NULL,
                                              g_cclosure_marshal_VOID__VOID,
                                              G_TYPE_NONE, 0);

  /**
   * CheeseCamera::state-flags-changed:
   * @camera: a #CheeseCamera
   * @state: the #GstState which @camera changed to
   *
   * Emitted when the state of the @camera #GstElement changed.
   */
  camera_signals[STATE_FLAGS_CHANGED] = g_signal_new ("state-flags-changed", G_OBJECT_CLASS_TYPE (klass),
                                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                                G_STRUCT_OFFSET (CheeseCameraClass, state_flags_changed),
                                                NULL, NULL,
                                                g_cclosure_marshal_VOID__INT,
                                                G_TYPE_NONE, 1, G_TYPE_INT);


  /**
   * CheeseCamera:video-texture:
   *
   * The video texture for the #CheeseCamera to render into.
   */
  properties[PROP_VIDEO_TEXTURE] = g_param_spec_pointer ("video-texture",
                                                         "Video texture",
                                                         "The video texture for the CheeseCamera to render into",
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_STRINGS);

  /**
   * CheeseCamera:device-node:
   *
   * The path to the device node for the video capture device.
   */
  properties[PROP_DEVICE_NODE] = g_param_spec_string ("device-node",
                                                      "Device node",
                                                      "The path to the device node for the video capture device",
                                                      "",
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_STATIC_STRINGS);

  /**
   * CheeseCamera:format:
   *
   * The format of the video capture device.
   */
  properties[PROP_FORMAT] = g_param_spec_boxed ("format",
                                                "Video format",
                                                "The format of the video capture device",
                                                CHEESE_TYPE_VIDEO_FORMAT,
                                                G_PARAM_READWRITE |
                                                G_PARAM_STATIC_STRINGS);

  /**
   * CheeseCamera:num-camera-devices:
   *
   * The currently number of camera devices available for being used.
   */

  properties[PROP_NUM_CAMERA_DEVICES] = g_param_spec_uint ("num-camera-devices",
                                                           "Number of camera devices",
                                                           "The currently number of camera devices available on the system",
                                                           0,
                                                           G_MAXUINT8,
                                                           0,
                                                           G_PARAM_READABLE |
                                                           G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST, properties);
}

static void
cheese_camera_init (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv = camera->priv = cheese_camera_get_instance_private (camera);

  priv->is_recording            = FALSE;
  priv->pipeline_is_playing     = FALSE;
  priv->photo_filename          = NULL;
  priv->camera_devices          = NULL;
  priv->device_node             = NULL;
  priv->current_format          = NULL;
  priv->monitor                 = NULL;
  priv->camera_source           = NULL;
  priv->video_source            = NULL;
}

/**
 * cheese_camera_new:
 * @video_texture: a #ClutterTexture
 * @camera_device_node: (allow-none): the device node path
 * @x_resolution: the resolution width
 * @y_resolution: the resolution height
 *
 * Create a new #CheeseCamera object.
 *
 * Returns: a new #CheeseCamera
 */
CheeseCamera *
cheese_camera_new (ClutterTexture *video_texture, const gchar *camera_device_node,
                   gint x_resolution, gint y_resolution)
{
  CheeseCamera      *camera;
  CheeseVideoFormat format = { x_resolution, y_resolution };

  if (camera_device_node)
  {
    camera = g_object_new (CHEESE_TYPE_CAMERA, "video-texture", video_texture,
                           "device-node", camera_device_node,
                           "format", &format, NULL);
  }
  else
  {
    camera = g_object_new (CHEESE_TYPE_CAMERA, "video-texture", video_texture,
                           "format", &format, NULL);
  }

  return camera;
}

/**
 * cheese_camera_set_device_by_device_node:
 * @camera: a #CheeseCamera
 * @file: (type filename): the device node path
 *
 * Set the active video capture device of the @camera, matching by device node
 * path.
 */
void
cheese_camera_set_device_by_device_node (CheeseCamera *camera, const gchar *file)
{
  g_return_if_fail (CHEESE_IS_CAMERA (camera));

  g_object_set (camera, "device-node", file, NULL);
}

/*
 * cheese_camera_set_device_by_uuid:
 * @camera: a #CheeseCamera
 * @uuid: UUID of a #CheeseCameraDevice
 *
 * Set the active video capture device of the @camera, matching by UUID.
 */
static void
cheese_camera_set_device_by_dev_uuid (CheeseCamera *camera, const gchar *uuid)
{
  CheeseCameraPrivate *priv = camera->priv;
  gint                 i;

  for (i = 0; i < priv->num_camera_devices; i++)
  {
    CheeseCameraDevice *device = g_ptr_array_index (priv->camera_devices, i);
    if (strcmp (cheese_camera_device_get_uuid (device), uuid) == 0)
    {
      g_object_set (camera,
                    "device-node", cheese_camera_device_get_uuid (device),
                    NULL);
      break;
    }
  }
}

/**
 * cheese_camera_setup:
 * @camera: a #CheeseCamera
 * @uuid: (allow-none): UUID of the video capture device, or %NULL
 * @error: return location for a #GError, or %NULL
 *
 * Setup a video capture device.
 */
void
cheese_camera_setup (CheeseCamera *camera, const gchar *uuid, GError **error)
{
  CheeseCameraPrivate *priv;
  GError  *tmp_error = NULL;
  GstElement *video_sink;

  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (CHEESE_IS_CAMERA (camera));

  priv = camera->priv;

  cheese_camera_detect_camera_devices (camera);

  if (priv->num_camera_devices < 1)
  {
    g_set_error (error, CHEESE_CAMERA_ERROR, CHEESE_CAMERA_ERROR_NO_DEVICE, _("No device found"));
    return;
  }

  if (uuid != NULL)
  {
    cheese_camera_set_device_by_dev_uuid (camera, uuid);
  }


  if ((priv->camerabin = gst_element_factory_make ("camerabin", "camerabin")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "camerabin");
  }
  if ((priv->camera_source = gst_element_factory_make ("wrappercamerabinsrc", "camera_source")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "wrappercamerabinsrc");
  }
  g_object_set (priv->camerabin, "camera-source", priv->camera_source, NULL);

  /* Create a clutter-gst sink and set it as camerabin sink*/

  if ((video_sink = gst_element_factory_make ("autocluttersink",
                                              "cluttersink")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "cluttervideosink");
    return;
  }
  g_object_set (G_OBJECT (video_sink), "texture", priv->video_texture,
                "async-handling", FALSE, NULL);
  g_object_set (G_OBJECT (priv->camerabin), "viewfinder-sink", video_sink, NULL);

  /* Set flags to enable conversions*/

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

  g_object_set (G_OBJECT (priv->camera_source), "video-source-filter", priv->video_filter_bin, NULL);

  priv->bus = gst_element_get_bus (priv->camerabin);
  gst_bus_add_signal_watch (priv->bus);

  g_signal_connect (G_OBJECT (priv->bus), "message",
                    G_CALLBACK (cheese_camera_bus_message_cb), camera);
}

/**
 * cheese_camera_get_camera_devices:
 * @camera: a #CheeseCamera
 *
 * Get the list of #CheeseCameraDevice objects, representing active video
 * capture devices on the system.
 *
 * Returns: (element-type Cheese.CameraDevice) (transfer container): an array
 * of #CheeseCameraDevice
 */
GPtrArray *
cheese_camera_get_camera_devices (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv;

  g_return_val_if_fail (CHEESE_IS_CAMERA (camera), NULL);

  priv = camera->priv;

  return g_ptr_array_ref (priv->camera_devices);
}

/**
 * cheese_camera_get_video_formats:
 * @camera: a #CheeseCamera
 *
 * Gets the list of #CheeseVideoFormat supported by the selected
 * #CheeseCameraDevice on the @camera.
 *
 * Returns: (element-type Cheese.VideoFormat) (transfer container): a #GList of
 * #CheeseVideoFormat, or %NULL if there was no device selected
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

/*
 * cheese_camera_is_playing:
 * @camera: a #CheeseCamera
 *
 * Get whether the @camera is in the playing state.
 *
 * Returns: %TRUE if the #CheeseCamera is in the playing state, %FALSE
 * otherwise
 */
static gboolean
cheese_camera_is_playing (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv = camera->priv;

  return priv->pipeline_is_playing;
}

/**
 * cheese_camera_set_video_format:
 * @camera: a #CheeseCamera
 * @format: a #CheeseVideoFormat
 *
 * Sets a #CheeseVideoFormat on a #CheeseCamera, restarting the video stream if
 * necessary.
 */
void
cheese_camera_set_video_format (CheeseCamera *camera, CheeseVideoFormat *format)
{
  CheeseCameraPrivate *priv;

  g_return_if_fail (CHEESE_IS_CAMERA (camera) || format != NULL);

  priv = camera->priv;

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
 * Get the #CheeseVideoFormat that is currently set on the @camera.
 *
 * Returns: (transfer none): the #CheeseVideoFormat set on the #CheeseCamera
 */
const CheeseVideoFormat *
cheese_camera_get_current_video_format (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv;

  g_return_val_if_fail (CHEESE_IS_CAMERA (camera), NULL);

  priv = camera->priv;

  return priv->current_format;
}

/**
 * cheese_camera_get_balance_property_range:
 * @camera: a #CheeseCamera
 * @property: name of the balance property
 * @min: (out): minimum value
 * @max: (out): maximum value
 * @def: (out): default value
 *
 * Get the minimum, maximum and default values for the requested @property of
 * the @camera.
 *
 * Returns: %TRUE if the operation was successful, %FALSE otherwise
 */
gboolean
cheese_camera_get_balance_property_range (CheeseCamera *camera,
                                          const gchar *property,
                                          gdouble *min, gdouble *max, gdouble *def)
{
  CheeseCameraPrivate *priv;
  GParamSpec          *pspec;

  g_return_val_if_fail (CHEESE_IS_CAMERA (camera), FALSE);

  priv = camera->priv;

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
 *
 * Set the requested @property on the @camera to @value.
 */
void
cheese_camera_set_balance_property (CheeseCamera *camera, const gchar *property, gdouble value)
{
  CheeseCameraPrivate *priv;

  g_return_if_fail (CHEESE_IS_CAMERA (camera));

  priv = camera->priv;

  g_object_set (G_OBJECT (priv->video_balance), property, value, NULL);
}

/**
 * cheese_camera_get_recorded_time:
 * @camera: A #CheeseCamera
 *
 * Get a string representation of the playing time
 * of the current video recording
 *
 * Returns: A string with the time representation.
 */
gchar *
cheese_camera_get_recorded_time (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv = camera->priv;
  GstFormat format = GST_FORMAT_TIME;
  gint64 curtime;
  GstElement *videosink;
  const gint TUNIT_60 = 60;
  gint total_time;
  gint hours;
  gint minutes;
  gint seconds;
  gboolean ret = FALSE;

  g_return_val_if_fail (CHEESE_IS_CAMERA (camera), NULL);

  videosink = gst_bin_get_by_name (GST_BIN_CAST (priv->camerabin), "videobin-filesink");
  if (videosink) {
    ret = gst_element_query_position (videosink, format, &curtime);
    gst_object_unref (videosink);
  }
  if (ret) {

    // Substract seconds, minutes and hours.
    total_time = GST_TIME_AS_SECONDS (curtime);
    seconds = total_time % TUNIT_60;
    total_time = total_time - seconds;
    minutes = (total_time % (TUNIT_60 * TUNIT_60)) / TUNIT_60;
    total_time = total_time - (minutes * TUNIT_60);
    hours = total_time / (TUNIT_60 * TUNIT_60);

    /* Translators: This is a time format, like "09:05:02" for 9
     * hours, 5 minutes, and 2 seconds. You may change ":" to
     * the separator that your locale uses or use "%Id" instead
     * of "%d" if your locale uses localized digits.
     */
    return g_strdup_printf (C_("time format", "%02i:%02i:%02i"),
                            hours, minutes, seconds);
  } else {
    GST_WARNING ("Failed to get time from video filesink from camerabin");
    return NULL;
  }
}
