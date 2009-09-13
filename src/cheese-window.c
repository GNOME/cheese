/*
 * Copyright © 2007,2008 daniel g. siegel <dgsiegel@gnome.org>
 * Copyright © 2007,2008 Jaap Haitsma <jaap@haitsma.org>
 * Copyright © 2008 Patryk Zawadzki <patrys@pld-linux.org>
 * Copyright © 2008 Ryan Zeigler <zeiglerr@gmail.com>
 * Copyright © 2008 Filippo Argiolas <filippo.argiolas@gmail.com>
 * Copyright © 2008 Felix Kaser <f.kaser@gmx.net>
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
  #include "cheese-config.h"
#endif

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include <gst/interfaces/xoverlay.h>
#include <gtk/gtk.h>
#include <libebook/e-book.h>

#ifdef HAVE_MMKEYS
  #include <X11/XF86keysym.h>
#endif /* HAVE_MMKEYS */

#include "cheese-countdown.h"
#include "cheese-effect-chooser.h"
#include "cheese-fileutil.h"
#include "cheese-gconf.h"
#include "cheese-thumb-view.h"
#include "eog-thumb-nav.h"
#include "cheese-window.h"
#include "ephy-spinner.h"
#include "gst-audio-play.h"
#include "cheese-no-camera.h"
#include "cheese-prefs-dialog.h"
#include "cheese-flash.h"

#define SHUTTER_SOUNDS             5
#define FULLSCREEN_POPUP_HEIGHT    40
#define FULLSCREEN_TIMEOUT         5 * 1000
#define FULLSCREEN_EFFECTS_TIMEOUT 15
#define DEFAULT_WINDOW_WIDTH       600
#define DEFAULT_WINDOW_HEIGHT      450

typedef enum
{
  WEBCAM_MODE_PHOTO,
  WEBCAM_MODE_VIDEO,
  WEBCAM_MODE_BURST
} WebcamMode;

typedef enum
{
  CHEESE_RESPONSE_SKIP,
  CHEESE_RESPONSE_SKIP_ALL,
  CHEESE_RESPONSE_DELETE_ALL
} CheeseDeleteResponseType;

#define CHEESE_BUTTON_SKIP       _("_Skip")
#define CHEESE_BUTTON_SKIP_ALL   _("S_kip All")
#define CHEESE_BUTTON_DELETE_ALL _("Delete _All")

typedef struct
{
  gboolean recording;

  gboolean isFullscreen;

  /* UDI device requested on the command line */
  char *startup_hal_dev_udi;
  char *video_filename;

  int counter;

  CheeseWebcam *webcam;
  WebcamMode webcam_mode;
  CheeseGConf *gconf;
  CheeseFileUtil *fileutil;

  CheeseDbus *server;

  GtkWidget *window;
  GtkWidget *fullscreen_popup;

  GtkWidget *notebook;
  GtkWidget *notebook_bar;
  GtkWidget *fullscreen_bar;

  GtkWidget *main_vbox;
  GtkWidget *video_vbox;

  GtkWidget *netbook_alignment;
  GtkWidget *toolbar_alignment;
  GtkWidget *effect_button_alignment;
  GtkWidget *togglegroup_alignment;

  GtkWidget *effect_frame;
  GtkWidget *effect_alignment;
  GtkWidget *effect_chooser;
  GtkWidget *throbber_frame;
  GtkWidget *throbber;
  GtkWidget *countdown_frame;
  GtkWidget *countdown_frame_fullscreen;
  GtkWidget *countdown;
  GtkWidget *countdown_fullscreen;
  GtkWidget *info_bar_frame;
  GtkWidget *info_bar;

  GtkWidget *button_effects;
  GtkWidget *button_photo;
  GtkWidget *button_video;
  GtkWidget *button_burst;
  GtkWidget *button_effects_fullscreen;
  GtkWidget *button_photo_fullscreen;
  GtkWidget *button_video_fullscreen;
  GtkWidget *button_burst_fullscreen;
  GtkWidget *button_exit_fullscreen;

  GtkWidget *image_take_photo;
  GtkWidget *image_take_photo_fullscreen;
  GtkWidget *label_effects;
  GtkWidget *label_photo;
  GtkWidget *label_photo_fullscreen;
  GtkWidget *label_take_photo;
  GtkWidget *label_take_photo_fullscreen;
  GtkWidget *label_video;
  GtkWidget *label_video_fullscreen;

  GtkWidget *thumb_scrollwindow;
  GtkWidget *thumb_nav;
  GtkWidget *thumb_view;
  GtkWidget *thumb_view_popup_menu;

  GtkWidget *screen;
  GtkWidget *take_picture;
  GtkWidget *take_picture_fullscreen;

  GtkActionGroup *actions_account_photo;
  GtkActionGroup *actions_countdown;
  GtkActionGroup *actions_effects;
  GtkActionGroup *actions_preferences;
  GtkActionGroup *actions_file;
  GtkActionGroup *actions_sendto;
  GtkActionGroup *actions_flickr;
  GtkActionGroup *actions_fspot;
  GtkActionGroup *actions_mail;
  GtkActionGroup *actions_main;
  GtkActionGroup *actions_photo;
  GtkActionGroup *actions_toggle;
  GtkActionGroup *actions_video;
  GtkActionGroup *actions_burst;
  GtkActionGroup *actions_fullscreen;
  GtkActionGroup *actions_wide_mode;

  GtkUIManager *ui_manager;

  GSource *fullscreen_timeout_source;

  int audio_play_counter;

  gint repeat_count;
  gboolean is_bursting;

  CheeseFlash *flash;
} CheeseWindow;

static void cheese_window_action_button_clicked_cb (GtkWidget *widget, CheeseWindow *cheese_window);

void
cheese_window_bring_to_front (gpointer data)
{
  CheeseWindow *cheese_window     = data;
  guint32       startup_timestamp = gdk_x11_get_server_time (gtk_widget_get_window (GTK_WIDGET (cheese_window->window)));

  gdk_x11_window_set_user_time (gtk_widget_get_window (GTK_WIDGET (cheese_window->window)), startup_timestamp);

  gtk_window_present (GTK_WINDOW (cheese_window->window));
}

static char *
audio_play_get_filename (CheeseWindow *cheese_window)
{
  char *filename;

  if (cheese_window->audio_play_counter > 21)
    filename = g_strdup_printf ("%s/sounds/shutter%i.ogg", PACKAGE_DATADIR,
                                g_random_int_range (0, SHUTTER_SOUNDS));
  else
    filename = g_strdup_printf ("%s/sounds/shutter0.ogg", PACKAGE_DATADIR);

  cheese_window->audio_play_counter++;

  return filename;
}

/* standard event handler */
static int
cheese_window_delete_event_cb (GtkWidget *widget, GdkEvent event, gpointer data)
{
  return FALSE;
}

static gboolean
cheese_window_key_press_event_cb (GtkWidget *win, GdkEventKey *event, CheeseWindow *cheese_window)
{
#ifndef HAVE_MMKEYS
  return FALSE;
#else
  /* If we have modifiers, and either Ctrl, Mod1 (Alt), or any
   * of Mod3 to Mod5 (Mod2 is num-lock...) are pressed, we
   * let Gtk+ handle the key */
  if (event->state != 0
      && ((event->state & GDK_CONTROL_MASK)
          || (event->state & GDK_MOD1_MASK)
          || (event->state & GDK_MOD3_MASK)
          || (event->state & GDK_MOD4_MASK)
          || (event->state & GDK_MOD5_MASK)))
    return FALSE;

  switch (event->keyval)
  {
    case XF86XK_WebCam:

      /* do stuff */
      cheese_window_action_button_clicked_cb (NULL, cheese_window);
      return TRUE;
  }

  return FALSE;
#endif /* !HAVE_MMKEYS */
}

static void
cheese_window_fullscreen_clear_timeout (CheeseWindow *cheese_window)
{
  if (cheese_window->fullscreen_timeout_source != NULL)
  {
    g_source_unref (cheese_window->fullscreen_timeout_source);
    g_source_destroy (cheese_window->fullscreen_timeout_source);
  }

  cheese_window->fullscreen_timeout_source = NULL;
}

static gboolean
cheese_window_fullscreen_timeout_cb (gpointer data)
{
  CheeseWindow *cheese_window = data;

  gtk_widget_hide_all (cheese_window->fullscreen_popup);

  cheese_window_fullscreen_clear_timeout (cheese_window);

  return FALSE;
}

static void
cheese_window_fullscreen_set_timeout (CheeseWindow *cheese_window)
{
  GSource *source;

  cheese_window_fullscreen_clear_timeout (cheese_window);

  source = g_timeout_source_new (FULLSCREEN_TIMEOUT);

  g_source_set_callback (source, cheese_window_fullscreen_timeout_cb, cheese_window, NULL);
  g_source_attach (source, NULL);

  cheese_window->fullscreen_timeout_source = source;
}

static void
cheese_window_fullscreen_show_bar (CheeseWindow *cheese_window)
{
  gtk_widget_show_all (cheese_window->fullscreen_popup);

  /* show me the notebook with the buttons if the countdown was not triggered */
  if (cheese_countdown_get_state (CHEESE_COUNTDOWN (cheese_window->countdown_fullscreen)) == 0)
    gtk_notebook_set_current_page (GTK_NOTEBOOK (cheese_window->fullscreen_bar), 0);
}

static gboolean
cheese_window_fullscreen_motion_notify_cb (GtkWidget      *widget,
                                           GdkEventMotion *event,
                                           CheeseWindow   *cheese_window)
{
  if (cheese_window->isFullscreen)
  {
    int height;
    int width;

    gtk_window_get_size (GTK_WINDOW (cheese_window->window), &width, &height);
    if (event->y > height - 5)
    {
      cheese_window_fullscreen_show_bar (cheese_window);
    }

    /* don't set the timeout in effect-chooser mode */
    if (gtk_notebook_get_current_page (GTK_NOTEBOOK (cheese_window->notebook)) != 1)
      cheese_window_fullscreen_set_timeout (cheese_window);
  }
  return FALSE;
}

