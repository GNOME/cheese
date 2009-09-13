/*
 * Copyright © 2008 James Liggett <jrliggett@cox.net>
 * Copyright © 2008, 2009 daniel g. siegel <dgsiegel@gnome.org>
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

#include "cheese-prefs-dialog.h"

typedef struct
{
  GtkWidget *cheese_prefs_dialog;
  GtkWidget *resolution_combo_box;
  GtkWidget *webcam_combo_box;
  GtkWidget *brightness_scale;
  GtkWidget *contrast_scale;
  GtkWidget *saturation_scale;
  GtkWidget *hue_scale;
  GtkWidget *burst_repeat;
  GtkWidget *burst_delay;

  GtkWidget *parent;
  CheeseWebcam *webcam;

  CheesePrefsDialogWidgets *widgets;
} CheesePrefsDialog;

static void
cheese_prefs_dialog_create_dialog (CheesePrefsDialog *prefs_dialog)
{
  GtkBuilder *builder;
  GError     *error;

  error   = NULL;
  builder = gtk_builder_new ();
  gtk_builder_add_from_file (builder, PACKAGE_DATADIR "/cheese-prefs.ui", &error);

  if (error)
  {
    g_error ("building ui from %s failed: %s", PACKAGE_DATADIR "/cheese-prefs.ui", error->message);
    g_clear_error (&error);
  }

  prefs_dialog->cheese_prefs_dialog = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                          "cheese_prefs_dialog"));
  prefs_dialog->resolution_combo_box = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                           "resolution_combo_box"));
  prefs_dialog->webcam_combo_box = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                       "webcam_combo_box"));
  prefs_dialog->brightness_scale = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                       "brightness_scale"));
  prefs_dialog->contrast_scale = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                     "contrast_scale"));
  prefs_dialog->saturation_scale = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                       "saturation_scale"));
  prefs_dialog->hue_scale = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                "hue_scale"));
  prefs_dialog->burst_repeat = GTK_WIDGET (gtk_builder_get_object (builder, "burst_repeat"));

  prefs_dialog->burst_delay = GTK_WIDGET (gtk_builder_get_object (builder, "burst_delay"));

  gtk_window_set_transient_for (GTK_WINDOW (prefs_dialog->cheese_prefs_dialog),
                                GTK_WINDOW (prefs_dialog->parent));
}

static void
cheese_prefs_dialog_on_resolution_changed (CheesePrefsWidget *widget, gpointer user_data)
{
  CheeseWebcam      *webcam;
  CheeseVideoFormat *current_format;
  CheeseVideoFormat *new_format;

  g_object_get (widget, "webcam", &webcam, NULL);

  current_format = cheese_webcam_get_current_video_format (webcam);
  new_format     = cheese_prefs_resolution_combo_get_selected_format (CHEESE_PREFS_RESOLUTION_COMBO (widget));

  if (new_format != current_format)
    cheese_webcam_set_video_format (webcam, new_format);
}

static void
cheese_prefs_dialog_on_device_changed (CheesePrefsWidget *widget, CheesePrefsDialog *prefs_dialog)
{
  CheeseWebcam *webcam;
  char         *new_device_name;
  char         *old_device_name;

  g_object_get (widget, "webcam", &webcam, NULL);
  g_object_get (webcam, "device_name", &old_device_name, NULL);

  new_device_name = cheese_prefs_webcam_combo_get_selected_webcam (CHEESE_PREFS_WEBCAM_COMBO (widget));
  g_object_set (webcam, "device_name", new_device_name, NULL);
  g_free (new_device_name);
  if (!cheese_webcam_switch_webcam_device (webcam))
  {
    g_warning ("Couldn't change webcam device.");

    /* Revert to default device */
    g_object_set (webcam, "device_name", old_device_name, NULL);
  }
  cheese_prefs_dialog_widgets_synchronize (prefs_dialog->widgets);
  g_free (old_device_name);
}

