/*
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

#include <string.h>
#include <glib-object.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gst/gst.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <X11/Xlib.h>

#include "cheese-camera.h"
#include "cheese-camera-device.h"
#include "cheese-camera-device-monitor.h"

G_DEFINE_TYPE (CheeseCamera, cheese_camera, G_TYPE_OBJECT)

#define CHEESE_CAMERA_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHEESE_TYPE_CAMERA, CheeseCameraPrivate))

#define CHEESE_CAMERA_ERROR cheese_camera_error_quark ()

typedef struct
{
  GtkWidget *video_window;

  GstElement *pipeline;
  GstBus *bus;

  /* We build the active pipeline by linking the appropriate pipelines listed below*/
  GstElement *camera_source_bin;
  GstElement *video_display_bin;
  GstElement *photo_save_bin;
  GstElement *video_save_bin;

  GstElement *video_source;
  GstElement *capsfilter;
  GstElement *video_file_sink;
  GstElement *photo_sink;
  GstElement *audio_source;
  GstElement *audio_enc;
  GstElement *video_enc;

  GstElement *effect_filter, *csp_post_effect;
  GstElement *video_balance, *csp_post_balance;

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
  PROP_VIDEO_WINDOW,
  PROP_DEVICE_NAME,
  PROP_FORMAT,
};

enum
{
  PHOTO_SAVED,
  PHOTO_TAKEN,
  VIDEO_SAVED,
  LAST_SIGNAL
};

static guint camera_signals[LAST_SIGNAL];

typedef enum
{
  RGB,
  YUV
} VideoColorSpace;

typedef struct
{
  CheeseCameraEffect effect;
  const char *pipeline_desc;
  VideoColorSpace colorspace; /* The color space the effect works in */
} EffectToPipelineDesc;


static const EffectToPipelineDesc EFFECT_TO_PIPELINE_DESC[] = {
  {CHEESE_CAMERA_EFFECT_NO_EFFECT,       "identity",                             RGB},
  {CHEESE_CAMERA_EFFECT_MAUVE,           "videobalance saturation=1.5 hue=+0.5", YUV},
  {CHEESE_CAMERA_EFFECT_NOIR_BLANC,      "videobalance saturation=0",            YUV},
  {CHEESE_CAMERA_EFFECT_SATURATION,      "videobalance saturation=2",            YUV},
  {CHEESE_CAMERA_EFFECT_HULK,            "videobalance saturation=1.5 hue=-0.5", YUV},
  {CHEESE_CAMERA_EFFECT_VERTICAL_FLIP,   "videoflip method=5",                   YUV},
  {CHEESE_CAMERA_EFFECT_HORIZONTAL_FLIP, "videoflip method=4",                   YUV},
  {CHEESE_CAMERA_EFFECT_SHAGADELIC,      "shagadelictv",                         RGB},
  {CHEESE_CAMERA_EFFECT_VERTIGO,         "vertigotv",                            RGB},
  {CHEESE_CAMERA_EFFECT_EDGE,            "edgetv",                               RGB},
  {CHEESE_CAMERA_EFFECT_DICE,            "dicetv",                               RGB},
  {CHEESE_CAMERA_EFFECT_WARP,            "warptv",                               RGB}
};

static const int NUM_EFFECTS = G_N_ELEMENTS (EFFECT_TO_PIPELINE_DESC);

GST_DEBUG_CATEGORY (cheese_camera_cat);
#define GST_CAT_DEFAULT cheese_camera_cat

GQuark
cheese_camera_error_quark (void)
{
  return g_quark_from_static_string ("cheese-camera-error-quark");
}