static void
cheese_window_toggle_wide_mode (GtkWidget *widget, CheeseWindow *cheese_window)
{
  gboolean toggled = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (widget));

  gtk_widget_set_size_request (cheese_window->notebook,
                               GTK_WIDGET (cheese_window->notebook)->allocation.width,
                               GTK_WIDGET (cheese_window->notebook)->allocation.height);

  /* set a single column in wide mode */
  gtk_icon_view_set_columns (GTK_ICON_VIEW (cheese_window->thumb_view), toggled ? 1 : G_MAXINT);

  /* switch thumb_nav mode */
  eog_thumb_nav_set_vertical (EOG_THUMB_NAV (cheese_window->thumb_nav), toggled);

  /* reparent thumb_view */
  g_object_ref (cheese_window->thumb_scrollwindow);
  if (toggled)
  {
    gtk_container_remove (GTK_CONTAINER (cheese_window->video_vbox), cheese_window->thumb_scrollwindow);
    gtk_container_add (GTK_CONTAINER (cheese_window->netbook_alignment), cheese_window->thumb_scrollwindow);
    g_object_unref (cheese_window->thumb_scrollwindow);
  }
  else
  {
    gtk_container_remove (GTK_CONTAINER (cheese_window->netbook_alignment), cheese_window->thumb_scrollwindow);
    gtk_box_pack_end (GTK_BOX (cheese_window->video_vbox), cheese_window->thumb_scrollwindow, FALSE, FALSE, 0);
    g_object_unref (cheese_window->thumb_scrollwindow);
    eog_thumb_nav_set_policy (EOG_THUMB_NAV (cheese_window->thumb_nav),
                              GTK_POLICY_AUTOMATIC,
                              GTK_POLICY_NEVER);
  }

  /* update spacing */

  /* NOTE: be really carefull when changing the ui file to update spacing
   * values here too! */
  if (toggled)
  {
    g_object_set (G_OBJECT (cheese_window->toolbar_alignment),
                  "bottom-padding", 10, NULL);
    g_object_set (G_OBJECT (cheese_window->togglegroup_alignment),
                  "left-padding", 6, NULL);
    g_object_set (G_OBJECT (cheese_window->effect_button_alignment),
                  "right-padding", 0, NULL);
    g_object_set (G_OBJECT (cheese_window->netbook_alignment),
                  "left-padding", 6, NULL);
  }
  else
  {
    g_object_set (G_OBJECT (cheese_window->toolbar_alignment),
                  "bottom-padding", 6, NULL);
    g_object_set (G_OBJECT (cheese_window->togglegroup_alignment),
                  "left-padding", 24, NULL);
    g_object_set (G_OBJECT (cheese_window->effect_button_alignment),
                  "right-padding", 24, NULL);
    g_object_set (G_OBJECT (cheese_window->netbook_alignment),
                  "left-padding", 0, NULL);
  }

  gtk_container_resize_children (GTK_CONTAINER (cheese_window->thumb_scrollwindow));

  GtkRequisition req;
  gtk_widget_size_request (cheese_window->window, &req);
  gtk_window_resize (GTK_WINDOW (cheese_window->window), req.width, req.height);
  gtk_widget_set_size_request (cheese_window->notebook, -1, -1);

  g_object_set (cheese_window->gconf, "gconf_prop_wide_mode", toggled, NULL);
}

static void
cheese_window_toggle_fullscreen (GtkWidget *widget, CheeseWindow *cheese_window)
{
  GdkColor   bg_color = {0, 0, 0, 0};
  GtkWidget *menubar;

  menubar = gtk_ui_manager_get_widget (cheese_window->ui_manager, "/MainMenu");

  if (!cheese_window->isFullscreen)
  {
    gtk_widget_hide (cheese_window->thumb_view);
    gtk_widget_hide (cheese_window->thumb_scrollwindow);
    gtk_widget_hide (menubar);
    gtk_widget_hide (cheese_window->notebook_bar);
    gtk_widget_modify_bg (cheese_window->window, GTK_STATE_NORMAL, &bg_color);

    gtk_widget_add_events (cheese_window->window, GDK_POINTER_MOTION_MASK);
    gtk_widget_add_events (cheese_window->screen, GDK_POINTER_MOTION_MASK);

    g_signal_connect (cheese_window->window, "motion-notify-event",
                      G_CALLBACK (cheese_window_fullscreen_motion_notify_cb),
                      cheese_window);
    g_signal_connect (cheese_window->screen, "motion-notify-event",
                      G_CALLBACK (cheese_window_fullscreen_motion_notify_cb),
                      cheese_window);

    gtk_window_fullscreen (GTK_WINDOW (cheese_window->window));

    gtk_widget_set_size_request (cheese_window->effect_alignment, -1, FULLSCREEN_POPUP_HEIGHT);
    cheese_window_fullscreen_show_bar (cheese_window);

    /* don't set the timeout in effect-chooser mode */
    if (gtk_notebook_get_current_page (GTK_NOTEBOOK (cheese_window->notebook)) != 1)
      cheese_window_fullscreen_set_timeout (cheese_window);

    cheese_window->isFullscreen = TRUE;
  }
  else
  {
    gtk_widget_show_all (cheese_window->window);
    gtk_widget_hide_all (cheese_window->fullscreen_popup);
    gtk_widget_modify_bg (cheese_window->window, GTK_STATE_NORMAL, NULL);

    g_signal_handlers_disconnect_by_func (cheese_window->window,
                                          (gpointer) cheese_window_fullscreen_motion_notify_cb, cheese_window);
    g_signal_handlers_disconnect_by_func (cheese_window->screen,
                                          (gpointer) cheese_window_fullscreen_motion_notify_cb, cheese_window);

    gtk_window_unfullscreen (GTK_WINDOW (cheese_window->window));
    gtk_widget_set_size_request (cheese_window->effect_alignment, -1, -1);
    cheese_window->isFullscreen = FALSE;

    cheese_window_fullscreen_clear_timeout (cheese_window);
  }
}

static void
cheese_window_exit_fullscreen_button_clicked_cb (GtkWidget *button, CheeseWindow *cheese_window)
{
  GtkAction *action = gtk_ui_manager_get_action (cheese_window->ui_manager, "/MainMenu/Cheese/Fullscreen");

  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), FALSE);
}

static gboolean
cheese_window_fullscreen_leave_notify_cb (GtkWidget        *widget,
                                          GdkEventCrossing *event,
                                          CheeseWindow     *cheese_window)
{
  cheese_window_fullscreen_clear_timeout (cheese_window);
  return FALSE;
}

static void
cheese_window_photo_saved_cb (CheeseWebcam *webcam, CheeseWindow *cheese_window)
{
  gdk_threads_enter ();
  if (!cheese_window->is_bursting)
  {
    gtk_action_group_set_sensitive (cheese_window->actions_effects, TRUE);
    gtk_action_group_set_sensitive (cheese_window->actions_toggle, TRUE);
    gtk_widget_set_sensitive (cheese_window->take_picture, TRUE);
    gtk_widget_set_sensitive (cheese_window->take_picture_fullscreen, TRUE);
  }
  gdk_flush ();
  gdk_threads_leave ();
}

static void
cheese_window_video_saved_cb (CheeseWebcam *webcam, CheeseWindow *cheese_window)
{
  gdk_threads_enter ();

  /* TODO look at this g_free */
  g_free (cheese_window->video_filename);
  cheese_window->video_filename = NULL;
  gtk_action_group_set_sensitive (cheese_window->actions_effects, TRUE);
  gtk_widget_set_sensitive (cheese_window->take_picture, TRUE);
  gtk_widget_set_sensitive (cheese_window->take_picture_fullscreen, TRUE);
  gdk_flush ();
  gdk_threads_leave ();
}

static void
cheese_window_cmd_close (GtkWidget *widget, CheeseWindow *cheese_window)
{
  g_object_unref (cheese_window->webcam);
  g_object_unref (cheese_window->actions_main);
  g_object_unref (cheese_window->actions_account_photo);
  g_object_unref (cheese_window->actions_countdown);
  g_object_unref (cheese_window->actions_effects);
  g_object_unref (cheese_window->actions_file);
  g_object_unref (cheese_window->actions_sendto);
  g_object_unref (cheese_window->actions_flickr);
  g_object_unref (cheese_window->actions_fspot);
  g_object_unref (cheese_window->actions_mail);
  g_object_unref (cheese_window->actions_photo);
  g_object_unref (cheese_window->actions_toggle);
  g_object_unref (cheese_window->actions_effects);
  g_object_unref (cheese_window->actions_preferences);
  g_object_unref (cheese_window->actions_file);
  g_object_unref (cheese_window->actions_video);
  g_object_unref (cheese_window->actions_burst);
  g_object_unref (cheese_window->actions_fullscreen);
  g_object_unref (cheese_window->gconf);

  g_free (cheese_window);
  gtk_main_quit ();
}

static void
cheese_window_cmd_open (GtkWidget *widget, CheeseWindow *cheese_window)
{
  char      *uri;
  char      *filename;
  GError    *error = NULL;
  GtkWidget *dialog;
  GdkScreen *screen;

  filename = cheese_thumb_view_get_selected_image (CHEESE_THUMB_VIEW (cheese_window->thumb_view));
  g_return_if_fail (filename);
  uri = g_filename_to_uri (filename, NULL, NULL);
  g_free (filename);

  screen = gtk_widget_get_screen (GTK_WIDGET (cheese_window->window));
  gtk_show_uri (screen, uri, gtk_get_current_event_time (), &error);

  if (error != NULL)
  {
    dialog = gtk_message_dialog_new (GTK_WINDOW (cheese_window->window),
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                     _("Failed to launch program to show:\n"
                                       "%s\n"
                                       "%s"), uri, error->message);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    g_error_free (error);
  }
  g_free (uri);
}

static void
cheese_window_cmd_save_as (GtkWidget *widget, CheeseWindow *cheese_window)
{
  GtkWidget *dialog;
  int        response;
  char      *filename;
  char      *basename;

  filename = cheese_thumb_view_get_selected_image (CHEESE_THUMB_VIEW (cheese_window->thumb_view));
  g_return_if_fail (filename);

  dialog = gtk_file_chooser_dialog_new (_("Save File"),
                                        GTK_WINDOW (cheese_window->window),
                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                        NULL);

  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);

  basename = g_path_get_basename (filename);
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), basename);
  g_free (basename);

  response = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_hide (dialog);

  if (response == GTK_RESPONSE_ACCEPT)
  {
    char    *target_filename;
    GError  *error = NULL;
    gboolean ok;

    target_filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    GFile *target = g_file_new_for_path (target_filename);

    GFile *source = g_file_new_for_path (filename);

    ok = g_file_copy (source, target, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &error);

    g_object_unref (source);
    g_object_unref (target);

    if (!ok)
    {
      char      *header;
      GtkWidget *dlg;

      g_error_free (error);
      header = g_strdup_printf (_("Could not save %s"), target_filename);

      dlg = gtk_message_dialog_new (GTK_WINDOW (cheese_window->window),
                                    GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                    "%s", header);
      gtk_dialog_run (GTK_DIALOG (dlg));
      gtk_widget_destroy (dlg);
      g_free (header);
    }
    g_free (target_filename);
  }
  g_free (filename);
  gtk_widget_destroy (dialog);
}

static void
cheese_window_delete_error_dialog (CheeseWindow *cheese_window, GFile *file, gchar *message)
{
  gchar     *primary, *secondary;
  GtkWidget *error_dialog;

  primary   = g_strdup (_("Error while deleting"));
  secondary = g_strdup_printf (_("The file \"%s\" cannot be deleted. Details: %s"),
                               g_file_get_basename (file), message);
  error_dialog = gtk_message_dialog_new (GTK_WINDOW (cheese_window->window),
                                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", primary);
  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (error_dialog),
                                            "%s", secondary);
  gtk_dialog_run (GTK_DIALOG (error_dialog));
  gtk_widget_destroy (error_dialog);
  g_free (primary);
  g_free (secondary);
}