static void
cheese_prefs_dialog_setup_widgets (CheesePrefsDialog *prefs_dialog)
{
  CheesePrefsWidget *resolution_widget;
  CheesePrefsWidget *webcam_widget;
  CheesePrefsWidget *brightness_widget;
  CheesePrefsWidget *contrast_widget;
  CheesePrefsWidget *saturation_widget;
  CheesePrefsWidget *hue_widget;
  CheesePrefsWidget *burst_delay_widget;
  CheesePrefsWidget *burst_repeat_widget;

  resolution_widget = CHEESE_PREFS_WIDGET (cheese_prefs_resolution_combo_new (prefs_dialog->resolution_combo_box,
                                                                              prefs_dialog->webcam,
                                                                              "gconf_prop_x_resolution",
                                                                              "gconf_prop_y_resolution",
                                                                              0, 0));
  g_signal_connect (G_OBJECT (resolution_widget), "changed",
                    G_CALLBACK (cheese_prefs_dialog_on_resolution_changed),
                    NULL);
  cheese_prefs_dialog_widgets_add (prefs_dialog->widgets, resolution_widget);

  webcam_widget = CHEESE_PREFS_WIDGET (cheese_prefs_webcam_combo_new (prefs_dialog->webcam_combo_box,
                                                                      prefs_dialog->webcam,
                                                                      "gconf_prop_webcam"));

  g_signal_connect (G_OBJECT (webcam_widget), "changed",
                    G_CALLBACK (cheese_prefs_dialog_on_device_changed),
                    prefs_dialog);
  cheese_prefs_dialog_widgets_add (prefs_dialog->widgets, webcam_widget);

  brightness_widget = CHEESE_PREFS_WIDGET (cheese_prefs_balance_scale_new (prefs_dialog->brightness_scale,
                                                                           prefs_dialog->webcam, "brightness",
                                                                           "gconf_prop_brightness"));

  cheese_prefs_dialog_widgets_add (prefs_dialog->widgets, brightness_widget);

  contrast_widget = CHEESE_PREFS_WIDGET (cheese_prefs_balance_scale_new (prefs_dialog->contrast_scale,
                                                                         prefs_dialog->webcam, "contrast",
                                                                         "gconf_prop_contrast"));

  cheese_prefs_dialog_widgets_add (prefs_dialog->widgets, contrast_widget);

  saturation_widget = CHEESE_PREFS_WIDGET (cheese_prefs_balance_scale_new (prefs_dialog->saturation_scale,
                                                                           prefs_dialog->webcam, "saturation",
                                                                           "gconf_prop_saturation"));

  cheese_prefs_dialog_widgets_add (prefs_dialog->widgets, saturation_widget);

  hue_widget = CHEESE_PREFS_WIDGET (cheese_prefs_balance_scale_new (prefs_dialog->hue_scale,
                                                                    prefs_dialog->webcam, "hue",
                                                                    "gconf_prop_hue"));

  cheese_prefs_dialog_widgets_add (prefs_dialog->widgets, hue_widget);

  burst_repeat_widget = CHEESE_PREFS_WIDGET (cheese_prefs_burst_spinbox_new (prefs_dialog->burst_repeat,
                                                                             "gconf_prop_burst_repeat"));

  cheese_prefs_dialog_widgets_add (prefs_dialog->widgets, burst_repeat_widget);

  burst_delay_widget = CHEESE_PREFS_WIDGET (cheese_prefs_burst_spinbox_new (prefs_dialog->burst_delay,
                                                                            "gconf_prop_burst_delay"));

  cheese_prefs_dialog_widgets_add (prefs_dialog->widgets, burst_delay_widget);


  cheese_prefs_dialog_widgets_synchronize (prefs_dialog->widgets);
}

static void
cheese_prefs_dialog_destroy_dialog (CheesePrefsDialog *prefs_dialog)
{
  gtk_widget_destroy (prefs_dialog->cheese_prefs_dialog);

  g_object_unref (prefs_dialog->widgets);

  g_free (prefs_dialog);
}

void
cheese_prefs_dialog_run (GtkWidget *parent, CheeseGConf *gconf, CheeseWebcam *webcam)
{
  CheesePrefsDialog *prefs_dialog;

  prefs_dialog = g_new0 (CheesePrefsDialog, 1);

  prefs_dialog->parent  = parent;
  prefs_dialog->webcam  = webcam;
  prefs_dialog->widgets = cheese_prefs_dialog_widgets_new (gconf);

  cheese_prefs_dialog_create_dialog (prefs_dialog);
  cheese_prefs_dialog_setup_widgets (prefs_dialog);

  gtk_dialog_run (GTK_DIALOG (prefs_dialog->cheese_prefs_dialog));

  cheese_prefs_dialog_destroy_dialog (prefs_dialog);
}