static GstBusSyncReply
cheese_camera_bus_sync_handler (GstBus *bus, GstMessage *message, CheeseCamera *camera)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);
  GstXOverlay         *overlay;

  if (GST_MESSAGE_TYPE (message) != GST_MESSAGE_ELEMENT)
    return GST_BUS_PASS;

  if (!gst_structure_has_name (message->structure, "prepare-xwindow-id"))
    return GST_BUS_PASS;

  overlay = GST_X_OVERLAY (GST_MESSAGE_SRC (message));

  if (g_object_class_find_property (G_OBJECT_GET_CLASS (overlay),
                                    "force-aspect-ratio"))
    g_object_set (G_OBJECT (overlay), "force-aspect-ratio", TRUE, NULL);

  gst_x_overlay_set_xwindow_id (overlay,
                                GDK_WINDOW_XWINDOW (gtk_widget_get_window (priv->video_window)));

  gst_message_unref (message);

  return GST_BUS_DROP;
}

static void
cheese_camera_change_sink (CheeseCamera *camera, GstElement *src,
                           GstElement *new_sink, GstElement *old_sink)
{
  CheeseCameraPrivate *priv       = CHEESE_CAMERA_GET_PRIVATE (camera);
  gboolean             is_playing = priv->pipeline_is_playing;

  cheese_camera_stop (camera);

  gst_element_unlink (src, old_sink);
  gst_object_ref (old_sink);
  gst_bin_remove (GST_BIN (priv->pipeline), old_sink);

  gst_bin_add (GST_BIN (priv->pipeline), new_sink);
  gst_element_link (src, new_sink);

  if (is_playing)
    cheese_camera_play (camera);
}

static gboolean
cheese_camera_expose_cb (GtkWidget *widget, GdkEventExpose *event, CheeseCamera *camera)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);
  GtkAllocation        allocation;
  GstState             state;
  GstXOverlay         *overlay = GST_X_OVERLAY (gst_bin_get_by_interface (GST_BIN (priv->pipeline),
                                                                          GST_TYPE_X_OVERLAY));

  gst_element_get_state (priv->pipeline, &state, NULL, 0);

  if ((state < GST_STATE_PLAYING) || (overlay == NULL))
  {
    gtk_widget_get_allocation (widget, &allocation);
    gdk_draw_rectangle (gtk_widget_get_window (widget),
                        gtk_widget_get_style (widget)->black_gc, TRUE,
                        0, 0, allocation.width, allocation.height);
  }
  else
  {
    gst_x_overlay_expose (overlay);
  }

  return FALSE;
}

static void
cheese_camera_photo_data_cb (GstElement *element, GstBuffer *buffer,
                             GstPad *pad, CheeseCamera *camera)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  GstCaps            *caps;
  const GstStructure *structure;
  int                 width, height, stride;
  GdkPixbuf          *pixbuf;
  const int           bits_per_pixel = 8;
  guchar             *data;

  caps      = gst_buffer_get_caps (buffer);
  structure = gst_caps_get_structure (caps, 0);
  gst_structure_get_int (structure, "width", &width);
  gst_structure_get_int (structure, "height", &height);

  stride = buffer->size / height;

  /* Only copy the data if we're giving away a pixbuf,
   * not if we're throwing everything away straight away */
  if (priv->photo_filename != NULL)
    data = NULL;
  else
    data = g_memdup (GST_BUFFER_DATA (buffer), buffer->size);
  pixbuf = gdk_pixbuf_new_from_data (data ? data : GST_BUFFER_DATA (buffer),
                                     GDK_COLORSPACE_RGB,
                                     FALSE, bits_per_pixel, width, height, stride,
                                     data ? (GdkPixbufDestroyNotify) g_free : NULL, NULL);

  g_signal_handler_disconnect (G_OBJECT (priv->photo_sink),
                               priv->photo_handler_signal_id);
  priv->photo_handler_signal_id = 0;

  if (priv->photo_filename != NULL)
  {
    gdk_pixbuf_save (pixbuf, priv->photo_filename, "jpeg", NULL, NULL);
    g_object_unref (G_OBJECT (pixbuf));

    g_free (priv->photo_filename);
    priv->photo_filename = NULL;

    g_signal_emit (camera, camera_signals[PHOTO_SAVED], 0);
  }
  else
  {
    g_signal_emit (camera, camera_signals[PHOTO_TAKEN], 0, pixbuf);
    g_object_unref (pixbuf);
  }
}