static void
cheese_window_cmd_delete_file (CheeseWindow *cheese_window, GList *files, gboolean batch)
{
  GList     *l           = NULL;
  GError    *error       = NULL;
  gint       list_length = g_list_length (files);
  GtkWidget *question_dialog;
  gint       response;
  gchar     *primary, *secondary;

  if (batch == FALSE)
  {
    if (list_length > 1)
    {
      primary = g_strdup_printf (ngettext ("Are you sure you want to permanently delete the %'d selected item?",
                                           "Are you sure you want to permanently delete the %'d selected items?",
                                           list_length),
                                 list_length);
    }
    else
    {
      primary = g_strdup_printf (_("Are you sure you want to permanently delete \"%s\"?"),
                                 g_file_get_basename (files->data));
    }
    secondary       = g_strdup_printf (_("If you delete an item, it will be permanently lost."));
    question_dialog = gtk_message_dialog_new (GTK_WINDOW (cheese_window->window),
                                              GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, "%s", primary);
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (question_dialog), "%s", secondary);
    gtk_dialog_add_button (GTK_DIALOG (question_dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
    gtk_dialog_add_button (GTK_DIALOG (question_dialog), GTK_STOCK_DELETE, GTK_RESPONSE_ACCEPT);
    response = gtk_dialog_run (GTK_DIALOG (question_dialog));
    gtk_widget_destroy (question_dialog);
    g_free (primary);
    g_free (secondary);
    if (response != GTK_RESPONSE_ACCEPT)
      return;
  }

  for (l = files; l != NULL; l = l->next)
  {
    g_print ("deleting %s\n", g_file_get_basename (l->data));
    if (!g_file_delete (l->data, NULL, &error))
    {
      cheese_window_delete_error_dialog (cheese_window, l->data,
                                         error != NULL ? error->message : _("Unknown Error"));
      g_error_free (error);
      error = NULL;
    }
    g_object_unref (l->data);
  }
}

static void
cheese_window_cmd_move_file_to_trash (CheeseWindow *cheese_window, GList *files)
{
  GError    *error = NULL;
  GList     *l     = NULL;
  GList     *d     = NULL;
  gchar     *primary, *secondary;
  GtkWidget *question_dialog;
  gint       response;
  gint       list_length = g_list_length (files);

  g_print ("received %d items to delete\n", list_length);

  for (l = files; l != NULL; l = l->next)
  {
    if (!g_file_test (g_file_get_path (l->data), G_FILE_TEST_EXISTS))
    {
      g_object_unref (l->data);
      break;
    }
    if (!g_file_trash (l->data, NULL, &error))
    {
      primary   = g_strdup (_("Cannot move file to trash, do you want to delete immediately?"));
      secondary = g_strdup_printf (_("The file \"%s\" cannot be moved to the trash. Details: %s"),
                                   g_file_get_basename (l->data), error->message);
      question_dialog = gtk_message_dialog_new (GTK_WINDOW (cheese_window->window),
                                                GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, "%s", primary);
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (question_dialog), "%s", secondary);
      gtk_dialog_add_button (GTK_DIALOG (question_dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
      if (list_length > 1)
      {
        /* no need for all those buttons we have a single file to delete */
        gtk_dialog_add_button (GTK_DIALOG (question_dialog), CHEESE_BUTTON_SKIP, CHEESE_RESPONSE_SKIP);
        gtk_dialog_add_button (GTK_DIALOG (question_dialog), CHEESE_BUTTON_SKIP_ALL, CHEESE_RESPONSE_SKIP_ALL);
        gtk_dialog_add_button (GTK_DIALOG (question_dialog), CHEESE_BUTTON_DELETE_ALL, CHEESE_RESPONSE_DELETE_ALL);
      }
      gtk_dialog_add_button (GTK_DIALOG (question_dialog), GTK_STOCK_DELETE, GTK_RESPONSE_ACCEPT);
      response = gtk_dialog_run (GTK_DIALOG (question_dialog));
      gtk_widget_destroy (question_dialog);
      g_free (primary);
      g_free (secondary);
      g_error_free (error);
      error = NULL;
      switch (response)
      {
        case CHEESE_RESPONSE_DELETE_ALL:

          /* forward the list to cmd_delete */
          cheese_window_cmd_delete_file (cheese_window, l, TRUE);
          return;

        case GTK_RESPONSE_ACCEPT:

          /* create a single file list for cmd_delete */
          d = g_list_append (d, g_object_ref (l->data));
          cheese_window_cmd_delete_file (cheese_window, d, TRUE);
          g_list_free (d);
          break;

        case CHEESE_RESPONSE_SKIP:

          /* do nothing, skip to the next item */
          break;

        case CHEESE_RESPONSE_SKIP_ALL:
        case GTK_RESPONSE_CANCEL:
        case GTK_RESPONSE_DELETE_EVENT:
        default:

          /* cancel the whole delete operation */
          return;
      }
    }
    else
    {
      cheese_thumb_view_remove_item (CHEESE_THUMB_VIEW (cheese_window->thumb_view), l->data);
    }
    g_object_unref (l->data);
  }
}

static void
cheese_window_move_all_media_to_trash (GtkWidget *widget, CheeseWindow *cheese_window)
{
  GtkWidget  *dlg;
  char       *prompt;
  int         response;
  char       *filename;
  GFile      *file;
  GList      *files_list = NULL;
  GDir       *dir_videos, *dir_photos;
  char       *path_videos, *path_photos;
  const char *name;

  prompt = g_strdup_printf (_("Really move all photos and videos to the trash?"));
  dlg    = gtk_message_dialog_new_with_markup (GTK_WINDOW (cheese_window->window),
                                               GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE,
                                               "<span weight=\"bold\" size=\"larger\">%s</span>",
                                               prompt);
  g_free (prompt);
  gtk_dialog_add_button (GTK_DIALOG (dlg), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
  gtk_dialog_add_button (GTK_DIALOG (dlg), _("_Move to Trash"), GTK_RESPONSE_OK);
  gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
  gtk_window_set_title (GTK_WINDOW (dlg), "");
  gtk_widget_show_all (dlg);

  response = gtk_dialog_run (GTK_DIALOG (dlg));
  gtk_widget_destroy (dlg);

  if (response != GTK_RESPONSE_OK)
    return;

  /* append all videos */
  path_videos = cheese_fileutil_get_video_path (cheese_window->fileutil);
  dir_videos  = g_dir_open (path_videos, 0, NULL);
  while ((name = g_dir_read_name (dir_videos)) != NULL)
  {
    if (g_str_has_suffix (name, VIDEO_NAME_SUFFIX))
    {
      filename = g_strjoin (G_DIR_SEPARATOR_S, path_videos, name, NULL);
      file     = g_file_new_for_path (filename);

      files_list = g_list_append (files_list, file);
      g_free (filename);
    }
  }
  g_dir_close (dir_videos);

  /* append all photos */
  path_photos = cheese_fileutil_get_photo_path (cheese_window->fileutil);
  dir_photos  = g_dir_open (path_photos, 0, NULL);
  while ((name = g_dir_read_name (dir_photos)) != NULL)
  {
    if (g_str_has_suffix (name, PHOTO_NAME_SUFFIX))
    {
      filename = g_strjoin (G_DIR_SEPARATOR_S, path_photos, name, NULL);
      file     = g_file_new_for_path (filename);

      files_list = g_list_append (files_list, file);
      g_free (filename);
    }
  }

  /* delete all items */
  cheese_window_cmd_move_file_to_trash (cheese_window, files_list);
  g_list_free (files_list);
  g_dir_close (dir_photos);
}

static void
cheese_window_delete_media (GtkWidget *widget, CheeseWindow *cheese_window)
{
  GList *files_list = NULL;

  files_list = cheese_thumb_view_get_selected_images_list (CHEESE_THUMB_VIEW (cheese_window->thumb_view));

  cheese_window_cmd_delete_file (cheese_window, files_list, FALSE);
  g_list_free (files_list);
}

static void
cheese_window_move_media_to_trash (GtkWidget *widget, CheeseWindow *cheese_window)
{
  GList *files_list = NULL;

  files_list = cheese_thumb_view_get_selected_images_list (CHEESE_THUMB_VIEW (cheese_window->thumb_view));

  cheese_window_cmd_move_file_to_trash (cheese_window, files_list);
  g_list_free (files_list);
}

static void
cheese_window_set_countdown (GtkWidget *widget, CheeseWindow *cheese_window)
{
  gboolean countdown = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (widget));

  g_object_set (cheese_window->gconf, "gconf_prop_countdown", countdown, NULL);
}

static void
cheese_window_cmd_set_about_me_photo (GtkWidget *widget, CheeseWindow *cheese_window)
{
  EContact  *contact;
  EBook     *book;
  GError    *error = NULL;
  GdkPixbuf *pixbuf;
  const int  MAX_PHOTO_HEIGHT = 150;
  const int  MAX_PHOTO_WIDTH  = 150;
  char      *filename;

  filename = cheese_thumb_view_get_selected_image (CHEESE_THUMB_VIEW (cheese_window->thumb_view));

  if (e_book_get_self (&contact, &book, NULL) && filename)
  {
    char *name = e_contact_get (contact, E_CONTACT_FULL_NAME);
    g_print ("Setting Account Photo for %s\n", name);

    pixbuf = gdk_pixbuf_new_from_file_at_scale (filename, MAX_PHOTO_HEIGHT,
                                                MAX_PHOTO_WIDTH, TRUE, NULL);
    if (contact)
    {
      EContactPhoto photo;
      guchar      **data;
      gsize        *length;

      photo.type                   = E_CONTACT_PHOTO_TYPE_INLINED;
      photo.data.inlined.mime_type = "image/jpeg";
      data                         = &photo.data.inlined.data;
      length                       = &photo.data.inlined.length;

      gdk_pixbuf_save_to_buffer (pixbuf, (char **) data, length, "png", NULL,
                                 "compression", "9", NULL);
      e_contact_set (contact, E_CONTACT_PHOTO, &photo);

      if (!e_book_commit_contact (book, contact, &error))
      {
        char      *header;
        GtkWidget *dlg;

        header = g_strdup_printf (_("Could not set the Account Photo"));
        dlg    = gtk_message_dialog_new (GTK_WINDOW (cheese_window->window),
                                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", header);
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), "%s", error->message);
        gtk_dialog_run (GTK_DIALOG (dlg));
        gtk_widget_destroy (dlg);
        g_free (header);
        g_error_free (error);
      }
      g_free (*data);
    }
    g_free (filename);
    g_object_unref (pixbuf);
  }
}

static void
cheese_window_cmd_command_line (GtkAction *action, CheeseWindow *cheese_window)
{
  GError     *error = NULL;
  char       *command_line;
  const char *action_name;
  GList      *files, *l;

  files = cheese_thumb_view_get_selected_images_list (CHEESE_THUMB_VIEW (cheese_window->thumb_view));
  char *filename = cheese_thumb_view_get_selected_image (CHEESE_THUMB_VIEW (cheese_window->thumb_view));

  action_name = gtk_action_get_name (action);
  if (strcmp (action_name, "SendByMail") == 0)
  {
    char *path;
    command_line = g_strdup_printf ("xdg-open mailto:?subject='%s'", _("Media files"));
    for (l = files; l != NULL; l = l->next)
    {
      path         = g_file_get_path (l->data);
      command_line = g_strjoin ("&attachment=", command_line, path, NULL);
      g_free (path);
      g_object_unref (l->data);
    }
    g_list_free (l);
    g_list_free (files);
  }
  else if (strcmp (action_name, "SendTo") == 0)
  {
    char *path;
    command_line = g_strdup_printf ("nautilus-sendto");
    for (l = files; l != NULL; l = l->next)
    {
      path         = g_file_get_path (l->data);
      command_line = g_strjoin (" ", command_line, path, NULL);
      g_free (path);
      g_object_unref (l->data);
    }
    g_list_free (l);
    g_list_free (files);
  }
  else if (strcmp (action_name, "ExportToFSpot") == 0)
  {
    char *dirname = g_path_get_dirname (filename);
    command_line = g_strdup_printf ("f-spot -i %s", dirname);
    g_free (dirname);
  }
  else if (strcmp (action_name, "ExportToFlickr") == 0)
  {
    char *path;
    command_line = g_strdup_printf ("postr");
    for (l = files; l != NULL; l = l->next)
    {
      path         = g_file_get_path (l->data);
      command_line = g_strjoin (" ", command_line, path, NULL);
      g_free (path);
      g_object_unref (l->data);
    }
    g_list_free (l);
    g_list_free (files);
  }
  else
  {
    return;
  }
  g_free (filename);

  if (!g_spawn_command_line_async (command_line, &error))
  {
    g_warning ("cannot launch command line: %s\n", error->message);
    g_error_free (error);
  }
  g_free (command_line);
}

static void
cheese_window_cmd_help_contents (GtkAction *action, CheeseWindow *cheese_window)
{
  GError    *error = NULL;
  GdkScreen *screen;

  screen = gtk_widget_get_screen (GTK_WIDGET (cheese_window));
  gtk_show_uri (screen, "ghelp:cheese", gtk_get_current_event_time (), &error);

  if (error != NULL)
  {
    GtkWidget *d;
    d = gtk_message_dialog_new (GTK_WINDOW (cheese_window->window),
                                GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                _("Unable to open help file for Cheese"));
    gtk_dialog_run (GTK_DIALOG (d));
    gtk_widget_destroy (d);
    g_error_free (error);
  }
}

static void
cheese_window_cmd_about (GtkAction *action, CheeseWindow *cheese_window)
{
  static const char *authors[] = {
    "daniel g. siegel <dgsiegel@gnome.org>",
    "Jaap A. Haitsma <jaap@haitsma.org>",
    "Filippo Argiolas <fargiolas@gnome.org>",
    "",
    "Aidan Delaney <a.j.delaney@brighton.ac.uk>",
    "Alex \"weej\" Jones <alex@weej.com>",
    "Andrea Cimitan <andrea.cimitan@gmail.com>",
    "Baptiste Mille-Mathias <bmm80@free.fr>",
    "Cosimo Cecchi <anarki@lilik.it>",
    "Diego Escalante Urrelo <dieguito@gmail.com>",
    "Felix Kaser <f.kaser@gmx.net>",
    "Gintautas Miliauskas <gintas@akl.lt>",
    "Hans de Goede <jwrdegoede@fedoraproject.org>",
    "James Liggett <jrliggett@cox.net>",
    "Luca Ferretti <elle.uca@libero.it>",
    "Mirco \"MacSlow\" Müller <macslow@bangang.de>",
    "Patryk Zawadzki <patrys@pld-linux.org>",
    "Ryan Zeigler <zeiglerr@gmail.com>",
    "Sebastian Keller <sebastian-keller@gmx.de>",
    "Steve Magoun <steve.magoun@canonical.com>",
    "Thomas Perl <thp@thpinfo.com>",
    "Tim Philipp Müller <tim@centricular.net>",
    "Todd Eisenberger <teisenberger@gmail.com>",
    "Tommi Vainikainen <thv@iki.fi>",
    NULL
  };

  static const char *artists[] = {
    "Andreas Nilsson <andreas@andreasn.se>",
    "Josef Vybíral <josef.vybiral@gmail.com>",
    "Kalle Persson <kalle@kallepersson.se>",
    "Lapo Calamandrei <calamandrei@gmail.com>",
    "Or Dvory <gnudles@nana.co.il>",
    "Ulisse Perusin <ulisail@yahoo.it>",
    NULL
  };

  static const char *documenters[] = {
    "Joshua Henderson <joshhendo@gmail.com>",
    "Jaap A. Haitsma <jaap@haitsma.org>",
    NULL
  };

  const char *translators;

  translators = _("translator-credits");

  const char *license[] = {
    N_("This program is free software; you can redistribute it and/or modify "
       "it under the terms of the GNU General Public License as published by "
       "the Free Software Foundation; either version 2 of the License, or "
       "(at your option) any later version.\n"),
    N_("This program is distributed in the hope that it will be useful, "
       "but WITHOUT ANY WARRANTY; without even the implied warranty of "
       "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
       "GNU General Public License for more details.\n"),
    N_("You should have received a copy of the GNU General Public License "
       "along with this program. If not, see <http://www.gnu.org/licenses/>.")
  };

  char *license_trans;

  license_trans = g_strconcat (_(license[0]), "\n", _(license[1]), "\n", _(license[2]), "\n", NULL);

  gtk_show_about_dialog (GTK_WINDOW (cheese_window->window),
                         "version", VERSION,
                         "copyright", "Copyright \xc2\xa9 2007 - 2009\n daniel g. siegel <dgsiegel@gnome.org>",
                         "comments", _("Take photos and videos with your webcam, with fun graphical effects"),
                         "authors", authors,
                         "translator-credits", translators,
                         "artists", artists,
                         "documenters", documenters,
                         "website", "http://projects.gnome.org/cheese",
                         "website-label", _("Cheese Website"),
                         "logo-icon-name", "cheese",
                         "wrap-license", TRUE,
                         "license", license_trans,
                         NULL);

  g_free (license_trans);
}

static void
cheese_window_selection_changed_cb (GtkIconView  *iconview,
                                    CheeseWindow *cheese_window)
{
  if (cheese_thumb_view_get_n_selected (CHEESE_THUMB_VIEW (cheese_window->thumb_view)) > 0)
  {
    gtk_action_group_set_sensitive (cheese_window->actions_file, TRUE);
  }
  else
  {
    gtk_action_group_set_sensitive (cheese_window->actions_file, FALSE);
  }
}

static gboolean
cheese_window_button_press_event_cb (GtkWidget *iconview, GdkEventButton *event,
                                     CheeseWindow *cheese_window)
{
  GtkTreePath *path;

  if (event->type == GDK_BUTTON_PRESS || event->type == GDK_2BUTTON_PRESS)
  {
    path = gtk_icon_view_get_path_at_pos (GTK_ICON_VIEW (iconview),
                                          (int) event->x, (int) event->y);
    if (path == NULL) return FALSE;

    if (event->type == GDK_BUTTON_PRESS && event->button == 1)
    {
      if (cheese_thumb_view_get_n_selected (CHEESE_THUMB_VIEW (cheese_window->thumb_view)) > 1)
      {
        gtk_icon_view_unselect_all (GTK_ICON_VIEW (cheese_window->thumb_view));
        gtk_icon_view_select_path (GTK_ICON_VIEW (cheese_window->thumb_view), path);
        gtk_icon_view_set_cursor (GTK_ICON_VIEW (cheese_window->thumb_view), path, NULL, FALSE);
      }
    }
    else if (event->type == GDK_BUTTON_PRESS && event->button == 3)
    {
      int button, event_time;

      if (event)
      {
        button     = event->button;
        event_time = event->time;
      }
      else
      {
        button     = 0;
        event_time = gtk_get_current_event_time ();
      }

      if (!gtk_icon_view_path_is_selected (GTK_ICON_VIEW (cheese_window->thumb_view), path) ||
          cheese_thumb_view_get_n_selected (CHEESE_THUMB_VIEW (cheese_window->thumb_view)) <= 1)
      {
        gtk_icon_view_unselect_all (GTK_ICON_VIEW (cheese_window->thumb_view));
        gtk_icon_view_select_path (GTK_ICON_VIEW (cheese_window->thumb_view), path);
        gtk_icon_view_set_cursor (GTK_ICON_VIEW (cheese_window->thumb_view), path, NULL, FALSE);
      }

      GList   *l, *files;
      gchar   *file;
      gboolean list_has_videos = FALSE;
      files = cheese_thumb_view_get_selected_images_list (CHEESE_THUMB_VIEW (cheese_window->thumb_view));

      for (l = files; l != NULL; l = l->next)
      {
        file = g_file_get_path (l->data);
        if (g_str_has_suffix (file, VIDEO_NAME_SUFFIX))
        {
          list_has_videos = TRUE;
        }
        g_free (file);
        g_object_unref (l->data);
      }
      g_list_free (l);
      g_list_free (files);

      if (list_has_videos)
      {
        gtk_action_group_set_sensitive (cheese_window->actions_flickr, FALSE);
        gtk_action_group_set_sensitive (cheese_window->actions_fspot, FALSE);
        gtk_action_group_set_sensitive (cheese_window->actions_account_photo, FALSE);
      }
      else
      {
        gtk_action_group_set_sensitive (cheese_window->actions_flickr, TRUE);
        gtk_action_group_set_sensitive (cheese_window->actions_fspot, TRUE);
        gtk_action_group_set_sensitive (cheese_window->actions_account_photo, TRUE);
      }

      gtk_menu_popup (GTK_MENU (cheese_window->thumb_view_popup_menu),
                      NULL, iconview, NULL, NULL, button, event_time);

      return TRUE;
    }
    else if (event->type == GDK_2BUTTON_PRESS && event->button == 1)
    {
      cheese_window_cmd_open (NULL, cheese_window);
      return TRUE;
    }
  }
  return FALSE;
}

static void
cheese_window_effect_button_pressed_cb (GtkWidget *widget, CheeseWindow *cheese_window)
{
  if (gtk_notebook_get_current_page (GTK_NOTEBOOK (cheese_window->notebook)) == 1)
  {
    gtk_notebook_set_current_page (GTK_NOTEBOOK (cheese_window->notebook), 0);
    gtk_label_set_text_with_mnemonic (GTK_LABEL (cheese_window->label_effects), _("_Effects"));
    gtk_widget_set_sensitive (cheese_window->take_picture, TRUE);
    gtk_widget_set_sensitive (cheese_window->take_picture_fullscreen, TRUE);
    if (cheese_window->webcam_mode == WEBCAM_MODE_PHOTO)
    {
      gtk_action_group_set_sensitive (cheese_window->actions_photo, TRUE);
    }
    else if (cheese_window->webcam_mode == WEBCAM_MODE_BURST)
    {
      gtk_action_group_set_sensitive (cheese_window->actions_burst, TRUE);
    }
    else
    {
      gtk_action_group_set_sensitive (cheese_window->actions_video, TRUE);
    }
    cheese_webcam_set_effect (cheese_window->webcam,
                              cheese_effect_chooser_get_selection (CHEESE_EFFECT_CHOOSER (cheese_window->effect_chooser)));
    g_object_set (cheese_window->gconf, "gconf_prop_selected_effects",
                  cheese_effect_chooser_get_selection_string (CHEESE_EFFECT_CHOOSER (cheese_window->effect_chooser)),
                  NULL);
  }
  else
  {
    gtk_notebook_set_current_page (GTK_NOTEBOOK (cheese_window->notebook), 1);
    gtk_widget_set_sensitive (GTK_WIDGET (cheese_window->take_picture), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (cheese_window->take_picture_fullscreen), FALSE);
    gtk_action_group_set_sensitive (cheese_window->actions_photo, FALSE);
    gtk_action_group_set_sensitive (cheese_window->actions_video, FALSE);
    gtk_action_group_set_sensitive (cheese_window->actions_burst, FALSE);
  }
}

void
cheese_window_countdown_hide_cb (gpointer data)
{
  CheeseWindow *cheese_window = (CheeseWindow *) data;

  gtk_notebook_set_current_page (GTK_NOTEBOOK (cheese_window->notebook_bar), 0);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (cheese_window->fullscreen_bar), 0);
}

