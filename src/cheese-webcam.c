/*
 * Copyright (C) 2007 Jaap Haitsma <jaap@haitsma.org>
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

#include <glib-object.h>
#include <glib.h>
#include <glib/gstring.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gst/gst.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>
#include <libhal.h>

#include "cheese-webcam.h"

G_DEFINE_TYPE (CheeseWebcam, cheese_webcam, G_TYPE_OBJECT)

#define CHEESE_WEBCAM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHEESE_TYPE_WEBCAM, CheeseWebcamPrivate))

typedef struct 
{
  int numerator;
  int denominator;
} CheeseFramerate;

typedef struct 
{
  char *mimetype;
  int width;
  int height;
  int num_framerates;
  CheeseFramerate *framerates; 
} CheeseVideoFormat;

typedef struct
{
  char *video_device; 
  char *gstreamer_src;
  char *product_name; 
  int num_video_formats;
  GArray *video_formats;
} CheeseWebcamDevice;


typedef struct
{
  GtkWidget* video_window;

  GstElement *pipeline; 
  GstBus     *bus;
  /* We build the active pipeline by linking the appropriate pipelines listed
     below*/
  GstElement *webcam_source_bin;
  GstElement *video_display_bin;
  GstElement *photo_save_bin;
  GstElement *video_save_bin;

  GstElement *video_source;
  GstElement *video_file_sink;
  GstElement *photo_sink;
  GstElement *audio_source;
  GstElement *audio_enc;
  GstElement *video_enc;

  GstElement *effect_filter, *csp_post_effect;

  gulong photo_handler_signal_id;

  gboolean is_recording;
  gboolean pipeline_is_playing;
  char *photo_filename;
    
  XF86VidModeGamma normal_gamma;
  float flash_intensity;

  int num_webcam_devices;
  CheeseWebcamDevice *webcam_devices;
} CheeseWebcamPrivate;



enum 
{
  PROP_0,
  PROP_VIDEO_WINDOW
};

enum 
{
  PHOTO_SAVED,
  VIDEO_SAVED,
  LAST_SIGNAL
};

static guint webcam_signals [LAST_SIGNAL];

typedef enum
{
  RGB,
  YUV
} VideoColorSpace;

typedef struct
{
  CheeseWebcamEffect effect;
  const char *pipeline_desc;
  VideoColorSpace colorspace; /* The color space the effect works in */
} EffectToPipelineDesc;


static const EffectToPipelineDesc EFFECT_TO_PIPELINE_DESC [] =
{
  {CHEESE_WEBCAM_EFFECT_NO_EFFECT, "identity", RGB},
  {CHEESE_WEBCAM_EFFECT_MAUVE, "videobalance saturation=1.5 hue=+0.5", YUV},
  {CHEESE_WEBCAM_EFFECT_NOIR_BLANC, "videobalance saturation=0", YUV},
  {CHEESE_WEBCAM_EFFECT_SATURATION, "videobalance saturation=2", YUV},
  {CHEESE_WEBCAM_EFFECT_HULK, "videobalance saturation=1.5 hue=-0.5", YUV},
  {CHEESE_WEBCAM_EFFECT_VERTICAL_FLIP, "videoflip method=4", YUV},
  {CHEESE_WEBCAM_EFFECT_HORIZONTAL_FLIP, "videoflip method=5", YUV},
  {CHEESE_WEBCAM_EFFECT_SHAGADELIC, "shagadelictv", RGB},
  {CHEESE_WEBCAM_EFFECT_VERTIGO, "vertigotv", RGB},
  {CHEESE_WEBCAM_EFFECT_EDGE, "edgetv", RGB},
  {CHEESE_WEBCAM_EFFECT_DICE, "dicetv", RGB},
  {CHEESE_WEBCAM_EFFECT_WARP, "warptv", RGB}
};

static const int NUM_EFFECTS = G_N_ELEMENTS (EFFECT_TO_PIPELINE_DESC);



static void
cheese_webcam_set_x_overlay (CheeseWebcam *webcam)
{
  CheeseWebcamPrivate *priv = CHEESE_WEBCAM_GET_PRIVATE (webcam);

  GstXOverlay *overlay = GST_X_OVERLAY (gst_bin_get_by_interface (GST_BIN (priv->pipeline), 
                                        GST_TYPE_X_OVERLAY));  
  gst_x_overlay_set_xwindow_id (overlay, GDK_WINDOW_XWINDOW (priv->video_window->window));
}