static void
cheese_camera_bus_message_cb (GstBus *bus, GstMessage *message, CheeseCamera *camera)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  if (GST_MESSAGE_TYPE (message) == GST_MESSAGE_EOS)
  {
    if (priv->is_recording)
    {
      GST_DEBUG ("Received EOS message");

      g_source_remove (priv->eos_timeout_id);

      /* emit signal by name here as the camera_signals array is empty in this thread */
      /* TODO: really understand how threads and static works and why this is needed */
      g_signal_emit_by_name (camera, "video-saved", NULL);

      cheese_camera_change_sink (camera, priv->video_display_bin,
                                 priv->photo_save_bin, priv->video_save_bin);
      priv->is_recording = FALSE;
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
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);
  GError *error = NULL;

  CheeseCameraDevice *device = cheese_camera_device_new (id,
                                                         device_file,
                                                         product_name,
                                                         api_version,
                                                         &error);
  if (device == NULL)
    GST_WARNING ("Device initialization for %s failed: %s ",
                 device_file,
                 (error != NULL) ? error->message : "Unknown reason");
  else  {
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
cheese_camera_create_camera_source_bin (CheeseCamera *camera)
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
    "%s name=video_source device=%s ! capsfilter name=capsfilter ! identity",
    cheese_camera_device_get_src (selected_camera),
    cheese_camera_device_get_device_file (selected_camera));

  priv->camera_source_bin = gst_parse_bin_from_description (camera_input,
                                                            TRUE, &err);
  g_free (camera_input);

  if (priv->camera_source_bin == NULL)
  {
    goto fallback;
  }

  priv->video_source = gst_bin_get_by_name (GST_BIN (priv->camera_source_bin), "video_source");
  priv->capsfilter   = gst_bin_get_by_name (GST_BIN (priv->camera_source_bin), "capsfilter");

  return TRUE;

fallback:
  if (err != NULL)
  {
    g_error_free (err);
    err = NULL;
  }
  return FALSE;
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

static gboolean
cheese_camera_create_video_display_bin (CheeseCamera *camera, GError **error)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  GstElement *tee, *video_display_queue, *video_scale, *video_sink, *save_queue;

  gboolean ok;
  GstPad  *pad;

  priv->video_display_bin = gst_bin_new ("video_display_bin");

  cheese_camera_create_camera_source_bin (camera);

  if ((priv->effect_filter = gst_element_factory_make ("identity", "effect")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "identity");
  }
  if ((priv->csp_post_effect = gst_element_factory_make ("ffmpegcolorspace", "csp_post_effect")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "ffmpegcolorspace");
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

  if ((tee = gst_element_factory_make ("tee", "tee")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "tee");
  }

  if ((save_queue = gst_element_factory_make ("queue", "save_queue")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "queue");
  }

  if ((video_display_queue = gst_element_factory_make ("queue", "video_display_queue")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "queue");
  }

  if ((video_scale = gst_element_factory_make ("videoscale", "video_scale")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "videoscale");
  }
  else
  {
    /* Use bilinear scaling */
    g_object_set (video_scale, "method", 1, NULL);
  }

  if ((video_sink = gst_element_factory_make ("gconfvideosink", "video_sink")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "gconfvideosink");
  }

  if (error != NULL && *error != NULL)
    return FALSE;

  gst_bin_add_many (GST_BIN (priv->video_display_bin), priv->camera_source_bin,
                    priv->effect_filter, priv->csp_post_effect,
                    priv->video_balance, priv->csp_post_balance,
                    tee, save_queue,
                    video_display_queue, video_scale, video_sink, NULL);

  ok = gst_element_link_many (priv->camera_source_bin, priv->effect_filter,
                              priv->csp_post_effect,
                              priv->video_balance, priv->csp_post_balance,
                              tee, NULL);

  ok &= gst_element_link_many (tee, save_queue, NULL);
  ok &= gst_element_link_many (tee, video_display_queue, video_scale, video_sink, NULL);

  /* add ghostpad */
  pad = gst_element_get_pad (save_queue, "src");
  gst_element_add_pad (priv->video_display_bin, gst_ghost_pad_new ("src", pad));
  gst_object_unref (GST_OBJECT (pad));


  if (!ok)
    g_error ("Unable to create display pipeline");

  return TRUE;
}

static gboolean
cheese_camera_create_photo_save_bin (CheeseCamera *camera, GError **error)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  GstElement *csp_photo_save_bin;

  gboolean ok;
  GstPad  *pad;
  GstCaps *caps;

  priv->photo_save_bin = gst_bin_new ("photo_save_bin");

  if ((csp_photo_save_bin = gst_element_factory_make ("ffmpegcolorspace", "csp_photo_save_bin")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "ffmpegcolorspace");
  }
  if ((priv->photo_sink = gst_element_factory_make ("fakesink", "photo_sink")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "fakesink");
  }

  if (error != NULL && *error != NULL)
    return FALSE;

  gst_bin_add_many (GST_BIN (priv->photo_save_bin), csp_photo_save_bin,
                    priv->photo_sink, NULL);

  /* add ghostpad */
  pad = gst_element_get_pad (csp_photo_save_bin, "sink");
  gst_element_add_pad (priv->photo_save_bin, gst_ghost_pad_new ("sink", pad));
  gst_object_unref (GST_OBJECT (pad));

  caps = gst_caps_new_simple ("video/x-raw-rgb",
                              "bpp", G_TYPE_INT, 24,
                              "depth", G_TYPE_INT, 24,
                              NULL);
  ok = gst_element_link_filtered (csp_photo_save_bin, priv->photo_sink, caps);
  gst_caps_unref (caps);

  g_object_set (G_OBJECT (priv->photo_sink), "signal-handoffs", TRUE, NULL);

  if (!ok)
    g_error ("Unable to create photo save pipeline");

  return TRUE;
}