void
cheese_window_countdown_picture_cb (gpointer data)
{
  CheeseWindow *cheese_window = (CheeseWindow *) data;
  GError       *error         = NULL;
  GstAudioPlay *audio_play;
  char         *shutter_filename;
  char         *photo_filename;

  photo_filename = cheese_fileutil_get_new_media_filename (cheese_window->fileutil, WEBCAM_MODE_PHOTO);
  if (cheese_webcam_take_photo (cheese_window->webcam, photo_filename))
  {
    cheese_flash_fire (cheese_window->flash);
    shutter_filename = audio_play_get_filename (cheese_window);
    audio_play       = gst_audio_play_file (shutter_filename, &error);
    if (!audio_play)
    {
      g_warning ("%s", error ? error->message : "Unknown error");
      g_error_free (error);
    }
    g_free (shutter_filename);
  }
  g_free (photo_filename);
}

static void
cheese_window_no_camera_info_bar_response (GtkWidget *widget, gint response_id, CheeseWindow *cheese_window)
{
  GError  *error = NULL;
  gboolean ret;

  if (response_id == GTK_RESPONSE_HELP)
  {
    ret = g_app_info_launch_default_for_uri ("ghelp:cheese?faq", NULL, &error);

    if (ret == FALSE)
    {
      GtkWidget *d;
      d = gtk_message_dialog_new (GTK_WINDOW (cheese_window->window),
                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                  _("Unable to open help file for Cheese"));
      gtk_dialog_run (GTK_DIALOG (d));
      gtk_widget_destroy (d);
      g_error_free (error);
    }
  }
}