static void
cheese_webcam_change_sink (CheeseWebcam *webcam, GstElement *src, 
                           GstElement *new_sink, GstElement *old_sink)
{
  CheeseWebcamPrivate* priv = CHEESE_WEBCAM_GET_PRIVATE (webcam);
  gboolean is_playing = priv->pipeline_is_playing;

  cheese_webcam_stop (webcam);

  gst_element_unlink (src, old_sink);
  gst_object_ref (old_sink);
  gst_bin_remove (GST_BIN (priv->pipeline), old_sink);

  gst_bin_add (GST_BIN (priv->pipeline), new_sink);
  gst_element_link (src, new_sink);

  if (is_playing)
    cheese_webcam_play (webcam);
}


static gboolean
cheese_webcam_expose_cb (GtkWidget *widget, GdkEventExpose *event, CheeseWebcam *webcam)
{
  cheese_webcam_set_x_overlay (webcam);
  return FALSE;
}

static void
cheese_webcam_photo_data_cb (GstElement *element, GstBuffer *buffer,
                             GstPad *pad, CheeseWebcam *webcam)
{
  CheeseWebcamPrivate *priv = CHEESE_WEBCAM_GET_PRIVATE (webcam);
  GstCaps *caps;
  const GstStructure *structure;
  int width, height, stride;
  GdkPixbuf *pixbuf;
  const int bits_per_pixel = 8;

  caps = gst_buffer_get_caps (buffer);
  structure = gst_caps_get_structure (caps, 0);
  gst_structure_get_int (structure, "width", &width);
  gst_structure_get_int (structure, "height", &height);

  stride = buffer->size/height;
  pixbuf = gdk_pixbuf_new_from_data(GST_BUFFER_DATA (buffer), GDK_COLORSPACE_RGB, 
                                    FALSE, bits_per_pixel, width, height, stride, 
                                    NULL, NULL);

  gdk_pixbuf_save (pixbuf, priv->photo_filename, "jpeg", NULL, NULL);
  g_object_unref (G_OBJECT (pixbuf));

  g_signal_handler_disconnect (G_OBJECT(priv->photo_sink), 
                               priv->photo_handler_signal_id);
}

static void
cheese_webcam_bus_message_cb (GstBus *bus, GstMessage *message, CheeseWebcam *webcam)
{
  CheeseWebcamPrivate* priv = CHEESE_WEBCAM_GET_PRIVATE (webcam);

  if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_EOS)
  {
    g_signal_emit (webcam, webcam_signals [VIDEO_SAVED], 0);

    gst_element_set_locked_state (priv->video_source, FALSE);
    gst_element_set_locked_state (priv->audio_source, FALSE);
    cheese_webcam_change_sink (webcam, priv->video_display_bin, 
                               priv->photo_save_bin, priv->video_save_bin);
    priv->is_recording = FALSE;
  }
}


static void
cheese_webcam_get_video_devices_from_hal (webcam)
{
  CheeseWebcamPrivate* priv = CHEESE_WEBCAM_GET_PRIVATE (webcam);
  int i;
  int num_udis;
  char **udis;
  DBusError error;
  LibHalContext *hal_ctx;


  dbus_error_init (&error);
  hal_ctx = libhal_ctx_new ();	
  if (hal_ctx == NULL) 
  {
    g_error ("error: libhal_ctx_new");
    dbus_error_free (&error);
    return;
  }

  if (!libhal_ctx_set_dbus_connection (hal_ctx, dbus_bus_get (DBUS_BUS_SYSTEM, &error))) 
  {
    g_error ("error: libhal_ctx_set_dbus_connection: %s: %s", error.name, error.message);
    dbus_error_free (&error);
    return;
  }

  if (!libhal_ctx_init (hal_ctx, &error)) 
  {
    if (dbus_error_is_set(&error)) 
    {
      g_error ("error: libhal_ctx_init: %s: %s\n", error.name, error.message);
      dbus_error_free(&error);
    }
    g_error ("Could not initialise connection to hald.\n"
             "Normally this means the HAL daemon (hald) is not running or not ready");
    return;
  }

  udis = libhal_find_device_by_capability (hal_ctx, "video4linux", &num_udis, &error);

  if (dbus_error_is_set (&error)) 
  {
    g_error ("error: %s: %s\n", error.name, error.message);
    dbus_error_free(&error);
    return;
  }

  /* Initialize webcam structures */
  priv->num_webcam_devices = num_udis;
  priv->webcam_devices = g_new0 (CheeseWebcamDevice, priv->num_webcam_devices);
  for (i = 0; i < priv->num_webcam_devices; i++)
  {
    priv->webcam_devices[i].num_video_formats = 0;
    priv->webcam_devices[i].video_formats = g_array_new (FALSE, FALSE, sizeof (CheeseVideoFormat));
  }

  for (i = 0; i < priv->num_webcam_devices; i++) 
  {
    char *device;
    
    device = libhal_device_get_property_string (hal_ctx, udis[i], "video4linux.device", &error);
    if (dbus_error_is_set (&error)) 
    {
      g_error ("error: %s: %s\n", error.name, error.message);
      dbus_error_free(&error);
      return;
    }
    priv->webcam_devices[i].video_device = g_strdup (device);
    libhal_free_string (device);
  }
  libhal_free_string_array (udis);
}