static gboolean
cheese_camera_create_video_save_bin (CheeseCamera *camera, GError **error)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  GstElement *audio_queue, *audio_convert, *audio_enc;
  GstElement *video_save_csp, *video_save_rate, *video_save_scale, *video_enc;
  GstElement *mux;
  GstPad     *pad;
  gboolean    ok;

  priv->video_save_bin = gst_bin_new ("video_save_bin");

  if ((priv->audio_source = gst_element_factory_make ("gconfaudiosrc", "audio_source")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "gconfaudiosrc");
  }
  if ((audio_queue = gst_element_factory_make ("queue", "audio_queue")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "queue");
  }
  if ((audio_convert = gst_element_factory_make ("audioconvert", "audio_convert")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "audioconvert");
  }
  if ((audio_enc = gst_element_factory_make ("vorbisenc", "audio_enc")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "vorbisenc");
  }

  if ((video_save_csp = gst_element_factory_make ("ffmpegcolorspace", "video_save_csp")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "ffmpegcolorspace");
  }
  if ((video_enc = gst_element_factory_make ("theoraenc", "video_enc")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "theoraenc");
  }
  else
  {
    g_object_set (video_enc, "keyframe-force", 1, NULL);
  }

  if ((video_save_rate = gst_element_factory_make ("videorate", "video_save_rate")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "videorate");
  }
  if ((video_save_scale = gst_element_factory_make ("videoscale", "video_save_scale")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "videoscale");
  }
  else
  {
    /* Use bilinear scaling */
    g_object_set (video_save_scale, "method", 1, NULL);
  }

  if ((mux = gst_element_factory_make ("oggmux", "mux")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "oggmux");
  }
  else
  {
    g_object_set (G_OBJECT (mux),
                  "max-delay", (guint64) 10000000,
                  "max-page-delay", (guint64) 10000000, NULL);
  }

  if ((priv->video_file_sink = gst_element_factory_make ("filesink", "video_file_sink")) == NULL)
  {
    cheese_camera_set_error_element_not_found (error, "filesink");
  }

  if (error != NULL && *error != NULL)
    return FALSE;

  gst_bin_add_many (GST_BIN (priv->video_save_bin), priv->audio_source, audio_queue,
                    audio_convert, audio_enc, video_save_csp, video_save_rate, video_save_scale, video_enc,
                    mux, priv->video_file_sink, NULL);

  /* add ghostpad */
  pad = gst_element_get_pad (video_save_csp, "sink");
  gst_element_add_pad (priv->video_save_bin, gst_ghost_pad_new ("sink", pad));
  gst_object_unref (GST_OBJECT (pad));


  ok = gst_element_link_many (priv->audio_source, audio_queue, audio_convert,
                              audio_enc, mux, priv->video_file_sink, NULL);

  ok &= gst_element_link_many (video_save_csp, video_save_rate, video_save_scale, video_enc,
                               NULL);
  ok &= gst_element_link (video_enc, mux);

  if (!ok)
    g_error ("Unable to create video save pipeline");

  return TRUE;
}