static void
cheese_window_stop_recording (CheeseWindow *cheese_window)
{
  if (cheese_window->recording)
  {
    gtk_action_group_set_sensitive (cheese_window->actions_effects, TRUE);
    gtk_action_group_set_sensitive (cheese_window->actions_toggle, TRUE);
    gtk_widget_set_sensitive (cheese_window->take_picture, FALSE);
    gchar *str = g_strconcat ("<b>", _("_Start Recording"), "</b>", NULL);
    gtk_label_set_text_with_mnemonic (GTK_LABEL (cheese_window->label_take_photo), str);
    gtk_label_set_text_with_mnemonic (GTK_LABEL (cheese_window->label_take_photo_fullscreen), str);
    g_free (str);
    gtk_label_set_use_markup (GTK_LABEL (cheese_window->label_take_photo), TRUE);
    gtk_image_set_from_stock (GTK_IMAGE (cheese_window->image_take_photo), GTK_STOCK_MEDIA_RECORD, GTK_ICON_SIZE_BUTTON);
    gtk_label_set_use_markup (GTK_LABEL (cheese_window->label_take_photo_fullscreen), TRUE);
    gtk_image_set_from_stock (GTK_IMAGE (cheese_window->image_take_photo_fullscreen),
                              GTK_STOCK_MEDIA_RECORD, GTK_ICON_SIZE_BUTTON);

    cheese_webcam_stop_video_recording (cheese_window->webcam);
    cheese_window->recording = FALSE;
  }
}

static gboolean
cheese_window_escape_key_cb (CheeseWindow *cheese_window,
                             GtkAccelGroup *accel_group,
                             guint keyval, GdkModifierType modifier)
{
  if (cheese_window->isFullscreen)
  {
    if (cheese_countdown_get_state (CHEESE_COUNTDOWN (cheese_window->countdown_fullscreen)) == 0)
    {
      GtkAction *action = gtk_ui_manager_get_action (cheese_window->ui_manager, "/MainMenu/Cheese/Fullscreen");
      gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), FALSE);
      return TRUE;
    }
  }

  cheese_countdown_cancel ((CheeseCountdown *) cheese_window->countdown);
  cheese_countdown_cancel ((CheeseCountdown *) cheese_window->countdown_fullscreen);

  if (cheese_window->webcam_mode == WEBCAM_MODE_PHOTO)
  {
    gtk_notebook_set_current_page (GTK_NOTEBOOK (cheese_window->notebook_bar), 0);
    gtk_notebook_set_current_page (GTK_NOTEBOOK (cheese_window->fullscreen_bar), 0);

    gtk_widget_set_sensitive (cheese_window->take_picture, TRUE);
    gtk_widget_set_sensitive (cheese_window->take_picture_fullscreen, TRUE);
  }
  else if (cheese_window->webcam_mode == WEBCAM_MODE_BURST)
  {
    cheese_window->repeat_count = 0;
    cheese_window->is_bursting  = FALSE;

    gtk_notebook_set_current_page (GTK_NOTEBOOK (cheese_window->notebook_bar), 0);
    gtk_notebook_set_current_page (GTK_NOTEBOOK (cheese_window->fullscreen_bar), 0);

    gtk_action_group_set_sensitive (cheese_window->actions_effects, TRUE);
    gtk_action_group_set_sensitive (cheese_window->actions_toggle, TRUE);

    gtk_widget_set_sensitive (cheese_window->take_picture, TRUE);
    gtk_widget_set_sensitive (cheese_window->take_picture_fullscreen, TRUE);
  }
  else
  {
    cheese_window_stop_recording (cheese_window);
  }
  return TRUE;
}

static gboolean
cheese_window_take_photo (gpointer data)
{
  gboolean      countdown;
  CheeseWindow *cheese_window = (CheeseWindow *) data;

  /* return if burst mode was cancelled */
  if (cheese_window->webcam_mode == WEBCAM_MODE_BURST &&
      !cheese_window->is_bursting && cheese_window->repeat_count <= 0)
  {
    return FALSE;
  }

  g_object_get (cheese_window->gconf, "gconf_prop_countdown", &countdown, NULL);
  if (countdown)
  {
    if (cheese_window->isFullscreen)
    {
      cheese_countdown_start ((CheeseCountdown *) cheese_window->countdown_fullscreen,
                              cheese_window_countdown_picture_cb,
                              cheese_window_countdown_hide_cb,
                              (gpointer) cheese_window);
      gtk_notebook_set_current_page (GTK_NOTEBOOK (cheese_window->fullscreen_bar), 1);

      /* show bar, start timeout
       * ATTENTION: if the countdown is longer than FULLSCREEN_TIMEOUT,
       * the bar will disapear before the countdown ends
       */
      cheese_window_fullscreen_show_bar (cheese_window);
      cheese_window_fullscreen_set_timeout (cheese_window);
    }
    else
    {
      cheese_countdown_start ((CheeseCountdown *) cheese_window->countdown,
                              cheese_window_countdown_picture_cb,
                              cheese_window_countdown_hide_cb,
                              (gpointer) cheese_window);
      gtk_notebook_set_current_page (GTK_NOTEBOOK (cheese_window->notebook_bar), 1);
    }
  }
  else
  {
    cheese_window_countdown_picture_cb (cheese_window);
  }

  gtk_widget_set_sensitive (cheese_window->take_picture, FALSE);
  gtk_widget_set_sensitive (cheese_window->take_picture_fullscreen, FALSE);

  if (cheese_window->webcam_mode == WEBCAM_MODE_BURST)
  {
    guint    repeat_delay = 1000;
    gboolean countdown    = FALSE;

    g_object_get (cheese_window->gconf, "gconf_prop_burst_delay", &repeat_delay, NULL);
    g_object_get (cheese_window->gconf, "gconf_prop_countdown", &countdown, NULL);

    if (countdown && repeat_delay < 5000)
    {
      /* A countdown takes 4 seconds, leave some headroom before repeating it.
       * Magic number chosen via expiriment on Netbook */
      repeat_delay = 5000;
    }

    /* start burst mode phot series */
    if (!cheese_window->is_bursting)
    {
      g_timeout_add (repeat_delay, cheese_window_take_photo, cheese_window);
      cheese_window->is_bursting = TRUE;
    }
    cheese_window->repeat_count--;
    if (cheese_window->repeat_count > 0)
    {
      return TRUE;
    }
  }
  cheese_window->is_bursting = FALSE;

  return FALSE;
}

static void
cheese_window_action_button_clicked_cb (GtkWidget *widget, CheeseWindow *cheese_window)
{
  char *str;

  switch (cheese_window->webcam_mode)
  {
    case WEBCAM_MODE_BURST:

      /* ignore keybindings and other while bursting */
      if (cheese_window->is_bursting)
      {
        break;
      }
      gtk_action_group_set_sensitive (cheese_window->actions_effects, FALSE);
      gtk_action_group_set_sensitive (cheese_window->actions_toggle, FALSE);
      g_object_get (cheese_window->gconf, "gconf_prop_burst_repeat", &cheese_window->repeat_count, NULL); /* reset burst counter */
    case WEBCAM_MODE_PHOTO:
      cheese_window_take_photo (cheese_window);
      break;
    case WEBCAM_MODE_VIDEO:
      if (!cheese_window->recording)
      {
        gtk_action_group_set_sensitive (cheese_window->actions_effects, FALSE);
        gtk_action_group_set_sensitive (cheese_window->actions_toggle, FALSE);
        str = g_strconcat ("<b>", _("_Stop Recording"), "</b>", NULL);
        gtk_label_set_text_with_mnemonic (GTK_LABEL (cheese_window->label_take_photo), str);
        gtk_label_set_text_with_mnemonic (GTK_LABEL (cheese_window->label_take_photo_fullscreen), str);
        g_free (str);
        gtk_label_set_use_markup (GTK_LABEL (cheese_window->label_take_photo), TRUE);
        gtk_image_set_from_stock (GTK_IMAGE (
                                    cheese_window->image_take_photo), GTK_STOCK_MEDIA_STOP, GTK_ICON_SIZE_BUTTON);
        gtk_label_set_use_markup (GTK_LABEL (cheese_window->label_take_photo_fullscreen), TRUE);
        gtk_image_set_from_stock (GTK_IMAGE (cheese_window->image_take_photo_fullscreen),
                                  GTK_STOCK_MEDIA_STOP, GTK_ICON_SIZE_BUTTON);

        cheese_window->video_filename = cheese_fileutil_get_new_media_filename (cheese_window->fileutil,
                                                                                WEBCAM_MODE_VIDEO);
        cheese_webcam_start_video_recording (cheese_window->webcam, cheese_window->video_filename);

        cheese_window->recording = TRUE;
      }
      else
      {
        cheese_window_stop_recording (cheese_window);
      }
      break;
    default:
      g_assert_not_reached ();
      break;
  }
}

static void
cheese_window_preferences_cb (GtkAction *action, CheeseWindow *cheese_window)
{
  cheese_prefs_dialog_run (cheese_window->window, cheese_window->gconf,
                           cheese_window->webcam);
}