static void
cheese_webcam_get_supported_framerates (CheeseVideoFormat *video_format, GstStructure *structure)
{
  const GValue *framerates;
  int i, j;

  framerates = gst_structure_get_value (structure, "framerate");
  if (GST_VALUE_HOLDS_FRACTION (framerates))
  {
    video_format->num_framerates = 1;
    video_format->framerates = g_new0 (CheeseFramerate, video_format->num_framerates);
    video_format->framerates[0].numerator = gst_value_get_fraction_numerator (framerates);
    video_format->framerates[0].denominator = gst_value_get_fraction_denominator (framerates);    
  }
  else if (GST_VALUE_HOLDS_LIST (framerates))
  {
    video_format->num_framerates = gst_value_list_get_size (framerates);
    video_format->framerates = g_new0 (CheeseFramerate, video_format->num_framerates);
    for (i = 0; i < video_format->num_framerates; i++)
    {
      const GValue *value;
      value = gst_value_list_get_value (framerates, i);
      video_format->framerates[i].numerator = gst_value_get_fraction_numerator (value);
      video_format->framerates[i].denominator = gst_value_get_fraction_denominator (value);
    }
  }
  else if (GST_VALUE_HOLDS_FRACTION_RANGE (framerates))
  {
    int numerator_min, denominator_min, numerator_max, denominator_max;
    const GValue *fraction_range_min;
    const GValue *fraction_range_max;

    fraction_range_min = gst_value_get_fraction_range_min (framerates);
    numerator_min = gst_value_get_fraction_numerator (fraction_range_min);
    denominator_min = gst_value_get_fraction_denominator (fraction_range_min);

    fraction_range_max = gst_value_get_fraction_range_max (framerates);
    numerator_max = gst_value_get_fraction_numerator (fraction_range_max);
    denominator_max = gst_value_get_fraction_denominator (fraction_range_max);
    g_print ("FractionRange: %d/%d - %d/%d\n", numerator_min, denominator_min, numerator_max, denominator_max);

    video_format->num_framerates = (numerator_max - numerator_min + 1) * (denominator_max - denominator_min + 1);
    video_format->framerates = g_new0 (CheeseFramerate, video_format->num_framerates);
    int k = 0;
    for (i = numerator_min; i <= numerator_max; i++)
    {
      for (j = denominator_min; j <= denominator_max; j++)
      {
        video_format->framerates[k].numerator = i;
        video_format->framerates[k].denominator = j;
        k++;
      }
    }
  }
  else
  {
      g_critical ("GValue type %s, cannot be handled for framerates", G_VALUE_TYPE_NAME (framerates));
  }    
}