int
cheese_camera_get_num_camera_devices (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  return priv->num_camera_devices;
}

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

gboolean
cheese_camera_switch_camera_device (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  gboolean was_recording        = FALSE;
  gboolean pipeline_was_playing = FALSE;
  gboolean disp_bin_created     = FALSE;
  gboolean disp_bin_added       = FALSE;
  gboolean disp_bin_linked      = FALSE;
  GError  *error                = NULL;

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

  gst_bin_remove (GST_BIN (priv->pipeline), priv->video_display_bin);

  disp_bin_created = cheese_camera_create_video_display_bin (camera, &error);
  if (!disp_bin_created)
  {
    return FALSE;
  }
  disp_bin_added = gst_bin_add (GST_BIN (priv->pipeline), priv->video_display_bin);
  if (!disp_bin_added)
  {
    gst_object_sink (priv->video_display_bin);
    return FALSE;
  }

  disp_bin_linked = gst_element_link (priv->video_display_bin, priv->photo_save_bin);
  if (!disp_bin_linked)
  {
    gst_bin_remove (GST_BIN (priv->pipeline), priv->video_display_bin);
    return FALSE;
  }

  if (pipeline_was_playing)
  {
    cheese_camera_play (camera);
  }

  /* if (was_recording)
   * {
   * Restart recording... ?
   * } */

  return TRUE;
}

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
    if (G_UNLIKELY (gst_caps_is_empty (caps)))
    {
      gst_caps_unref (caps);
      caps = gst_caps_new_any ();
    }
  }

  g_object_set (priv->capsfilter, "caps", caps, NULL);
  gst_caps_unref (caps);

  gst_element_set_state (priv->pipeline, GST_STATE_PLAYING);
  priv->pipeline_is_playing = TRUE;
}

void
cheese_camera_stop (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  if (priv->pipeline != NULL)
    gst_element_set_state (priv->pipeline, GST_STATE_NULL);
  priv->pipeline_is_playing = FALSE;
}

static void
cheese_camera_change_effect_filter (CheeseCamera *camera, GstElement *new_filter)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  gboolean is_playing = priv->pipeline_is_playing;
  gboolean ok;

  cheese_camera_stop (camera);

  gst_element_unlink_many (priv->camera_source_bin, priv->effect_filter,
                           priv->csp_post_effect, NULL);

  gst_bin_remove (GST_BIN (priv->video_display_bin), priv->effect_filter);

  gst_bin_add (GST_BIN (priv->video_display_bin), new_filter);
  ok = gst_element_link_many (priv->camera_source_bin, new_filter,
                              priv->csp_post_effect, NULL);
  g_return_if_fail (ok);

  if (is_playing)
    cheese_camera_play (camera);

  priv->effect_filter = new_filter;
}