static const GtkActionEntry action_entries_main[] = {
  {"Cheese",       NULL,            N_("_Cheese")                       },

  {"Edit",         NULL,            N_("_Edit")                         },
  {"RemoveAll",    NULL,            N_("Move All to Trash"), NULL, NULL,
   G_CALLBACK (cheese_window_move_all_media_to_trash)},

  {"Help",         NULL,            N_("_Help")                         },

  {"Quit",         GTK_STOCK_QUIT,  NULL, NULL, NULL, G_CALLBACK (cheese_window_cmd_close)},
  {"HelpContents", GTK_STOCK_HELP,  N_("_Contents"), "F1", N_("Help on this Application"),
   G_CALLBACK (cheese_window_cmd_help_contents)},
  {"About",        GTK_STOCK_ABOUT, NULL, NULL, NULL, G_CALLBACK (cheese_window_cmd_about)},
};

static const GtkToggleActionEntry action_entries_countdown[] = {
  {"Countdown", NULL, N_("Countdown"), NULL, NULL, G_CALLBACK (cheese_window_set_countdown), FALSE},
};

static const GtkToggleActionEntry action_entries_effects[] = {
  {"Effects", NULL, N_("_Effects"), NULL, NULL, G_CALLBACK (cheese_window_effect_button_pressed_cb), FALSE},
};

static const GtkActionEntry action_entries_preferences[] = {
  {"Preferences", GTK_STOCK_PREFERENCES, N_("Preferences"), NULL, NULL, G_CALLBACK (cheese_window_preferences_cb)},
};

static const GtkToggleActionEntry action_entries_fullscreen[] = {
  {"Fullscreen", GTK_STOCK_FULLSCREEN, NULL, "F11", NULL, G_CALLBACK (cheese_window_toggle_fullscreen), FALSE},
};

static const GtkToggleActionEntry action_entries_wide_mode[] = {
  {"WideMode", NULL, N_("_Wide mode"), NULL, NULL, G_CALLBACK (cheese_window_toggle_wide_mode), FALSE},
};

static const GtkRadioActionEntry action_entries_toggle[] = {
  {"Photo", NULL, N_("_Photo"), NULL, NULL, 0},
  {"Video", NULL, N_("_Video"), NULL, NULL, 1},
  {"Burst", NULL, N_("_Burst"), NULL, NULL, 2},
};

static const GtkActionEntry action_entries_file[] = {
  {"Open",        GTK_STOCK_OPEN,    N_("_Open"),          "<control>O",    NULL,
   G_CALLBACK (cheese_window_cmd_open)},
  {"SaveAs",      GTK_STOCK_SAVE_AS, N_("Save _As..."),    "<control>S",    NULL,
   G_CALLBACK (cheese_window_cmd_save_as)},
  {"MoveToTrash", "user-trash",      N_("Move to _Trash"), "Delete",        NULL,
   G_CALLBACK (cheese_window_move_media_to_trash)},
  {"Delete",      NULL,              N_("Delete"),         "<shift>Delete", NULL,
   G_CALLBACK (cheese_window_delete_media)},
};

static const GtkActionEntry action_entries_photo[] = {
  {"TakePhoto", NULL, N_("_Take a Photo"), "space", NULL, G_CALLBACK (cheese_window_action_button_clicked_cb)},
};

static const GtkToggleActionEntry action_entries_video[] = {
  {"TakeVideo", NULL, N_("_Recording"), "space", NULL, G_CALLBACK (cheese_window_action_button_clicked_cb), FALSE},
};

static const GtkActionEntry action_entries_burst[] = {
  {"TakeBurst", NULL, N_("_Take multiple Photos"), "space", NULL, G_CALLBACK (cheese_window_action_button_clicked_cb)},
};

static const GtkActionEntry action_entries_account_photo[] = {
  {"SetAsAccountPhoto", NULL, N_("_Set As Account Photo"), NULL, NULL, G_CALLBACK (cheese_window_cmd_set_about_me_photo)},
};

static const GtkActionEntry action_entries_mail[] = {
  {"SendByMail", NULL, N_("Send by _Mail"), NULL, NULL, G_CALLBACK (cheese_window_cmd_command_line)},
};

static const GtkActionEntry action_entries_sendto[] = {
  {"SendTo", NULL, N_("Send _To"), NULL, NULL, G_CALLBACK (cheese_window_cmd_command_line)},
};

static const GtkActionEntry action_entries_fspot[] = {
  {"ExportToFSpot", NULL, N_("Export to F-_Spot"), NULL, NULL, G_CALLBACK (cheese_window_cmd_command_line)},
};

static const GtkActionEntry action_entries_flickr[] = {
  {"ExportToFlickr", NULL, N_("Export to _Flickr"), NULL, NULL, G_CALLBACK (cheese_window_cmd_command_line)},
};

static void
cheese_window_activate_radio_action (GtkAction *action, GtkRadioAction *current, CheeseWindow *cheese_window)
{
  gchar *str;

  if (strcmp (gtk_action_get_name (GTK_ACTION (current)), "Photo") == 0)
  {
    cheese_window->webcam_mode = WEBCAM_MODE_PHOTO;

    str = g_strconcat ("<b>", _("_Take a Photo"), "</b>", NULL);
    gtk_label_set_text_with_mnemonic (GTK_LABEL (cheese_window->label_take_photo), g_strdup (str));
    gtk_label_set_use_markup (GTK_LABEL (cheese_window->label_take_photo), TRUE);
    gtk_label_set_text_with_mnemonic (GTK_LABEL (cheese_window->label_take_photo_fullscreen), g_strdup (str));
    gtk_label_set_use_markup (GTK_LABEL (cheese_window->label_take_photo_fullscreen), TRUE);
    gtk_action_group_set_sensitive (cheese_window->actions_photo, TRUE);
    gtk_action_group_set_sensitive (cheese_window->actions_video, FALSE);
    gtk_action_group_set_sensitive (cheese_window->actions_burst, FALSE);
  }
  else if (strcmp (gtk_action_get_name (GTK_ACTION (current)), "Burst") == 0)
  {
    cheese_window->webcam_mode = WEBCAM_MODE_BURST;

    str = g_strconcat ("<b>", _("_Take multiple Photos"), "</b>", NULL);
    gtk_label_set_text_with_mnemonic (GTK_LABEL (cheese_window->label_take_photo), g_strdup (str));
    gtk_label_set_use_markup (GTK_LABEL (cheese_window->label_take_photo), TRUE);
    gtk_label_set_text_with_mnemonic (GTK_LABEL (cheese_window->label_take_photo_fullscreen), g_strdup (str));
    gtk_label_set_use_markup (GTK_LABEL (cheese_window->label_take_photo_fullscreen), TRUE);
    gtk_action_group_set_sensitive (cheese_window->actions_photo, FALSE);
    gtk_action_group_set_sensitive (cheese_window->actions_video, FALSE);
    gtk_action_group_set_sensitive (cheese_window->actions_burst, TRUE);
  }
  else
  {
    cheese_window->webcam_mode = WEBCAM_MODE_VIDEO;

    str = g_strconcat ("<b>", _("_Start recording"), "</b>", NULL);
    gtk_label_set_text_with_mnemonic (GTK_LABEL (cheese_window->label_take_photo), g_strdup (str));
    gtk_label_set_use_markup (GTK_LABEL (cheese_window->label_take_photo), TRUE);
    gtk_label_set_text_with_mnemonic (GTK_LABEL (cheese_window->label_take_photo_fullscreen), g_strdup (str));
    gtk_label_set_use_markup (GTK_LABEL (cheese_window->label_take_photo_fullscreen), TRUE);
    gtk_action_group_set_sensitive (cheese_window->actions_photo, FALSE);
    gtk_action_group_set_sensitive (cheese_window->actions_video, TRUE);
    gtk_action_group_set_sensitive (cheese_window->actions_burst, FALSE);
  }
  g_free (str);
}

GtkActionGroup *
cheese_window_action_group_new (CheeseWindow *cheese_window, char *name,
                                const GtkActionEntry *action_entries, int num_action_entries)
{
  GtkActionGroup *action_group;

  action_group = gtk_action_group_new (name);
  gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions (action_group, action_entries,
                                num_action_entries, cheese_window);
  gtk_ui_manager_insert_action_group (cheese_window->ui_manager, action_group, 0);

  return action_group;
}

GtkActionGroup *
cheese_window_toggle_action_group_new (CheeseWindow *cheese_window, char *name,
                                       const GtkToggleActionEntry *action_entries, int num_action_entries)
{
  GtkActionGroup *action_group;

  action_group = gtk_action_group_new (name);
  gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_toggle_actions (action_group, action_entries,
                                       num_action_entries, cheese_window);
  gtk_ui_manager_insert_action_group (cheese_window->ui_manager, action_group, 0);

  return action_group;
}

GtkActionGroup *
cheese_window_radio_action_group_new (CheeseWindow *cheese_window, char *name,
                                      const GtkRadioActionEntry *action_entries, int num_action_entries)
{
  GtkActionGroup *action_group;

  action_group = gtk_action_group_new (name);
  gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_radio_actions (action_group, action_entries,
                                      num_action_entries, 0,
                                      G_CALLBACK (cheese_window_activate_radio_action),
                                      cheese_window);
  gtk_ui_manager_insert_action_group (cheese_window->ui_manager, action_group, 0);

  return action_group;
}

static void
cheese_window_set_info_bar (CheeseWindow *cheese_window,
                            GtkWidget    *info_bar)
{
  if (cheese_window->info_bar == info_bar)
    return;

  if (cheese_window->info_bar != NULL)
    gtk_widget_destroy (cheese_window->info_bar);

  cheese_window->info_bar = info_bar;

  if (info_bar == NULL)
    return;

  gtk_container_add (GTK_CONTAINER (cheese_window->info_bar_frame), cheese_window->info_bar);
  gtk_widget_show (GTK_WIDGET (cheese_window->info_bar));
}

