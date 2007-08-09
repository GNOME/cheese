/*
 * Copyright (C) 2007 Copyright (C) 2007 daniel g. siegel <dgsiegel@gmail.com>
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

#include "cheese-config.h"

#include <gst/interfaces/xoverlay.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs.h>

#include "cheese.h"
#include "cheese-window.h"
#include "cheese-pipeline.h"
#include "cheese-pipeline-photo.h"
#include "cheese-pipeline-video.h"
#include "cheese-effects-widget.h"


struct _cheese_pipeline
{
  PipelinePhoto *PhotoPipeline;
  PipelineVideo *VideoPipeline;

  gboolean pipeline_is_photo;
  gchar *source_pipeline;
  GstElement *gst_test_pipeline;
};

struct _cheese_pipeline cheese_pipeline;

// private methods
static gboolean cheese_pipeline_test_build(const gchar *pipeline_desc, GError **p_err);
static gboolean cheese_pipeline_test(const gchar *pipeline_desc);
//static void cheese_pipeline_error_dlg(const gchar *pipeline_desc, const gchar *error_message);
static void cheese_pipeline_error_print(const gchar *pipeline_desc, const gchar *error_message);

void
cheese_pipeline_finalize()
{
  cheese_pipeline_set_stop();
  cheese_pipeline.pipeline_is_photo ?
    g_object_unref(G_OBJECT(cheese_pipeline.PhotoPipeline)) :
    g_object_unref(G_OBJECT(cheese_pipeline.VideoPipeline));
}

void
cheese_pipeline_init()
{
  cheese_pipeline.pipeline_is_photo = TRUE;
  cheese_pipeline.PhotoPipeline = PIPELINE_PHOTO(cheese_pipeline_photo_new());
  cheese_pipeline_create();

  cheese_pipeline_set_play();
}

void
cheese_pipeline_set_play()
{
  cheese_pipeline.pipeline_is_photo ?
    cheese_pipeline_photo_set_play(cheese_pipeline.PhotoPipeline) :
    cheese_pipeline_video_set_play(cheese_pipeline.VideoPipeline);
}

void
cheese_pipeline_set_stop()
{
  cheese_pipeline.pipeline_is_photo ?
    cheese_pipeline_photo_set_stop(cheese_pipeline.PhotoPipeline) :
    cheese_pipeline_video_set_stop(cheese_pipeline.VideoPipeline);
}

void
cheese_pipeline_button_clicked(GtkWidget *widget)
{
  cheese_pipeline.pipeline_is_photo ?
    cheese_pipeline_photo_button_clicked(widget, cheese_pipeline.PhotoPipeline) :
    cheese_pipeline_video_button_clicked(widget, cheese_pipeline.VideoPipeline);
}

GstElement *
cheese_pipeline_get_ximagesink()
{
  return cheese_pipeline.pipeline_is_photo ?
    cheese_pipeline_photo_get_ximagesink(cheese_pipeline.PhotoPipeline) :
    cheese_pipeline_video_get_ximagesink(cheese_pipeline.VideoPipeline);
}

GstElement *
cheese_pipeline_get_fakesink()
{
  return cheese_pipeline.pipeline_is_photo ?
    cheese_pipeline_photo_get_fakesink(cheese_pipeline.PhotoPipeline) :
    cheese_pipeline_video_get_fakesink(cheese_pipeline.VideoPipeline);
}

GstElement *
cheese_pipeline_get_pipeline()
{
  return cheese_pipeline.pipeline_is_photo ?
    cheese_pipeline_photo_get_pipeline(cheese_pipeline.PhotoPipeline) :
    cheese_pipeline_video_get_pipeline(cheese_pipeline.VideoPipeline);
}

void
cheese_pipeline_change_effect()
{
  gchar *effect = cheese_effects_get_selection();
  cheese_pipeline.pipeline_is_photo ?
    cheese_pipeline_photo_change_effect(effect, cheese_pipeline.PhotoPipeline) :
    cheese_pipeline_video_change_effect(effect, cheese_pipeline.VideoPipeline);
}

void
cheese_pipeline_change_pipeline_type()
{
  cheese_effects_widget_remove_all_effects();
  if (cheese_pipeline.pipeline_is_photo) {
    g_print("changing to video-mode\n");

    cheese_pipeline_set_stop();
    g_object_unref(G_OBJECT(cheese_pipeline.PhotoPipeline));

    cheese_pipeline.pipeline_is_photo = FALSE;
    cheese_pipeline.VideoPipeline = PIPELINE_VIDEO(cheese_pipeline_video_new());
    cheese_pipeline_video_create(cheese_pipeline.source_pipeline, cheese_pipeline.VideoPipeline);
    cheese_pipeline_set_play();
  } else {
    g_print("changing to photo-mode\n");

    cheese_pipeline_set_stop();
    g_object_unref(G_OBJECT(cheese_pipeline.VideoPipeline));

    cheese_pipeline.pipeline_is_photo = TRUE;
    cheese_pipeline.PhotoPipeline = PIPELINE_PHOTO(cheese_pipeline_photo_new());
    cheese_pipeline_photo_create(cheese_pipeline.source_pipeline, cheese_pipeline.PhotoPipeline);
    cheese_pipeline_set_play();
  }
}

void
cheese_pipeline_create() {

  g_message("Probing the webcam, please ignore the following, not applicabable tries");
  if (cheese_pipeline_test("v4l2src ! fakesink")) {
    cheese_pipeline.source_pipeline = "v4l2src";
  } else if (cheese_pipeline_test("v4lsrc ! video/x-raw-rgb,width=640,height=480 ! fakesink")) {
    cheese_pipeline.source_pipeline = "v4lsrc ! video/x-raw-rgb,width=640,height=480";
  } else if (cheese_pipeline_test("v4lsrc ! video/x-raw-yuv,width=640,height=480 ! fakesink")) {
    cheese_pipeline.source_pipeline = "v4lsrc ! video/x-raw-yuv,width=640,height=480";
  } else if (cheese_pipeline_test("v4lsrc ! video/x-raw-rgb,width=320,height=240 ! fakesink")) {
    cheese_pipeline.source_pipeline = "v4lsrc ! video/x-raw-rgb,width=320,height=240";
  } else if (cheese_pipeline_test("v4lsrc ! video/x-raw-rgb,width=1280,height=960 ! fakesink")) {
    cheese_pipeline.source_pipeline = "v4lsrc ! video/x-raw-rgb,width=1280,height=960";
  } else if (cheese_pipeline_test("v4lsrc ! video/x-raw-rgb,width=160,height=120 ! fakesink")) {
    cheese_pipeline.source_pipeline = "v4lsrc ! video/x-raw-rgb,width=160,height=120";
  } else if (cheese_pipeline_test("v4lsrc ! fakesink")) {
    cheese_pipeline.source_pipeline = "v4lsrc";
  } else {
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new(GTK_WINDOW(cheese_window.window),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
        _("Unable to find a webcam, SORRY!"));

    gtk_dialog_run(GTK_DIALOG (dialog));
    gtk_widget_destroy(dialog);
    cheese_pipeline.source_pipeline = "videotestsrc";
  }

  g_print("using source: %s\n", cheese_pipeline.source_pipeline);

  cheese_pipeline_photo_create(cheese_pipeline.source_pipeline, cheese_pipeline.PhotoPipeline);
}

/*
 * shamelessly Stolen from gnome-media:
 * pipeline-tests.c
 * Copyright (C) 2002 Jan Schmidt
 * Copyright (C) 2005 Tim-Philipp Müller <tim centricular net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

static gboolean
cheese_pipeline_test_build(const gchar *pipeline_desc, GError **p_err) {
  gboolean return_val = FALSE;

  g_assert (p_err != NULL);

  if (pipeline_desc) {
    cheese_pipeline.gst_test_pipeline = gst_parse_launch(pipeline_desc, p_err);

    if (*p_err == NULL && cheese_pipeline.gst_test_pipeline != NULL) {
      return_val = TRUE;
    }
  }

  return return_val;
}

static gboolean
cheese_pipeline_test(const gchar *pipeline_desc) {
  GstStateChangeReturn ret;
  GstMessage *msg;
  GError *err = NULL;
  GstBus *bus;


  /* Build the pipeline */
  if (!cheese_pipeline_test_build(pipeline_desc, &err)) {
    /* Show the error pipeline */
    //cheese_pipeline_error_dlg(pipeline_desc, (err) ? err->message : NULL);
    cheese_pipeline_error_print(pipeline_desc, (err) ? err->message : NULL);
    if (err)
      g_error_free(err);
    return FALSE;
  }

  /* Start the pipeline and wait for max. 3 seconds for it to start up */
  gst_element_set_state(cheese_pipeline.gst_test_pipeline, GST_STATE_PLAYING);
  ret = gst_element_get_state(cheese_pipeline.gst_test_pipeline, NULL, NULL, 3 * GST_SECOND);

  /* Check if any error messages were posted on the bus */
  bus = gst_element_get_bus(cheese_pipeline.gst_test_pipeline);
  msg = gst_bus_poll(bus, GST_MESSAGE_ERROR, 0);
  gst_object_unref(bus);

  if (cheese_pipeline.gst_test_pipeline) {
    gst_element_set_state (cheese_pipeline.gst_test_pipeline, GST_STATE_NULL);
    gst_object_unref (cheese_pipeline.gst_test_pipeline);
    cheese_pipeline.gst_test_pipeline = NULL;
  }

  if (msg != NULL) {
    gchar *dbg = NULL;

    gst_message_parse_error(msg, &err, &dbg);
    gst_message_unref(msg);

    g_message("Error running pipeline '%s': %s [%s]", pipeline_desc,
        (err) ? err->message : "(null error)",
        (dbg) ? dbg : "no additional debugging details");
    //cheese_pipeline_error_dlg(pipeline_desc, err->message);
    cheese_pipeline_error_print(pipeline_desc, err->message);
    g_error_free (err);
    g_free (dbg);
    return FALSE;
  } else if (ret != GST_STATE_CHANGE_SUCCESS) {
    //cheese_pipeline_error_dlg(pipeline_desc, NULL);
    cheese_pipeline_error_print(pipeline_desc, NULL);
    return FALSE;
  } else {
    //works
    return TRUE;
  }
  return FALSE;
}