void
cheese_camera_set_effect (CheeseCamera *camera, CheeseCameraEffect effect)
{
  GString    *rgb_effects_str = g_string_new ("");
  GString    *yuv_effects_str = g_string_new ("");
  char       *effects_pipeline_desc;
  int         i;
  GstElement *effect_filter;
  GError     *err = NULL;

  for (i = 0; i < NUM_EFFECTS; i++)
  {
    if (effect & EFFECT_TO_PIPELINE_DESC[i].effect)
    {
      if (EFFECT_TO_PIPELINE_DESC[i].colorspace == RGB)
      {
        g_string_append (rgb_effects_str, EFFECT_TO_PIPELINE_DESC[i].pipeline_desc);
        g_string_append (rgb_effects_str, " ! ");
      }
      else
      {
        g_string_append (yuv_effects_str, " ! ");
        g_string_append (yuv_effects_str, EFFECT_TO_PIPELINE_DESC[i].pipeline_desc);
      }
    }
  }
  effects_pipeline_desc = g_strconcat ("ffmpegcolorspace ! ",
                                       rgb_effects_str->str,
                                       "ffmpegcolorspace",
                                       yuv_effects_str->str,
                                       NULL);

  effect_filter = gst_parse_bin_from_description (effects_pipeline_desc, TRUE, &err);
  if (!effect_filter || (err != NULL))
  {
    g_error_free (err);
    g_error ("ERROR effect_filter\n");
  }
  cheese_camera_change_effect_filter (camera, effect_filter);

  g_free (effects_pipeline_desc);
  g_string_free (rgb_effects_str, TRUE);
  g_string_free (yuv_effects_str, TRUE);
}

void
cheese_camera_start_video_recording (CheeseCamera *camera, char *filename)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  g_object_set (CHEESE_CAMERA_GET_PRIVATE (camera)->video_file_sink, "location", filename, NULL);
  cheese_camera_change_sink (camera, priv->video_display_bin,
                             priv->video_save_bin, priv->photo_save_bin);
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

    cheese_camera_change_sink (camera, priv->video_display_bin,
                               priv->photo_save_bin, priv->video_save_bin);
    priv->is_recording = FALSE;
  }

  return FALSE;
}

void
cheese_camera_stop_video_recording (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);
  GstState             state;

  gst_element_get_state (priv->pipeline, &state, NULL, 0);

  if (state == GST_STATE_PLAYING)
  {
    /* Send EOS message down the pipeline by stopping video and audio source*/
    GST_DEBUG ("Sending EOS event down the recording pipeline");
    gst_element_send_event (priv->video_source, gst_event_new_eos ());
    gst_element_send_event (priv->audio_source, gst_event_new_eos ());
    priv->eos_timeout_id = g_timeout_add (3000, cheese_camera_force_stop_video_recording, camera);
  }
  else
  {
    cheese_camera_force_stop_video_recording (camera);
  }
}

gboolean
cheese_camera_take_photo (CheeseCamera *camera, char *filename)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  if (priv->photo_handler_signal_id != 0)
  {
    GST_WARNING ("Still waiting for previous photo data, ignoring new request");
    return FALSE;
  }

  g_free (priv->photo_filename);
  priv->photo_filename = g_strdup (filename);

  /* Take the photo by connecting the handoff signal */
  priv->photo_handler_signal_id = g_signal_connect (G_OBJECT (priv->photo_sink),
                                                    "handoff",
                                                    G_CALLBACK (cheese_camera_photo_data_cb),
                                                    camera);
  return TRUE;
}

gboolean
cheese_camera_take_photo_pixbuf (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  if (priv->photo_handler_signal_id != 0)
  {
    GST_WARNING ("Still waiting for previous photo data, ignoring new request");
    return FALSE;
  }

  /* Take the photo by connecting the handoff signal */
  priv->photo_handler_signal_id = g_signal_connect (G_OBJECT (priv->photo_sink),
                                                    "handoff",
                                                    G_CALLBACK (cheese_camera_photo_data_cb),
                                                    camera);
  return TRUE;
}