static void
cheese_window_create_window (CheeseWindow *cheese_window)
{
  GError     *error = NULL;
  char       *path;
  GtkBuilder *builder;
  GtkWidget  *menubar;

  cheese_window->info_bar = NULL;

  builder = gtk_builder_new ();
  gtk_builder_add_from_file (builder, PACKAGE_DATADIR "/cheese.ui", &error);

  if (error)
  {
    g_error ("building ui from %s failed: %s", PACKAGE_DATADIR "/cheese.ui", error->message);
    g_clear_error (&error);
  }

  cheese_window->window                      = GTK_WIDGET (gtk_builder_get_object (builder, "cheese_window"));
  cheese_window->button_effects              = GTK_WIDGET (gtk_builder_get_object (builder, "button_effects"));
  cheese_window->button_photo                = GTK_WIDGET (gtk_builder_get_object (builder, "button_photo"));
  cheese_window->button_video                = GTK_WIDGET (gtk_builder_get_object (builder, "button_video"));
  cheese_window->button_burst                = GTK_WIDGET (gtk_builder_get_object (builder, "button_burst"));
  cheese_window->image_take_photo            = GTK_WIDGET (gtk_builder_get_object (builder, "image_take_photo"));
  cheese_window->label_effects               = GTK_WIDGET (gtk_builder_get_object (builder, "label_effects"));
  cheese_window->label_photo                 = GTK_WIDGET (gtk_builder_get_object (builder, "label_photo"));
  cheese_window->label_take_photo            = GTK_WIDGET (gtk_builder_get_object (builder, "label_take_photo"));
  cheese_window->label_video                 = GTK_WIDGET (gtk_builder_get_object (builder, "label_video"));
  cheese_window->main_vbox                   = GTK_WIDGET (gtk_builder_get_object (builder, "main_vbox"));
  cheese_window->netbook_alignment           = GTK_WIDGET (gtk_builder_get_object (builder, "netbook_alignment"));
  cheese_window->togglegroup_alignment       = GTK_WIDGET (gtk_builder_get_object (builder, "togglegroup_alignment"));
  cheese_window->effect_button_alignment     = GTK_WIDGET (gtk_builder_get_object (builder, "effect_button_alignment"));
  cheese_window->toolbar_alignment           = GTK_WIDGET (gtk_builder_get_object (builder, "toolbar_alignment"));
  cheese_window->video_vbox                  = GTK_WIDGET (gtk_builder_get_object (builder, "video_vbox"));
  cheese_window->notebook                    = GTK_WIDGET (gtk_builder_get_object (builder, "notebook"));
  cheese_window->notebook_bar                = GTK_WIDGET (gtk_builder_get_object (builder, "notebook_bar"));
  cheese_window->screen                      = GTK_WIDGET (gtk_builder_get_object (builder, "video_screen"));
  cheese_window->take_picture                = GTK_WIDGET (gtk_builder_get_object (builder, "take_picture"));
  cheese_window->thumb_scrollwindow          = GTK_WIDGET (gtk_builder_get_object (builder, "thumb_scrollwindow"));
  cheese_window->throbber_frame              = GTK_WIDGET (gtk_builder_get_object (builder, "throbber_frame"));
  cheese_window->countdown_frame             = GTK_WIDGET (gtk_builder_get_object (builder, "countdown_frame"));
  cheese_window->effect_frame                = GTK_WIDGET (gtk_builder_get_object (builder, "effect_frame"));
  cheese_window->effect_alignment            = GTK_WIDGET (gtk_builder_get_object (builder, "effect_alignment"));
  cheese_window->info_bar_frame              = GTK_WIDGET (gtk_builder_get_object (builder, "info_bar_frame"));
  cheese_window->fullscreen_popup            = GTK_WIDGET (gtk_builder_get_object (builder, "fullscreen_popup"));
  cheese_window->fullscreen_bar              = GTK_WIDGET (gtk_builder_get_object (builder, "fullscreen_notebook_bar"));
  cheese_window->button_effects_fullscreen   = GTK_WIDGET (gtk_builder_get_object (builder, "button_effects_fullscreen"));
  cheese_window->button_photo_fullscreen     = GTK_WIDGET (gtk_builder_get_object (builder, "button_photo_fullscreen"));
  cheese_window->button_video_fullscreen     = GTK_WIDGET (gtk_builder_get_object (builder, "button_video_fullscreen"));
  cheese_window->button_burst_fullscreen     = GTK_WIDGET (gtk_builder_get_object (builder, "button_burst_fullscreen"));
  cheese_window->take_picture_fullscreen     = GTK_WIDGET (gtk_builder_get_object (builder, "take_picture_fullscreen"));
  cheese_window->label_take_photo_fullscreen =
    GTK_WIDGET (gtk_builder_get_object (builder, "label_take_photo_fullscreen"));
  cheese_window->image_take_photo_fullscreen =
    GTK_WIDGET (gtk_builder_get_object (builder, "image_take_photo_fullscreen"));
  cheese_window->label_photo_fullscreen     = GTK_WIDGET (gtk_builder_get_object (builder, "label_photo_fullscreen"));
  cheese_window->label_video_fullscreen     = GTK_WIDGET (gtk_builder_get_object (builder, "label_video_fullscreen"));
  cheese_window->countdown_frame_fullscreen =
    GTK_WIDGET (gtk_builder_get_object (builder, "countdown_frame_fullscreen"));
  cheese_window->button_exit_fullscreen = GTK_WIDGET (gtk_builder_get_object (builder, "button_exit_fullscreen"));

  g_object_unref (builder);

  /* configure the popup position and size */
  GdkScreen *screen = gtk_window_get_screen (GTK_WINDOW (cheese_window->fullscreen_popup));
  gtk_window_set_default_size (GTK_WINDOW (cheese_window->fullscreen_popup),
                               gdk_screen_get_width (screen), FULLSCREEN_POPUP_HEIGHT);
  gtk_window_move (GTK_WINDOW (cheese_window->fullscreen_popup), 0,
                   gdk_screen_get_height (screen) - FULLSCREEN_POPUP_HEIGHT);

  g_signal_connect (cheese_window->fullscreen_popup,
                    "enter-notify-event",
                    G_CALLBACK (cheese_window_fullscreen_leave_notify_cb),
                    cheese_window);

  g_signal_connect (cheese_window->button_exit_fullscreen, "clicked",
                    G_CALLBACK (cheese_window_exit_fullscreen_button_clicked_cb),
                    cheese_window);

  char *str = g_strconcat ("<b>", _("_Take a photo"), "</b>", NULL);
  gtk_label_set_text_with_mnemonic (GTK_LABEL (cheese_window->label_take_photo), str);
  gtk_label_set_text_with_mnemonic (GTK_LABEL (cheese_window->label_take_photo_fullscreen), str);
  g_free (str);
  gtk_label_set_use_markup (GTK_LABEL (cheese_window->label_take_photo), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (cheese_window->take_picture), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (cheese_window->label_take_photo_fullscreen), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (cheese_window->take_picture_fullscreen), FALSE);

  cheese_window->thumb_view = cheese_thumb_view_new ();
  cheese_window->thumb_nav  = eog_thumb_nav_new (cheese_window->thumb_view, FALSE);

  gtk_container_add (GTK_CONTAINER (cheese_window->thumb_scrollwindow), cheese_window->thumb_nav);

  /* show the scroll window to get it included in the size requisition done later */
  gtk_widget_show_all (cheese_window->thumb_scrollwindow);

  char *gconf_effects;
  g_object_get (cheese_window->gconf, "gconf_prop_selected_effects", &gconf_effects, NULL);
  cheese_window->effect_chooser = cheese_effect_chooser_new (gconf_effects);
  gtk_container_add (GTK_CONTAINER (cheese_window->effect_frame), cheese_window->effect_chooser);
  g_free (gconf_effects);

  cheese_window->throbber = ephy_spinner_new ();
  ephy_spinner_set_size (EPHY_SPINNER (cheese_window->throbber), GTK_ICON_SIZE_DIALOG);
  gtk_container_add (GTK_CONTAINER (cheese_window->throbber_frame), cheese_window->throbber);
  gtk_widget_show (cheese_window->throbber);

  cheese_window->countdown = cheese_countdown_new ();
  gtk_container_add (GTK_CONTAINER (cheese_window->countdown_frame), cheese_window->countdown);
  gtk_widget_show (cheese_window->countdown);

  cheese_window->countdown_fullscreen = cheese_countdown_new ();
  gtk_container_add (GTK_CONTAINER (cheese_window->countdown_frame_fullscreen), cheese_window->countdown_fullscreen);

  gtk_widget_realize (cheese_window->screen);
  GdkWindow *win = gtk_widget_get_window (cheese_window->screen);
  if (!gdk_window_ensure_native (win))
  {
    /* FIXME: this breaks offscreen stuff, we should really find
     * another way to embed video that doesn't require an XID */

    /* abort: no native window, no xoverlay, no cheese. */
    g_error ("Could not create a native X11 window for the drawing area");
  }
  gdk_window_set_back_pixmap (gtk_widget_get_window (cheese_window->screen), NULL, FALSE);
  gtk_widget_set_app_paintable (cheese_window->screen, TRUE);
  gtk_widget_set_double_buffered (cheese_window->screen, FALSE);
  gtk_widget_add_events (cheese_window->screen, GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);

  cheese_window->ui_manager = gtk_ui_manager_new ();

  cheese_window->actions_main = cheese_window_action_group_new (cheese_window,
                                                                "ActionsMain",
                                                                action_entries_main,
                                                                G_N_ELEMENTS (action_entries_main));
  cheese_window->actions_toggle = cheese_window_radio_action_group_new (cheese_window,
                                                                        "ActionsRadio",
                                                                        action_entries_toggle,
                                                                        G_N_ELEMENTS (action_entries_toggle));
  cheese_window->actions_effects = cheese_window_toggle_action_group_new (cheese_window,
                                                                          "ActionsEffects",
                                                                          action_entries_effects,
                                                                          G_N_ELEMENTS (action_entries_effects));

  cheese_window->actions_fullscreen = cheese_window_toggle_action_group_new (cheese_window,
                                                                             "ActionsFullscreen",
                                                                             action_entries_fullscreen,
                                                                             G_N_ELEMENTS (action_entries_fullscreen));

  cheese_window->actions_wide_mode = cheese_window_toggle_action_group_new (cheese_window,
                                                                            "ActionsWideMode",
                                                                            action_entries_wide_mode,
                                                                            G_N_ELEMENTS (action_entries_fullscreen));

  cheese_window->actions_preferences = cheese_window_action_group_new (cheese_window,
                                                                       "ActionsPreferences",
                                                                       action_entries_preferences,
                                                                       G_N_ELEMENTS (action_entries_preferences));
  cheese_window->actions_file = cheese_window_action_group_new (cheese_window,
                                                                "ActionsFile",
                                                                action_entries_file,
                                                                G_N_ELEMENTS (action_entries_file));
  cheese_window->actions_photo = cheese_window_action_group_new (cheese_window,
                                                                 "ActionsPhoto",
                                                                 action_entries_photo,
                                                                 G_N_ELEMENTS (action_entries_photo));
  cheese_window->actions_countdown = cheese_window_toggle_action_group_new (cheese_window,
                                                                            "ActionsCountdown",
                                                                            action_entries_countdown,
                                                                            G_N_ELEMENTS (action_entries_countdown));
  cheese_window->actions_video = cheese_window_toggle_action_group_new (cheese_window,
                                                                        "ActionsVideo",
                                                                        action_entries_video,
                                                                        G_N_ELEMENTS (action_entries_video));
  gtk_action_group_set_sensitive (cheese_window->actions_video, FALSE);
  cheese_window->actions_burst = cheese_window_action_group_new (cheese_window,
                                                                 "ActionsBurst",
                                                                 action_entries_burst,
                                                                 G_N_ELEMENTS (action_entries_burst));
  gtk_action_group_set_sensitive (cheese_window->actions_burst, FALSE);
  cheese_window->actions_account_photo = cheese_window_action_group_new (cheese_window,
                                                                         "ActionsAccountPhoto",
                                                                         action_entries_account_photo,
                                                                         G_N_ELEMENTS (action_entries_account_photo));
  cheese_window->actions_mail = cheese_window_action_group_new (cheese_window,
                                                                "ActionsMail",
                                                                action_entries_mail,
                                                                G_N_ELEMENTS (action_entries_mail));
  cheese_window->actions_sendto = cheese_window_action_group_new (cheese_window,
                                                                  "ActionsSendTo",
                                                                  action_entries_sendto,
                                                                  G_N_ELEMENTS (action_entries_sendto));

  /* handling and activation of send to/send mail actions. We only show one send mail action */
  path = g_find_program_in_path ("nautilus-sendto");
  gboolean nautilus_sendto = (path != NULL);
  if (nautilus_sendto)
  {
    gtk_action_group_set_visible (cheese_window->actions_sendto, TRUE);
    gtk_action_group_set_visible (cheese_window->actions_mail, FALSE);
  }
  else
  {
    path = g_find_program_in_path ("xdg-open");
    gtk_action_group_set_visible (cheese_window->actions_mail, path != NULL);
    gtk_action_group_set_visible (cheese_window->actions_sendto, FALSE);
  }
  g_free (path);

  cheese_window->actions_fspot = cheese_window_action_group_new (cheese_window,
                                                                 "ActionsFSpot",
                                                                 action_entries_fspot,
                                                                 G_N_ELEMENTS (action_entries_fspot));
  path = g_find_program_in_path ("f-spot");
  gtk_action_group_set_visible (cheese_window->actions_fspot, path != NULL);
  g_free (path);


  cheese_window->actions_flickr = cheese_window_action_group_new (cheese_window,
                                                                  "ActionsFlickr",
                                                                  action_entries_flickr,
                                                                  G_N_ELEMENTS (action_entries_flickr));
  path = g_find_program_in_path ("postr");
  gtk_action_group_set_visible (cheese_window->actions_flickr, path != NULL);
  g_free (path);

  gtk_ui_manager_add_ui_from_file (cheese_window->ui_manager, PACKAGE_DATADIR "/cheese-ui.xml", &error);

  if (error)
  {
    g_critical ("building menus from %s failed: %s", PACKAGE_DATADIR "/cheese-ui.xml", error->message);
    g_error_free (error);
  }

  GtkAction *action = gtk_ui_manager_get_action (cheese_window->ui_manager, "/MainMenu/Cheese/CountdownToggle");
  gboolean   countdown;
  g_object_get (cheese_window->gconf, "gconf_prop_countdown", &countdown, NULL);
  if (countdown)
  {
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), TRUE);
  }

  action = gtk_ui_manager_get_action (cheese_window->ui_manager, "/ThumbnailPopup/Delete");
  gboolean enable_delete;
  g_object_get (cheese_window->gconf, "gconf_prop_enable_delete", &enable_delete, NULL);
  gtk_action_set_visible (GTK_ACTION (action), enable_delete);

  menubar = gtk_ui_manager_get_widget (cheese_window->ui_manager, "/MainMenu");
  gtk_box_pack_start (GTK_BOX (cheese_window->main_vbox), menubar, FALSE, FALSE, 0);

  cheese_window->thumb_view_popup_menu = gtk_ui_manager_get_widget (cheese_window->ui_manager,
                                                                    "/ThumbnailPopup");

  gtk_window_add_accel_group (GTK_WINDOW (cheese_window->window),
                              gtk_ui_manager_get_accel_group (cheese_window->ui_manager));
  gtk_accel_group_connect (gtk_ui_manager_get_accel_group (cheese_window->ui_manager),
                           GDK_Escape, 0, 0,
                           g_cclosure_new_swap (G_CALLBACK (cheese_window_escape_key_cb),
                                                cheese_window, NULL));

  gtk_action_group_set_sensitive (cheese_window->actions_file, FALSE);

  action = gtk_ui_manager_get_action (cheese_window->ui_manager, "/MainMenu/Edit/Effects");
  gtk_activatable_set_related_action (GTK_ACTIVATABLE (cheese_window->button_effects), action);
  gtk_activatable_set_related_action (GTK_ACTIVATABLE (cheese_window->button_effects_fullscreen), action);

  action = gtk_ui_manager_get_action (cheese_window->ui_manager, "/MainMenu/Cheese/Photo");
  gtk_activatable_set_related_action (GTK_ACTIVATABLE (cheese_window->button_photo), action);
  gtk_activatable_set_related_action (GTK_ACTIVATABLE (cheese_window->button_photo_fullscreen), action);

  action = gtk_ui_manager_get_action (cheese_window->ui_manager, "/MainMenu/Cheese/Video");
  gtk_activatable_set_related_action (GTK_ACTIVATABLE (cheese_window->button_video), action);
  gtk_activatable_set_related_action (GTK_ACTIVATABLE (cheese_window->button_video_fullscreen), action);

  action = gtk_ui_manager_get_action (cheese_window->ui_manager, "/MainMenu/Cheese/Burst");
  gtk_activatable_set_related_action (GTK_ACTIVATABLE (cheese_window->button_burst), action);
  gtk_activatable_set_related_action (GTK_ACTIVATABLE (cheese_window->button_burst_fullscreen), action);


  /* Default handlers for closing the application */
  g_signal_connect (cheese_window->window, "destroy",
                    G_CALLBACK (cheese_window_cmd_close), cheese_window);
  g_signal_connect (cheese_window->window, "delete_event",
                    G_CALLBACK (cheese_window_delete_event_cb), NULL);

  /* Listen for key presses */
  gtk_widget_add_events (cheese_window->window, GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
  g_signal_connect (cheese_window->window, "key_press_event",
                    G_CALLBACK (cheese_window_key_press_event_cb), cheese_window);

  g_signal_connect (cheese_window->take_picture, "clicked",
                    G_CALLBACK (cheese_window_action_button_clicked_cb), cheese_window);
  g_signal_connect (cheese_window->take_picture_fullscreen, "clicked",
                    G_CALLBACK (cheese_window_action_button_clicked_cb), cheese_window);
  g_signal_connect (cheese_window->thumb_view, "selection_changed",
                    G_CALLBACK (cheese_window_selection_changed_cb), cheese_window);
  g_signal_connect (cheese_window->thumb_view, "button_press_event",
                    G_CALLBACK (cheese_window_button_press_event_cb), cheese_window);
}