/*
static void
cheese_pipeline_error_dlg(const gchar *pipeline_desc, const gchar *error_message) {
  gchar *errstr;

  if (error_message) {
    errstr = g_strdup_printf("[%s]: %s", pipeline_desc, error_message);
  } else {
    errstr = g_strdup_printf(("Failed to construct test pipeline for '%s'"),
        pipeline_desc);
  }

  GtkWidget *dialog;

  dialog = gtk_message_dialog_new(GTK_WINDOW(cheese_window.window),
      GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
      "SORRY, but Cheese has failed to set up you webcam with %s:\n\n%s",
      g_strsplit(pipeline_desc, " !", 0)[0], errstr);

  gtk_dialog_run(GTK_DIALOG (dialog));
  gtk_widget_destroy(dialog);

  g_free(errstr);
}
*/

static void
cheese_pipeline_error_print(const gchar *pipeline_desc, const gchar *error_message) {
  gchar *errstr;

  if (error_message) {
    errstr = g_strdup_printf("[%s]: %s", pipeline_desc, error_message);
  } else {
    errstr = g_strdup_printf(("Failed to construct test pipeline for '%s'"),
        pipeline_desc);
  }

  g_message("test pipeline for %s failed:\n%s",
      g_strsplit(pipeline_desc, " !", 0)[0], errstr);

  g_free(errstr);
}