static void
cheese_webcam_get_supported_video_formats (CheeseWebcamDevice *webcam_device, GstCaps *caps)
{
  int i;
  int num_structures;

  num_structures = gst_caps_get_size (caps);
  for (i = 0; i < num_structures; i++)
  {
    GstStructure *structure;
    const GValue *width, *height;
    structure = gst_caps_get_structure (caps, i);

    width = gst_structure_get_value (structure, "width");
    height = gst_structure_get_value (structure, "height");

    if (G_VALUE_HOLDS_INT (width))
    {
      CheeseVideoFormat video_format;

      video_format.mimetype = g_strdup (gst_structure_get_name (structure));
      gst_structure_get_int (structure, "width", &(video_format.width));
      gst_structure_get_int (structure, "height", &(video_format.height));
      cheese_webcam_get_supported_framerates (&video_format, structure);

      g_array_append_val (webcam_device->video_formats, video_format);
      webcam_device->num_video_formats++;
    }
    else if (GST_VALUE_HOLDS_INT_RANGE (width))
    {
      int min_width, max_width, min_height, max_height;
      int cur_width, cur_height;

      min_width = gst_value_get_int_range_min (width);
      max_width = gst_value_get_int_range_max (width);
      min_height = gst_value_get_int_range_min (height);
      max_height = gst_value_get_int_range_max (height);

      cur_width = min_width;
      cur_height = min_height;
      while (cur_width < max_width && cur_height < max_height)
      {
        CheeseVideoFormat video_format;

        video_format.mimetype = g_strdup (gst_structure_get_name (structure));
        video_format.width = cur_width;
        video_format.height = cur_height;
        cheese_webcam_get_supported_framerates (&video_format, structure);
        g_array_append_val (webcam_device->video_formats, video_format);
        webcam_device->num_video_formats++;
        cur_width *= 2;
        cur_height *= 2;
      }

      cur_width = max_width;
      cur_height = max_height;
      while (cur_width > min_width && cur_height > min_height)
      {
        CheeseVideoFormat video_format;

        video_format.mimetype = g_strdup (gst_structure_get_name (structure));
        video_format.width = cur_width;
        video_format.height = cur_height;
        cheese_webcam_get_supported_framerates (&video_format, structure);
        g_array_append_val (webcam_device->video_formats, video_format);
        webcam_device->num_video_formats++;
        cur_width /= 2;
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
cheese_webcam_get_webcam_device_data (CheeseWebcamDevice *webcam_device)
{
  char *pipeline_desc;
  GstElement *pipeline;
  GError *err;
  GstStateChangeReturn ret;
  GstMessage *msg;
  GstBus *bus;
  gboolean pipeline_works = FALSE;
  int i, j;

  static const char* GSTREAMER_VIDEO_SOURCES[] = 
  {
    "v4l2src",
    "v4lsrc"
  };

  i = 0;
  while (!pipeline_works && (i < G_N_ELEMENTS (GSTREAMER_VIDEO_SOURCES)))
  {
    pipeline_desc = g_strdup_printf ("%s name=source device=%s ! fakesink", 
                                     GSTREAMER_VIDEO_SOURCES[i],
                                     webcam_device->video_device);
    err = NULL;
    pipeline = gst_parse_launch (pipeline_desc, &err);
    if ((pipeline != NULL) && (err == NULL))
    {
      /* Start the pipeline and wait for max. 3 seconds for it to start up */
      gst_element_set_state (pipeline, GST_STATE_PLAYING);
      ret = gst_element_get_state (pipeline, NULL, NULL, 3 * GST_SECOND);

      /* Check if any error messages were posted on the bus */
      bus = gst_element_get_bus (pipeline);
      msg = gst_bus_poll (bus, GST_MESSAGE_ERROR, 0);
      gst_object_unref (bus);
 
      if ((msg == NULL) && (ret == GST_STATE_CHANGE_SUCCESS))
      {
        GstElement *src;
        GstPad* pad;
        char *name;
        GstCaps *caps;
        
        pipeline_works = TRUE;
        gst_element_set_state (pipeline, GST_STATE_PAUSED);

        webcam_device->gstreamer_src = g_strdup (GSTREAMER_VIDEO_SOURCES[i]);
        src = gst_bin_get_by_name (GST_BIN (pipeline), "source");

        g_object_get (G_OBJECT (src), "device-name", &name, NULL);
        if (name == NULL)
          name = "Unknown";

        g_print ("Detected webcam: %s\n", name);
        webcam_device->product_name = g_strdup (name);
        pad = gst_element_get_pad (src, "src");
        caps = gst_pad_get_caps (pad);
        gst_object_unref (pad);
        cheese_webcam_get_supported_video_formats (webcam_device, caps);
        gst_caps_unref (caps);
      }
      gst_element_set_state (pipeline, GST_STATE_NULL);
      gst_object_unref (pipeline);
    }
    if (err)
      g_error_free (err);

    g_free (pipeline_desc);
    i++;
  }
  for (i = 0; i < webcam_device->num_video_formats; i++)
  {
    CheeseVideoFormat video_format;
   
    video_format = g_array_index (webcam_device->video_formats, CheeseVideoFormat, i);
    g_print ("%s %d x %d num_framerates %d\n", video_format.mimetype, video_format.width, 
             video_format.height, video_format.num_framerates);
    for (j = 0; j < video_format.num_framerates; j++)
    {
      g_print ("%d/%d ", video_format.framerates[j].numerator,
               video_format.framerates[j].denominator);
    }
    g_print ("\n");
  }
}

static void
cheese_webcam_detect_webcam_devices (CheeseWebcam *webcam)
{
  CheeseWebcamPrivate* priv = CHEESE_WEBCAM_GET_PRIVATE (webcam);
  int i;

  cheese_webcam_get_video_devices_from_hal (webcam);
  for (i = 0; i < priv->num_webcam_devices; i++) 
  {
    cheese_webcam_get_webcam_device_data (&(priv->webcam_devices[i]));
  }
}

static gboolean 
cheese_webcam_create_webcam_source_bin (CheeseWebcam *webcam)
{
  CheeseWebcamPrivate* priv = CHEESE_WEBCAM_GET_PRIVATE (webcam);
  GError *err = NULL;
  char *webcam_input;
  
  if (priv->num_webcam_devices == 0)
  {
    priv->webcam_source_bin = gst_parse_bin_from_description ("videotestsrc name=video_source",
                                                              TRUE, &err);
  }
  else
  {
    CheeseVideoFormat *format; 
    int i;
    /* For now just pick the first device */
    int selected_device = 0;
    int framerate_numerator, framerate_denominator;
    CheeseWebcamDevice *selected_webcam = &(priv->webcam_devices[selected_device]);

    /* Select the highest resolution */
    format = &(g_array_index (selected_webcam->video_formats, CheeseVideoFormat, 0));
    for (i = 1; i < selected_webcam->num_video_formats; i++)
    {
      
      if (g_array_index (selected_webcam->video_formats, CheeseVideoFormat, i).width >  format->width)
      {
        format = &(g_array_index (selected_webcam->video_formats, CheeseVideoFormat, i));
      }
    }
    /* Select the highest framerate up to 30 Hz*/
    framerate_numerator = 1;
    framerate_denominator = 1;
    for (i = 0; i < format->num_framerates; i++)
    {
      float framerate = format->framerates[i].numerator / format->framerates[i].denominator;
      if (framerate > ((float)framerate_numerator / framerate_denominator)
          && framerate <= 30)
      {
        framerate_numerator = format->framerates[i].numerator;
        framerate_denominator = format->framerates[i].denominator;        
      }
    }

    webcam_input = g_strdup_printf ("%s name=video_source device=%s ! %s,width=%d,height=%d,framerate=%d/%d ! identity",
                                    selected_webcam->gstreamer_src,
                                    selected_webcam->video_device,
                                    format->mimetype,
                                    format->width,
                                    format->height,
                                    framerate_numerator,
                                    framerate_denominator);
    g_print ("%s\n", webcam_input);

    priv->webcam_source_bin = gst_parse_bin_from_description (webcam_input,
                                                              TRUE, &err);
    g_free (webcam_input);    
  }
  if (err != NULL)
  {
    g_error_free (err);
    return FALSE;
  }

  priv->video_source = gst_bin_get_by_name (GST_BIN (priv->webcam_source_bin), "video_source");
  return TRUE;
}

static gboolean
cheese_webcam_create_video_display_bin (CheeseWebcam *webcam)
{
  CheeseWebcamPrivate *priv = CHEESE_WEBCAM_GET_PRIVATE (webcam);
  GstElement *tee, *video_display_queue, *video_scale, *video_sink, *save_queue;

  gboolean ok;
  GstPad *pad;

  priv->video_display_bin = gst_bin_new ("video_display_bin");

  cheese_webcam_create_webcam_source_bin (webcam);

  priv->effect_filter = gst_element_factory_make ("identity", "effect");
  priv->csp_post_effect = gst_element_factory_make ("ffmpegcolorspace", "csp_post_effect");

  tee = gst_element_factory_make ("tee", "tee");
  
  save_queue = gst_element_factory_make ("queue", "save_queue");

  video_display_queue = gst_element_factory_make ("queue", "video_display_queue");

  video_scale = gst_element_factory_make ("videoscale", "video_scale");
  /* Use bilinear scaling */
  g_object_set (video_scale, "method", 1, NULL);

  video_sink = gst_element_factory_make ("gconfvideosink", "video_sink");


  gst_bin_add_many (GST_BIN (priv->video_display_bin), priv->webcam_source_bin, 
                    priv->effect_filter, priv->csp_post_effect, tee, save_queue, 
                    video_display_queue, video_scale, video_sink, NULL);

  ok = gst_element_link_many (priv->webcam_source_bin, priv->effect_filter, 
                              priv->csp_post_effect, tee, NULL);

  ok &= gst_element_link_many (tee, save_queue, NULL);
  ok &= gst_element_link_many (tee, video_display_queue, video_scale, video_sink, NULL);

  /* add ghostpad */
  pad = gst_element_get_pad (save_queue, "src");
  gst_element_add_pad (priv->video_display_bin, gst_ghost_pad_new ("src", pad));
  gst_object_unref (GST_OBJECT (pad));


  if (!ok)
    g_warning ("Unable to create display pipeline");

  return TRUE;
}

static gboolean
cheese_webcam_create_photo_save_bin (CheeseWebcam *webcam)
{
  CheeseWebcamPrivate *priv = CHEESE_WEBCAM_GET_PRIVATE (webcam);
  GstElement *csp_photo_save_bin;

  gboolean ok;
  GstPad *pad;
  GstCaps *caps;

  priv->photo_save_bin = gst_bin_new ("photo_save_bin");

  csp_photo_save_bin = gst_element_factory_make ("ffmpegcolorspace", "csp_photo_save_bin");
  priv->photo_sink = gst_element_factory_make ("fakesink", "photo_sink");

  gst_bin_add_many (GST_BIN (priv->photo_save_bin), csp_photo_save_bin,
                    priv->photo_sink, NULL);

  /* add ghostpad */
  pad = gst_element_get_pad (csp_photo_save_bin, "sink");
  gst_element_add_pad (priv->photo_save_bin, gst_ghost_pad_new ("sink", pad));
  gst_object_unref (GST_OBJECT (pad));

  caps = gst_caps_new_simple ("video/x-raw-rgb",
      			      "bpp", G_TYPE_INT, 24,
      			      "depth", G_TYPE_INT, 24,
      			      "red_mask",   G_TYPE_INT, 0xff0000, /* enforce rgb */
      			      "green_mask", G_TYPE_INT, 0x00ff00,
      			      "blue_mask",  G_TYPE_INT, 0x0000ff, NULL);
  ok = gst_element_link_filtered (csp_photo_save_bin, priv->photo_sink, caps);
  gst_caps_unref (caps);

  g_object_set (G_OBJECT (priv->photo_sink), "signal-handoffs", TRUE, NULL);

  if (!ok)
    g_warning ("Unable to create photo save pipeline");

  return TRUE;
}

static gboolean 
cheese_webcam_create_video_save_bin (CheeseWebcam *webcam)
{
  CheeseWebcamPrivate *priv = CHEESE_WEBCAM_GET_PRIVATE (webcam);

  GstElement *audio_queue, *audio_convert, *audio_enc;
  GstElement *video_save_csp, *video_save_scale, *video_enc;
  GstElement *mux;
  GstPad *pad;
  gboolean ok;
  GstCaps *caps;

  priv->video_save_bin = gst_bin_new ("video_save_bin");

//TODO switch to gconfaudiosrc but that one is not working for me yet
  priv->audio_source = gst_element_factory_make ("audiotestsrc", "audio_source");
//  priv->audio_source = gst_element_factory_make ("gconfaudiosrc", "audio_source");
  audio_queue = gst_element_factory_make ("queue", "audio_queue");
  audio_convert = gst_element_factory_make ("audioconvert", "audio_convert");
  audio_enc = gst_element_factory_make ("vorbisenc", "audio_enc");

  video_save_csp = gst_element_factory_make ("ffmpegcolorspace", "video_save_csp");
  video_enc = gst_element_factory_make ("theoraenc", "video_enc");
  g_object_set (video_enc, "keyframe-force", 1, NULL);
  video_save_scale = gst_element_factory_make ("videoscale", "video_save_scale");
  /* Use bilinear scaling */
  g_object_set (video_save_scale, "method", 1, NULL);

  mux = gst_element_factory_make ("oggmux", "mux");
  priv->video_file_sink = gst_element_factory_make ("filesink", "video_file_sink");

  gst_bin_add_many (GST_BIN (priv->video_save_bin), priv->audio_source, audio_queue,
		    audio_convert, audio_enc, video_save_csp, video_save_scale, video_enc, 
                    mux, priv->video_file_sink, NULL);

  /* add ghostpad */
  pad = gst_element_get_pad (video_save_csp, "sink");
  gst_element_add_pad (priv->video_save_bin, gst_ghost_pad_new ("sink", pad));
  gst_object_unref (GST_OBJECT (pad));


  ok = gst_element_link_many (priv->audio_source, audio_queue, audio_convert, 
                              audio_enc, mux, priv->video_file_sink, NULL);

  /* Record videos always in 320x240 */
  ok &= gst_element_link (video_save_csp ,video_save_scale);
  caps = gst_caps_new_simple ("video/x-raw-yuv",
                              "width", G_TYPE_INT, 320,
                              "height", G_TYPE_INT, 240,
                              NULL);
  ok &= gst_element_link_filtered (video_save_scale, video_enc, caps);
  gst_caps_unref (caps);

  ok &= gst_element_link (video_enc, mux);

  if (!ok)
    g_warning ("Unable to create video save pipeline");

  return TRUE;
}

static void
cheese_webcam_flash_set_intensity (CheeseWebcam *webcam, float intensity)
{
  const float MAX_GAMMA = 10.0;
  XF86VidModeGamma gamma;
  CheeseWebcamPrivate *priv = CHEESE_WEBCAM_GET_PRIVATE (webcam);

  g_return_if_fail ((intensity >= 0.0) || (intensity <= 1.0));

  gamma.red = MAX_GAMMA * intensity + priv->normal_gamma.red * (1.0 - intensity);
  gamma.green = MAX_GAMMA * intensity + priv->normal_gamma.green * (1.0 - intensity);
  gamma.blue = MAX_GAMMA * intensity + priv->normal_gamma.blue * (1.0 - intensity);

  XF86VidModeSetGamma (GDK_DISPLAY (), 0, &gamma);

  priv->flash_intensity = intensity;
}

static gboolean
cheese_webcam_flash_dim_cb (CheeseWebcam *webcam)
{
  CheeseWebcamPrivate *priv = CHEESE_WEBCAM_GET_PRIVATE (webcam);

  priv->flash_intensity -= 0.1;
  cheese_webcam_flash_set_intensity (webcam, priv->flash_intensity);

  if (priv->flash_intensity <= 0.0)
  {
    g_signal_emit (webcam, webcam_signals [PHOTO_SAVED], 0);
    return FALSE;
  }
  return TRUE;
}

static void
cheese_webcam_flash (CheeseWebcam *webcam)
{
  cheese_webcam_flash_set_intensity (webcam, 1.0);
  g_timeout_add (50, (GSourceFunc) cheese_webcam_flash_dim_cb, webcam);
}

int
cheese_webcam_get_num_webcam_devices (CheeseWebcam *webcam)
{
  CheeseWebcamPrivate* priv = CHEESE_WEBCAM_GET_PRIVATE (webcam);
  return priv->num_webcam_devices;
}

void
cheese_webcam_play (CheeseWebcam *webcam)
{
  CheeseWebcamPrivate* priv = CHEESE_WEBCAM_GET_PRIVATE (webcam);
  GstStateChangeReturn ret;

  ret = gst_element_set_state (priv->pipeline, GST_STATE_PLAYING);
  cheese_webcam_set_x_overlay (webcam);

  priv->pipeline_is_playing = TRUE;
}

void 
cheese_webcam_stop (CheeseWebcam *webcam)
{
  CheeseWebcamPrivate* priv = CHEESE_WEBCAM_GET_PRIVATE (webcam);
  gst_element_set_state (priv->pipeline, GST_STATE_NULL);
  priv->pipeline_is_playing = FALSE;
}

static void
cheese_webcam_change_effect_filter (CheeseWebcam *webcam, GstElement *new_filter)
{
  CheeseWebcamPrivate* priv = CHEESE_WEBCAM_GET_PRIVATE (webcam);
  gboolean is_playing = priv->pipeline_is_playing;
  gboolean ok;
  cheese_webcam_stop (webcam);

  gst_element_unlink_many (priv->webcam_source_bin, priv->effect_filter, 
                           priv->csp_post_effect, NULL);

  gst_bin_remove (GST_BIN (priv->video_display_bin), priv->effect_filter);

  gst_bin_add (GST_BIN (priv->video_display_bin), new_filter);
  ok = gst_element_link_many (priv->webcam_source_bin, new_filter,
                              priv->csp_post_effect, NULL);
  g_return_if_fail (ok);

  if (is_playing)
    cheese_webcam_play (webcam);

  priv->effect_filter = new_filter;
}

void
cheese_webcam_set_effect (CheeseWebcam *webcam, CheeseWebcamEffect effect)
{
  GString *rgb_effects_str = g_string_new ("");
  GString *yuv_effects_str = g_string_new ("");
  char *effects_pipeline_desc;
  int i;
  GstElement *effect_filter;
  GError *err = NULL;

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
    g_error ("ERROR effect_filter\n");
  }
  cheese_webcam_change_effect_filter (webcam, effect_filter);

  g_free (effects_pipeline_desc);
  g_string_free (rgb_effects_str, TRUE);
  g_string_free (yuv_effects_str, TRUE);
}

void
cheese_webcam_start_video_recording (CheeseWebcam *webcam, char *filename)
{
  CheeseWebcamPrivate* priv = CHEESE_WEBCAM_GET_PRIVATE (webcam);

  g_object_set (CHEESE_WEBCAM_GET_PRIVATE (webcam)->video_file_sink, "location", filename, NULL);
  cheese_webcam_change_sink (webcam, priv->video_display_bin, 
                             priv->video_save_bin, priv->photo_save_bin);
  priv->is_recording = TRUE;
}

void
cheese_webcam_stop_video_recording (CheeseWebcam *webcam)
{
  CheeseWebcamPrivate* priv = CHEESE_WEBCAM_GET_PRIVATE (webcam);

  /* Send EOS message down the pipeline by stopping video and audio source*/
  gst_element_set_state (priv->video_source, GST_STATE_NULL);
  gst_element_set_locked_state (priv->video_source, TRUE);
  gst_element_set_state (priv->audio_source, GST_STATE_NULL);
  gst_element_set_locked_state (priv->audio_source, TRUE);
}

void
cheese_webcam_take_photo (CheeseWebcam *webcam, char *filename)
{
  CheeseWebcamPrivate* priv = CHEESE_WEBCAM_GET_PRIVATE (webcam);
  g_free (priv->photo_filename);
  priv->photo_filename = g_strdup (filename);
  /* Take the photo by connecting the handoff signal */
  priv->photo_handler_signal_id = g_signal_connect (G_OBJECT (priv->photo_sink), 
                                                    "handoff",
                                                    G_CALLBACK (cheese_webcam_photo_data_cb), 
                                                    webcam);
  cheese_webcam_flash (webcam);
}


static void
cheese_webcam_finalize (GObject *object)
{
  CheeseWebcam *webcam;
  int i, j;

  webcam = CHEESE_WEBCAM (object);
  CheeseWebcamPrivate *priv = CHEESE_WEBCAM_GET_PRIVATE (webcam);  

  cheese_webcam_stop (webcam);
  gst_object_unref (priv->pipeline);

  if (priv->is_recording)
    gst_object_unref (priv->photo_save_bin);
  else
    gst_object_unref (priv->video_save_bin);

  g_free (priv->photo_filename);

  /* Free CheeseWebcamDevice array */
  for (i = 0; i < priv->num_webcam_devices; i++)
  {
    for (j = 0; j < priv->webcam_devices[i].num_video_formats; j++)
    {
      g_free (g_array_index (priv->webcam_devices[i].video_formats, CheeseVideoFormat, j).framerates);
      g_free (g_array_index (priv->webcam_devices[i].video_formats, CheeseVideoFormat, j).mimetype);
    }
    g_array_free (priv->webcam_devices[i].video_formats, TRUE);
  }
  g_free (priv->webcam_devices);

  G_OBJECT_CLASS (cheese_webcam_parent_class)->finalize (object);
}

static void
cheese_webcam_get_property (GObject *object, guint prop_id, GValue *value,
                            GParamSpec *pspec)
{
  CheeseWebcam *self;
  self = CHEESE_WEBCAM (object);
  CheeseWebcamPrivate *priv = CHEESE_WEBCAM_GET_PRIVATE (self);

  switch (prop_id) 
  {
    case PROP_VIDEO_WINDOW:
      g_value_set_pointer (value, priv->video_window);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
cheese_webcam_set_property (GObject *object, guint prop_id, const GValue *value,
                            GParamSpec *pspec)
{
  CheeseWebcam *self;
  self = CHEESE_WEBCAM (object);
  CheeseWebcamPrivate *priv = CHEESE_WEBCAM_GET_PRIVATE (self);

  switch (prop_id) 
  {
    case PROP_VIDEO_WINDOW:
      priv->video_window = g_value_get_pointer (value);
      g_signal_connect(priv->video_window, "expose-event", 
                       G_CALLBACK(cheese_webcam_expose_cb), self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
cheese_webcam_class_init (CheeseWebcamClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = cheese_webcam_finalize;
  object_class->get_property = cheese_webcam_get_property;
  object_class->set_property = cheese_webcam_set_property;

/* TODO: check G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION */
  webcam_signals [PHOTO_SAVED] = g_signal_new ("photo-saved", G_OBJECT_CLASS_TYPE (klass),
                                               G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                               G_STRUCT_OFFSET (CheeseWebcamClass, photo_saved),
                                               NULL, NULL,
                                               g_cclosure_marshal_VOID__VOID,
                                               G_TYPE_NONE, 0);

  webcam_signals [VIDEO_SAVED] = g_signal_new ("video-saved", G_OBJECT_CLASS_TYPE (klass),
                                               G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                               G_STRUCT_OFFSET (CheeseWebcamClass, video_saved),
                                               NULL, NULL,
                                               g_cclosure_marshal_VOID__VOID,
                                               G_TYPE_NONE, 0);


  g_object_class_install_property (object_class, PROP_VIDEO_WINDOW,
                                   g_param_spec_pointer ("video-window",
                                                          NULL,
                                                          NULL,
                                                          G_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (CheeseWebcamPrivate));
}

static void
cheese_webcam_init (CheeseWebcam *webcam)
{
  CheeseWebcamPrivate* priv = CHEESE_WEBCAM_GET_PRIVATE (webcam);
  gboolean ok;

  priv->is_recording = FALSE;
  priv->pipeline_is_playing = FALSE;
  priv->photo_filename = NULL;
  priv->webcam_devices = NULL;

  cheese_webcam_detect_webcam_devices (webcam);
  cheese_webcam_create_video_display_bin (webcam);
  cheese_webcam_create_photo_save_bin (webcam);
  cheese_webcam_create_video_save_bin (webcam);

  priv->pipeline = gst_pipeline_new ("pipeline");

  gst_bin_add_many (GST_BIN (priv->pipeline), priv->video_display_bin, 
                    priv->photo_save_bin, NULL);

  ok = gst_element_link (priv->video_display_bin, priv->photo_save_bin);

  priv->bus = gst_element_get_bus (priv->pipeline);
  gst_bus_add_signal_watch (priv->bus);

  g_signal_connect (G_OBJECT (priv->bus), "message",
                    G_CALLBACK (cheese_webcam_bus_message_cb), webcam);

  if (!ok)
    g_warning ("Unable link pipeline for photo");

  XF86VidModeGetGamma (GDK_DISPLAY (), 0, &(priv->normal_gamma));
}

CheeseWebcam*
cheese_webcam_new (GtkWidget* video_window)
{
  CheeseWebcam *webcam;
  webcam = g_object_new (CHEESE_TYPE_WEBCAM, "video-window", video_window, NULL);
  return webcam;
}