void
setup_camera (CheeseWindow *cheese_window)
{
  char      *webcam_device = NULL;
  int        x_resolution;
  int        y_resolution;
  gdouble    brightness;
  gdouble    contrast;
  gdouble    saturation;
  gdouble    hue;
  GtkWidget *info_bar;

  GError *error;

  g_object_get (cheese_window->gconf,
                "gconf_prop_x_resolution", &x_resolution,
                "gconf_prop_y_resolution", &y_resolution,
                "gconf_prop_webcam", &webcam_device,
                "gconf_prop_brightness", &brightness,
                "gconf_prop_contrast", &contrast,
                "gconf_prop_saturation", &saturation,
                "gconf_prop_hue", &hue,
                NULL);

  gdk_threads_enter ();
  cheese_window->webcam = cheese_webcam_new (cheese_window->screen,
                                             webcam_device, x_resolution,
                                             y_resolution);
  gdk_threads_leave ();

  g_free (webcam_device);

  error = NULL;
  cheese_webcam_setup (cheese_window->webcam, cheese_window->startup_hal_dev_udi, &error);
  if (error != NULL)
  {
    GtkWidget *dialog;
    gchar     *primary, *secondary;

    primary   = g_strdup (_("Check your gstreamer installation"));
    secondary = g_strdup (error->message);

    gdk_threads_enter ();

    dialog = gtk_message_dialog_new (NULL,
                                     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_OK,
                                     "%s", primary);
    gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog),
                                                "%s", secondary);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    g_error_free (error);
    g_free (primary);
    g_free (secondary);

    /* Clean up and exit */
    cheese_window_cmd_close (NULL, cheese_window);

    gdk_threads_leave ();

    return;
  }

  g_signal_connect (cheese_window->webcam, "photo-saved",
                    G_CALLBACK (cheese_window_photo_saved_cb), cheese_window);
  g_signal_connect (cheese_window->webcam, "video-saved",
                    G_CALLBACK (cheese_window_video_saved_cb), cheese_window);

  cheese_webcam_set_effect (cheese_window->webcam,
                            cheese_effect_chooser_get_selection (CHEESE_EFFECT_CHOOSER (cheese_window->effect_chooser)));

  cheese_webcam_set_balance_property (cheese_window->webcam, "brightness", brightness);
  cheese_webcam_set_balance_property (cheese_window->webcam, "contrast", contrast);
  cheese_webcam_set_balance_property (cheese_window->webcam, "saturation", saturation);
  cheese_webcam_set_balance_property (cheese_window->webcam, "hue", hue);

  cheese_webcam_play (cheese_window->webcam);
  gdk_threads_enter ();
  gtk_notebook_set_current_page (GTK_NOTEBOOK (cheese_window->notebook), 0);
  ephy_spinner_stop (EPHY_SPINNER (cheese_window->throbber));
  if (cheese_webcam_get_num_webcam_devices (cheese_window->webcam) == 0)
  {
    info_bar = cheese_no_camera_info_bar_new ();

    g_signal_connect (info_bar,
                      "response",
                      G_CALLBACK (cheese_window_no_camera_info_bar_response),
                      cheese_window);

    cheese_window_set_info_bar (cheese_window, info_bar);
  }
  gtk_widget_set_sensitive (GTK_WIDGET (cheese_window->take_picture), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (cheese_window->take_picture_fullscreen), TRUE);
  gtk_action_group_set_sensitive (cheese_window->actions_effects, TRUE);

  gdk_threads_leave ();
}

void
cheese_window_init (char *hal_dev_udi, CheeseDbus *dbus_server, gboolean startup_in_wide_mode)
{
  CheeseWindow *cheese_window;
  gboolean      startup_in_wide_mode_saved;

  cheese_window = g_new0 (CheeseWindow, 1);

  cheese_window->startup_hal_dev_udi = hal_dev_udi;
  cheese_window->gconf               = cheese_gconf_new ();
  cheese_window->fileutil            = cheese_fileutil_new ();
  cheese_window->flash               = cheese_flash_new ();
  cheese_window->audio_play_counter  = 0;
  cheese_window->isFullscreen        = FALSE;
  cheese_window->is_bursting         = FALSE;

  cheese_window->server = dbus_server;

  /* save a pointer to the cheese window in cheese dbus */
  cheese_dbus_set_window (cheese_window);

  cheese_window->fullscreen_timeout_source = NULL;

  cheese_window_create_window (cheese_window);
  gtk_action_group_set_sensitive (cheese_window->actions_effects, FALSE);

  ephy_spinner_start (EPHY_SPINNER (cheese_window->throbber));

  gtk_notebook_set_current_page (GTK_NOTEBOOK (cheese_window->notebook), 2);

  cheese_window->webcam_mode = WEBCAM_MODE_PHOTO;
  cheese_window->recording   = FALSE;

  g_object_get (cheese_window->gconf,
                "gconf_prop_wide_mode",
                &startup_in_wide_mode_saved,
                NULL);

  startup_in_wide_mode = startup_in_wide_mode_saved ? TRUE : startup_in_wide_mode;

  if (startup_in_wide_mode)
  {
    GtkAction *action = gtk_ui_manager_get_action (cheese_window->ui_manager, "/MainMenu/Cheese/WideMode");
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), TRUE);
  }

  /* handy trick to set default size of the drawing area while not
   * limiting its minimum size, thanks Owen! */

  GtkRequisition req;
  gtk_widget_set_size_request (cheese_window->notebook,
                               DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
  gtk_widget_size_request (cheese_window->window, &req);
  gtk_window_resize (GTK_WINDOW (cheese_window->window), req.width, req.height);
  gtk_widget_set_size_request (cheese_window->notebook, -1, -1);

  gtk_widget_show_all (cheese_window->window);

  /* Run cam setup in its own thread */
  GError *error = NULL;
  if (!g_thread_create ((GThreadFunc) setup_camera, cheese_window, FALSE, &error))
  {
    g_error ("Failed to create setup thread: %s\n", error->message);
    g_error_free (error);
    return;
  }
}