static void
cheese_camera_finalize (GObject *object)
{
  CheeseCamera *camera;

  camera = CHEESE_CAMERA (object);
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  cheese_camera_stop (camera);
  if (priv->pipeline != NULL)
    gst_object_unref (priv->pipeline);

  if (priv->is_recording && priv->photo_save_bin != NULL)
    gst_object_unref (priv->photo_save_bin);
  else if (priv->video_save_bin != NULL)
    gst_object_unref (priv->video_save_bin);

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
    case PROP_VIDEO_WINDOW:
      g_value_set_pointer (value, priv->video_window);
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
    case PROP_VIDEO_WINDOW:
      priv->video_window = g_value_get_pointer (value);
      g_signal_connect (priv->video_window, "expose-event",
                        G_CALLBACK (cheese_camera_expose_cb), self);
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


  g_object_class_install_property (object_class, PROP_VIDEO_WINDOW,
                                   g_param_spec_pointer ("video-window",
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
cheese_camera_new (GtkWidget *video_window, char *camera_device_name,
                   int x_resolution, int y_resolution)
{
  CheeseCamera      *camera;
  CheeseVideoFormat *format = g_slice_new (CheeseVideoFormat);

  format->width  = x_resolution;
  format->height = y_resolution;

  if (camera_device_name)
  {
    camera = g_object_new (CHEESE_TYPE_CAMERA, "video-window", video_window,
                           "device_name", camera_device_name,
                           "format", format, NULL);
  }
  else
  {
    camera = g_object_new (CHEESE_TYPE_CAMERA, "video-window", video_window,
                           "format", format, NULL);
  }

  return camera;
}

void
cheese_camera_setup (CheeseCamera *camera, char *id, GError **error)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  gboolean ok        = TRUE;
  GError  *tmp_error = NULL;

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

  priv->pipeline = gst_pipeline_new ("pipeline");

  cheese_camera_create_video_display_bin (camera, &tmp_error);

  cheese_camera_create_photo_save_bin (camera, &tmp_error);

  cheese_camera_create_video_save_bin (camera, &tmp_error);
  if (tmp_error != NULL)
  {
    g_propagate_prefixed_error (error, tmp_error,
                                _("One or more needed GStreamer elements are missing: "));
    GST_WARNING ("%s", (*error)->message);
    return;
  }

  gst_bin_add_many (GST_BIN (priv->pipeline), priv->video_display_bin,
                    priv->photo_save_bin, NULL);

  ok = gst_element_link (priv->video_display_bin, priv->photo_save_bin);

  priv->bus = gst_element_get_bus (priv->pipeline);
  gst_bus_add_signal_watch (priv->bus);

  g_signal_connect (G_OBJECT (priv->bus), "message",
                    G_CALLBACK (cheese_camera_bus_message_cb), camera);

  gst_bus_set_sync_handler (priv->bus, (GstBusSyncHandler) cheese_camera_bus_sync_handler, camera);

  if (!ok)
    g_error ("Unable link pipeline for photo");
}

GPtrArray *
cheese_camera_get_camera_devices (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv;

  g_return_val_if_fail (CHEESE_IS_CAMERA (camera), NULL);

  priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  return g_ptr_array_ref (priv->camera_devices);
}

void
cheese_camera_set_device_by_dev_file (CheeseCamera *camera, char *file)
{
  g_return_if_fail (CHEESE_IS_CAMERA (camera));
  g_object_set (camera, "device_name", file, NULL);
}

void
cheese_camera_set_device_by_dev_udi (CheeseCamera *camera, char *udi)
{
  CheeseCameraPrivate *priv;
  int i;

  g_return_if_fail (CHEESE_IS_CAMERA (camera));

  priv = CHEESE_CAMERA_GET_PRIVATE (camera);

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

gboolean
cheese_camera_is_playing (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv;

  g_return_val_if_fail (CHEESE_IS_CAMERA (camera), FALSE);

  priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  return priv->pipeline_is_playing;
}

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

const CheeseVideoFormat *
cheese_camera_get_current_video_format (CheeseCamera *camera)
{
  CheeseCameraPrivate *priv = CHEESE_CAMERA_GET_PRIVATE (camera);
  g_return_val_if_fail (CHEESE_IS_CAMERA (camera), NULL);

  return priv->current_format;
}

gboolean
cheese_camera_get_balance_property_range (CheeseCamera *camera,
                                          gchar *property,
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

void
cheese_camera_set_balance_property (CheeseCamera *camera, gchar *property, gdouble value)
{
  CheeseCameraPrivate *priv;

  g_return_if_fail (CHEESE_IS_CAMERA (camera));

  priv = CHEESE_CAMERA_GET_PRIVATE (camera);

  g_object_set (G_OBJECT (priv->video_balance), property, value, NULL);
}
