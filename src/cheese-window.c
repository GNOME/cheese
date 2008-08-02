/*
 * Copyright (C) 2007,2008 daniel g. siegel <dgsiegel@gmail.com>
 * Copyright (C) 2007,2008 Jaap Haitsma <jaap@haitsma.org>
 * Copyright (C) 2008 Patryk Zawadzki <patrys@pld-linux.org>
 * Copyright (C) 2008 Ryan Zeigler <zeiglerr@gmail.com>
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
#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include <gst/interfaces/xoverlay.h>
#include <gtk/gtk.h>
#include <libebook/e-book.h>

#ifdef HILDON
#include <hildon/hildon-program.h>
#endif

#include "cheese-countdown.h"
#include "cheese-effect-chooser.h"
#include "cheese-fileutil.h"
#include "cheese-gconf.h"
#include "cheese-thumb-view.h"
#include "cheese-window.h"
#include "ephy-spinner.h"
#include "gst-audio-play.h"
#include "gedit-message-area.h"
#include "cheese-no-camera.h"
#include "cheese-prefs-dialog.h"

#define SHUTTER_SOUNDS 5

typedef enum
{
  WEBCAM_MODE_PHOTO,
  WEBCAM_MODE_VIDEO
} WebcamMode;

typedef struct 
{
  gboolean recording;

  /* UDI device requested on the command line */
  char *startup_hal_dev_udi;
  char *video_filename;
  char *photo_filename;

  int counter;

  CheeseWebcam *webcam;
  WebcamMode webcam_mode;
  CheeseGConf *gconf;
  CheeseFileUtil *fileutil;

  GtkWidget *window;
  GtkWidget *notebook;
  GtkWidget *notebook_bar;

  GtkWidget *main_vbox;
  GtkWidget *video_vbox;

  GtkWidget *effect_frame;
  GtkWidget *effect_chooser;
  GtkWidget *throbber_frame;
  GtkWidget *throbber;
  GtkWidget *countdown_frame;
  GtkWidget *countdown;
  GtkWidget *message_area_frame;
  GtkWidget *message_area;

  GtkWidget *button_effects;
  GtkWidget *button_photo;
  GtkWidget *button_video;

  GtkWidget *image_take_photo;
  GtkWidget *label_effects;
  GtkWidget *label_photo;
  GtkWidget *label_take_photo;
  GtkWidget *label_video;

  GtkWidget *thumb_scrollwindow;
  GtkWidget *thumb_view;
  GtkWidget *thumb_view_popup_menu;

  GtkWidget *screen;
  GtkWidget *take_picture;

#ifdef HILDON
   GtkWidget *main_hbox;
   GtkWidget *subwindow;
#endif      

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

  GtkUIManager *ui_manager;

  int audio_play_counter;
  GRand *rand;

} CheeseWindow;


/* Make url in about dialog clickable */
static void
cheese_about_dialog_handle_url (GtkAboutDialog *dialog, const char *url, gpointer data)
{
  GError *error = NULL;
  GtkWidget *error_dialog;
  gboolean ret;

  ret = g_app_info_launch_default_for_uri (url, NULL, &error);
  if (ret == FALSE)
  {
    error_dialog = gtk_message_dialog_new (GTK_WINDOW (dialog), 
                                           GTK_DIALOG_DESTROY_WITH_PARENT, 
                                           GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
                                           _("Failed to open browser to show:\n%s"), url); 
    gtk_dialog_run (GTK_DIALOG (error_dialog));
    gtk_widget_destroy (error_dialog);
    g_error_free(error);
  }
}

/* Make email in about dialog clickable */
static void
cheese_about_dialog_handle_email (GtkAboutDialog *dialog, const char *email, gpointer data)
{
  char *uri;
  GError *error = NULL;
  GtkWidget *error_dialog;
  gboolean ret;

  uri = g_strconcat ("mailto:", email, NULL);

  ret = g_app_info_launch_default_for_uri (uri, NULL, &error);
  if (ret == FALSE)
  {
    error_dialog = gtk_message_dialog_new (GTK_WINDOW (dialog), 
                                           GTK_DIALOG_DESTROY_WITH_PARENT, 
                                           GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
                                           _("Failed to open email client to send message to:\n%s"), email); 
    gtk_dialog_run (GTK_DIALOG (error_dialog));
    gtk_widget_destroy (error_dialog);
    g_error_free(error);
  }
  g_free (uri);
}

static char *
audio_play_get_filename (CheeseWindow *cheese_window)
{
  char *filename;
  if (cheese_window->audio_play_counter > 21)
   filename = g_strdup_printf ("%s/sounds/shutter%i.ogg", PACKAGE_DATADIR,
                               g_rand_int_range (cheese_window->rand, 1, SHUTTER_SOUNDS));
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

static void
cheese_window_photo_saved_cb (CheeseWebcam *webcam, CheeseWindow *cheese_window)
{
  // TODO look at this g_free
  g_free (cheese_window->photo_filename);
  cheese_window->photo_filename = NULL;
  gtk_widget_set_sensitive (cheese_window->take_picture, TRUE);
}

static void
cheese_window_video_saved_cb (CheeseWebcam *webcam, CheeseWindow *cheese_window)
{
  // TODO look at this g_free
  g_free (cheese_window->video_filename);
  cheese_window->video_filename = NULL;
  gtk_action_group_set_sensitive (cheese_window->actions_effects, TRUE);
  gtk_widget_set_sensitive (cheese_window->take_picture, TRUE);
}


static void
cheese_window_cmd_close (GtkWidget *widget, CheeseWindow *cheese_window)
{
  g_free (cheese_window->rand);
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
  g_object_unref (cheese_window->ui_manager);
  g_object_unref (cheese_window->gconf);

  g_free (cheese_window);
  gtk_main_quit ();
}

static void
cheese_window_cmd_open (GtkWidget *widget, CheeseWindow *cheese_window)
{
  char *uri;
  char *filename;
  gboolean ret;
  GError *error = NULL;
  GtkWidget *dialog;

  filename = cheese_thumb_view_get_selected_image (CHEESE_THUMB_VIEW (cheese_window->thumb_view));
  g_return_if_fail (filename);
  uri = g_filename_to_uri (filename, NULL, NULL);
  g_free (filename);
 
  ret = g_app_info_launch_default_for_uri (uri, NULL, &error);
  if (ret == FALSE)
  {
    dialog = gtk_message_dialog_new (GTK_WINDOW (cheese_window->window), 
                                     GTK_DIALOG_DESTROY_WITH_PARENT, 
                                     GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, 
                                     _("Failed to launch program to show:\n%s"), uri);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    g_error_free(error);
  }
  g_free (uri);
}

static void
cheese_window_cmd_save_as (GtkWidget *widget, CheeseWindow *cheese_window)
{
  GtkWidget *dialog;
  int response;
  char *filename;
  char *basename;

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
    char *target_filename;
    GError *error = NULL;
    gboolean ok;

    target_filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    GFile *target = g_file_new_for_path (target_filename);

    GFile *source = g_file_new_for_path (filename);

    ok = g_file_copy (source, target, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &error);

    g_object_unref (source);
    g_object_unref (target);

    if (!ok)
    {
      char *header;
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
    g_free(target_filename);
  }
  g_free(filename);
  gtk_widget_destroy (dialog);
}

static void
cheese_window_cmd_move_file_to_trash (CheeseWindow *cheese_window, GList *files)
{
  GError *error = NULL;
  GList *l;
  gchar *primary, *secondary;
  GtkWidget *error_dialog;

  for (l = files; l != NULL; l = l->next)
  {
    if (!g_file_trash (l->data, NULL, &error))
    {
      primary = g_strdup (_("Cannot move file to trash"));
      secondary = g_strdup_printf (_("The file \"%s\" cannot be moved to the trash. Details: %s"),
                                   g_file_get_basename (l->data), error->message);

      error_dialog = gtk_message_dialog_new (GTK_WINDOW (cheese_window->window),
                                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                             GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", primary);
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (error_dialog),
                                                "%s", secondary);
      gtk_dialog_run (GTK_DIALOG (error_dialog));
      gtk_widget_destroy (error_dialog);

      g_free (primary);
      g_free (secondary);
      g_error_free(error);
      error = NULL;
      /*TODO if we can't move files to trash, maybe we should try to delete them....*/
    }
  }
}

static void
cheese_window_move_all_media_to_trash (GtkWidget *widget, CheeseWindow *cheese_window)
{
  GtkWidget *dlg;
  char *prompt;
  int response;
  char *filename;
  GFile *file;
  GList *files_list = NULL;
  GDir *dir_videos, *dir_photos;
  char *path_videos, *path_photos;
  const char *name;

  prompt = g_strdup_printf (_("Really move all photos and videos to the trash?"));
  dlg = gtk_message_dialog_new_with_markup (GTK_WINDOW (cheese_window->window),
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

  if (response !=  GTK_RESPONSE_OK)
    return;

  //delete all videos
  path_videos = cheese_fileutil_get_video_path (cheese_window->fileutil);
  dir_videos = g_dir_open (path_videos, 0, NULL);
  while ((name = g_dir_read_name (dir_videos)) != NULL)
  {
    if (g_str_has_suffix (name, VIDEO_NAME_SUFFIX))
    {
      filename = g_strjoin (G_DIR_SEPARATOR_S, path_videos, name, NULL);
      file = g_file_new_for_path (filename);

      files_list = g_list_append (files_list, file);
      g_free (filename);
    }
  }
  cheese_window_cmd_move_file_to_trash (cheese_window, files_list);
  g_list_free (files_list);
  g_dir_close (dir_videos);
  
  //delete all photos
  path_photos = cheese_fileutil_get_photo_path (cheese_window->fileutil);
  dir_photos = g_dir_open (path_photos, 0, NULL);
  while ((name = g_dir_read_name (dir_photos)) != NULL)
  {
    if (g_str_has_suffix (name, PHOTO_NAME_SUFFIX))
    {
      filename = g_strjoin (G_DIR_SEPARATOR_S, path_photos, name, NULL);
      file = g_file_new_for_path (filename);

      files_list = g_list_append (files_list, file);
      g_free (filename);
    }
  }
  cheese_window_cmd_move_file_to_trash (cheese_window, files_list);
  g_list_free (files_list);
  g_dir_close (dir_photos);
}

static void
cheese_window_move_media_to_trash (GtkWidget *widget, CheeseWindow *cheese_window)
{
  char *filename;
  GFile *file;
  GList *files_list = NULL;

  filename = cheese_thumb_view_get_selected_image (CHEESE_THUMB_VIEW (cheese_window->thumb_view));
  g_return_if_fail (filename);

  // return if the file does not exist, i.e. was already deleted
  if (!g_file_test (filename, G_FILE_TEST_EXISTS)) 
    return;

  file = g_file_new_for_path (filename);
  g_free (filename);

  files_list = g_list_append (files_list, file);
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
  EContact *contact;
  EBook *book;
  GError *error;
  GdkPixbuf *pixbuf;
  const int MAX_PHOTO_HEIGHT = 150;
  const int MAX_PHOTO_WIDTH = 150;
  char *filename;

  filename = cheese_thumb_view_get_selected_image (CHEESE_THUMB_VIEW (cheese_window->thumb_view));
 
  if (e_book_get_self (&contact, &book, NULL) && filename) 
  {
    char *name = e_contact_get (contact, E_CONTACT_FULL_NAME);
    g_print("Setting Account Photo for %s\n", name);

    pixbuf = gdk_pixbuf_new_from_file_at_scale (filename, MAX_PHOTO_HEIGHT, 
                                                MAX_PHOTO_WIDTH, TRUE, NULL);
    if (contact)
    {
      EContactPhoto photo;
      guchar **data;
      gsize *length;

      photo.type = E_CONTACT_PHOTO_TYPE_INLINED;
      photo.data.inlined.mime_type = "image/jpeg";
      data = &photo.data.inlined.data;
      length = &photo.data.inlined.length;

      gdk_pixbuf_save_to_buffer (pixbuf, (char **) data, length, "png", NULL, 
                                 "compression", "9", NULL); 
      e_contact_set (contact, E_CONTACT_PHOTO, &photo);

      if (!e_book_commit_contact(book, contact, &error)) 
      {
        char *header;
        GtkWidget *dlg;

        header = g_strdup_printf (_("Could not set the Account Photo"));
        dlg = gtk_message_dialog_new (GTK_WINDOW (cheese_window->window),
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
  GError *error = NULL;
  char *command_line;
  const char *action_name;
  char *filename;
  filename = cheese_thumb_view_get_selected_image (CHEESE_THUMB_VIEW (cheese_window->thumb_view));
  g_return_if_fail (filename);

  action_name = gtk_action_get_name (action);
  if (strcmp (action_name, "SendByMail") == 0)
  {
    char *basename = g_path_get_basename (filename);
    command_line = g_strdup_printf ("gnome-open mailto:?subject=%s&attachment=%s",
                                    basename, filename);
    g_free (basename);
  }
  if (strcmp (action_name, "SendTo") == 0)
  {
    command_line = g_strdup_printf ("nautilus-sendto %s", filename);
  }
  else if (strcmp (action_name, "ExportToFSpot") == 0)
  {
    char *dirname =  g_path_get_dirname (filename);
    command_line = g_strdup_printf ("f-spot -i %s", dirname);
    g_free (dirname);
  }
  else if (strcmp (action_name, "ExportToFlickr") == 0)
  {
    command_line = g_strdup_printf ("postr %s", filename);
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
  GError *error = NULL;
  gboolean ret;

  ret = g_app_info_launch_default_for_uri ("ghelp:cheese", NULL, &error);

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

static void
cheese_window_cmd_about (GtkAction *action, CheeseWindow *cheese_window)
{
  static const char *authors[] = {
    "daniel g. siegel <dgsiegel@gmail.com>",
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
                         "copyright", "Copyright \xc2\xa9 2007, 2008\n daniel g. siegel <dgsiegel@gmail.com>",
                         "comments", _("Take photos and videos with your webcam, with fun graphical effects"),
                         "authors", authors,
                         "translator-credits", translators,
                         "website", "http://www.gnome.org/projects/cheese",
                         "website-label", _("Cheese Website"),
                         "logo-icon-name", "cheese",
                         "wrap-license", TRUE,
                         "license", license_trans,
                         NULL);

  g_free (license_trans);
}

static gboolean
cheese_window_button_press_event_cb (GtkWidget *iconview, GdkEventButton *event,
                                     CheeseWindow *cheese_window)
{
  GtkTreePath *path;

  gtk_action_group_set_sensitive (cheese_window->actions_file, TRUE);
  if (event->type == GDK_BUTTON_PRESS || event->type == GDK_2BUTTON_PRESS)
  {
    path = gtk_icon_view_get_path_at_pos (GTK_ICON_VIEW (iconview),
                                          (int) event->x, (int) event->y);
    if (path == NULL)
    {
      gtk_action_group_set_sensitive (cheese_window->actions_file, FALSE);
      return FALSE;
    }

    gtk_icon_view_select_path (GTK_ICON_VIEW (cheese_window->thumb_view), path);

    if (event->type == GDK_BUTTON_PRESS && event->button == 3)
    {
      int button, event_time;

      if (event)
      {
        button = event->button;
        event_time = event->time;
      }
      else
      {
        button = 0;
        event_time = gtk_get_current_event_time ();
      }

      char *selected_file = cheese_thumb_view_get_selected_image (CHEESE_THUMB_VIEW (cheese_window->thumb_view));

      if (g_str_has_suffix (selected_file, VIDEO_NAME_SUFFIX))
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
      g_free (selected_file);
        
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
    gtk_notebook_set_current_page (GTK_NOTEBOOK(cheese_window->notebook), 0);
    gtk_label_set_text_with_mnemonic (GTK_LABEL (cheese_window->label_effects), _("_Effects"));
    gtk_widget_set_sensitive (cheese_window->take_picture, TRUE);
    if (cheese_window->webcam_mode == WEBCAM_MODE_PHOTO)
    {
      gtk_action_group_set_sensitive (cheese_window->actions_photo, TRUE);
    }
    else
    {
      gtk_action_group_set_sensitive (cheese_window->actions_video, TRUE);
    }
    cheese_webcam_set_effect (cheese_window->webcam, 
                              cheese_effect_chooser_get_selection (CHEESE_EFFECT_CHOOSER (cheese_window->effect_chooser)));
    g_object_set (cheese_window->gconf,
                  "gconf_prop_selected_effects",
                  cheese_effect_chooser_get_selection_string (CHEESE_EFFECT_CHOOSER (cheese_window->effect_chooser)),
                  NULL);
  }
  else
  {
    gtk_notebook_set_current_page (GTK_NOTEBOOK (cheese_window->notebook), 1);
    gtk_widget_set_sensitive (GTK_WIDGET (cheese_window->take_picture), FALSE);
    gtk_action_group_set_sensitive (cheese_window->actions_photo, FALSE);
    gtk_action_group_set_sensitive (cheese_window->actions_video, FALSE);
  }
}

static void
cheese_window_photo_video_toggle_buttons_cb (GtkWidget *widget, CheeseWindow *cheese_window)
{
  char *str;
  static gboolean ignore_callback = FALSE;
  GtkAction *photo = NULL;
  GtkAction *video = NULL;
  GList *actions, *tmp;
  
  /* When we call gtk_toggle_button_set_active a "toggle" message is generated
     we ignore that one */
  if (ignore_callback)
  {
    ignore_callback = FALSE;
    return;
  }

  // FIXME: THIS IS CRAP!
  actions = gtk_action_group_list_actions (cheese_window->actions_toggle);
  tmp = actions;
  while (tmp != NULL)
  {
    if (strcmp (gtk_action_get_name (GTK_ACTION (tmp->data)), "Photo") == 0)
      photo = tmp->data;
    else
      video = tmp->data;
    tmp = g_list_next (tmp);
  }

  /* Set ignore_callback because we are call gtk_toggle_button_set_active in the next few lines */
  ignore_callback = TRUE;

  if (widget == cheese_window->button_video)
  {
    if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cheese_window->button_video)))
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cheese_window->button_video), TRUE);
    }
    else
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cheese_window->button_photo), FALSE);
    }
  }
  else if (widget == cheese_window->button_photo)
  {
    if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cheese_window->button_photo)))
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cheese_window->button_photo), TRUE);
    }
    else
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cheese_window->button_video), FALSE);
    }
  }
  else
  {
    g_error ("Unknown toggle button");
  }

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cheese_window->button_video))) 
  {
    cheese_window->webcam_mode = WEBCAM_MODE_VIDEO;

    str = g_strconcat ("<b>", _("_Start recording"), "</b>", NULL);
    gtk_label_set_text_with_mnemonic (GTK_LABEL (cheese_window->label_take_photo), str);
    g_free (str);
    gtk_label_set_use_markup (GTK_LABEL (cheese_window->label_take_photo), TRUE);
    gtk_action_activate (video);
    gtk_action_group_set_sensitive (cheese_window->actions_photo, FALSE);
    gtk_action_group_set_sensitive (cheese_window->actions_video, TRUE);
  }
  else
  {
    cheese_window->webcam_mode = WEBCAM_MODE_PHOTO;

    str = g_strconcat ("<b>", _("_Take a Photo"), "</b>", NULL);
    gtk_label_set_text_with_mnemonic (GTK_LABEL (cheese_window->label_take_photo), str);
    g_free (str);
    gtk_label_set_use_markup (GTK_LABEL (cheese_window->label_take_photo), TRUE);
    gtk_action_activate (photo);
    gtk_action_group_set_sensitive (cheese_window->actions_photo, TRUE);
    gtk_action_group_set_sensitive (cheese_window->actions_video, FALSE);
  }
  g_list_free (actions);
  g_list_free (tmp);

}

void
cheese_window_countdown_hide_cb (gpointer data)
{
  CheeseWindow *cheese_window = (CheeseWindow *) data;
  gtk_notebook_set_current_page(GTK_NOTEBOOK(cheese_window->notebook_bar), 0);
}

void
cheese_window_countdown_picture_cb (gpointer data)
{
  CheeseWindow *cheese_window = (CheeseWindow *) data;
  GError *error = NULL;
  GstAudioPlay *audio_play;
  char *file;

  file = audio_play_get_filename (cheese_window);
  audio_play = gst_audio_play_file (file, &error);
  if (!audio_play) 
  {
    g_warning ("%s", error ? error->message : "Unknown error");
    g_error_free (error);
  }

  g_free (file);

  cheese_window->photo_filename = cheese_fileutil_get_new_media_filename (cheese_window->fileutil, WEBCAM_MODE_PHOTO);
  cheese_webcam_take_photo (cheese_window->webcam, cheese_window->photo_filename);
}

static void
cheese_window_no_camera_message_area_response (GtkWidget *widget, gint response_id, GtkWidget *cheese_window)
{
  GError *error = NULL;
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
      gtk_dialog_run(GTK_DIALOG(d));
      gtk_widget_destroy(d);
      g_error_free(error);
    }
  }
}

static void
cheese_window_stop_recording (CheeseWindow *cheese_window)
{
  if (cheese_window->recording)
  {
    cheese_webcam_stop_video_recording (cheese_window->webcam);
    gtk_action_group_set_sensitive (cheese_window->actions_effects, TRUE);
    gtk_widget_set_sensitive (cheese_window->button_photo, TRUE);
    gtk_widget_set_sensitive (cheese_window->button_video, TRUE);
    gchar * str = g_strconcat ("<b>", _("_Start Recording"), "</b>", NULL);
    gtk_label_set_text_with_mnemonic (GTK_LABEL (cheese_window->label_take_photo), str);
    g_free (str);
    gtk_label_set_use_markup (GTK_LABEL (cheese_window->label_take_photo), TRUE);
    gtk_image_set_from_stock (GTK_IMAGE (cheese_window->image_take_photo), GTK_STOCK_MEDIA_RECORD, GTK_ICON_SIZE_BUTTON);
    
    cheese_window->recording = FALSE;
  }
}

static gboolean
cheese_window_cancel_cb (CheeseWindow *cheese_window,
                         GtkAccelGroup *accel_group,
                         guint keyval, GdkModifierType modifier)
{
  cheese_countdown_cancel ((CheeseCountdown *) cheese_window->countdown);
  
  if (cheese_window->webcam_mode == WEBCAM_MODE_PHOTO)
  {
    gtk_notebook_set_current_page (GTK_NOTEBOOK (cheese_window->notebook_bar), 0);
      
    gtk_widget_set_sensitive (cheese_window->take_picture, TRUE);
  }
  else
  {
    cheese_window_stop_recording (cheese_window);
  }
  return TRUE;
}

static void
cheese_window_action_button_clicked_cb (GtkWidget *widget, CheeseWindow *cheese_window)
{
  char *str;
  

  if (cheese_window->webcam_mode == WEBCAM_MODE_PHOTO)
  {
    gboolean countdown;
    g_object_get (cheese_window->gconf, "gconf_prop_countdown", &countdown, NULL);
    if (countdown)
    {
      cheese_countdown_start((CheeseCountdown *) cheese_window->countdown,
                             cheese_window_countdown_picture_cb,
                             cheese_window_countdown_hide_cb,
                             (gpointer) cheese_window);
      gtk_notebook_set_current_page (GTK_NOTEBOOK(cheese_window->notebook_bar), 1);
    }
    else
    {
      cheese_window_countdown_picture_cb (cheese_window);
    }

    gtk_widget_set_sensitive (cheese_window->take_picture, FALSE);
    // FIXME: set menu inactive
  }
  else if (cheese_window->webcam_mode == WEBCAM_MODE_VIDEO)
  {
    if (!cheese_window->recording)
    {
      gtk_action_group_set_sensitive (cheese_window->actions_effects, FALSE);
      gtk_widget_set_sensitive (cheese_window->button_photo, FALSE);
      gtk_widget_set_sensitive (cheese_window->button_video, FALSE);
      str = g_strconcat ("<b>", _("_Stop Recording"), "</b>", NULL);
      gtk_label_set_text_with_mnemonic (GTK_LABEL (cheese_window->label_take_photo), str);
      g_free (str);
      gtk_label_set_use_markup (GTK_LABEL (cheese_window->label_take_photo), TRUE);
      gtk_image_set_from_stock (GTK_IMAGE (cheese_window->image_take_photo), GTK_STOCK_MEDIA_STOP, GTK_ICON_SIZE_BUTTON);

      cheese_window->video_filename = cheese_fileutil_get_new_media_filename (cheese_window->fileutil, WEBCAM_MODE_VIDEO);
      cheese_webcam_start_video_recording (cheese_window->webcam, cheese_window->video_filename);

      cheese_window->recording = TRUE;
    }
    else
    {
      cheese_window_stop_recording (cheese_window);
    }
  }
}

static void 
cheese_window_preferences_cb (GtkAction *action, CheeseWindow *cheese_window)
{
  cheese_prefs_dialog_run (cheese_window->window, cheese_window->gconf,
                           cheese_window->webcam);
}

static const GtkActionEntry action_entries_main[] = {
  {"Cheese", NULL, N_("_Cheese")},

  {"Edit", NULL, N_("_Edit")},
  {"RemoveAll", NULL, N_("Move All to Trash"), NULL, NULL, G_CALLBACK (cheese_window_move_all_media_to_trash)},

  {"Help", NULL, N_("_Help")},

  {"Quit", GTK_STOCK_QUIT, NULL, NULL, NULL, G_CALLBACK (cheese_window_cmd_close)},
  {"HelpContents", GTK_STOCK_HELP, N_("_Contents"), "F1", N_("Help on this Application"), G_CALLBACK (cheese_window_cmd_help_contents)},
  {"About", GTK_STOCK_ABOUT, NULL, NULL, NULL, G_CALLBACK (cheese_window_cmd_about)},

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

static const GtkRadioActionEntry action_entries_toggle[] = {
  {"Photo", NULL, N_("_Photo"), NULL, NULL, 0},
  {"Video", NULL, N_("_Video"), NULL, NULL, 1},
};

static const GtkActionEntry action_entries_file[] = {
  {"Open", GTK_STOCK_OPEN, N_("_Open"), "<control>O", NULL, G_CALLBACK (cheese_window_cmd_open)},
  {"SaveAs", GTK_STOCK_SAVE_AS, N_("Save _As..."), "<control>S", NULL, G_CALLBACK (cheese_window_cmd_save_as)},
  {"MoveToTrash", "user-trash", N_("Move to _Trash"), "Delete", NULL, G_CALLBACK (cheese_window_move_media_to_trash)},
};

static const GtkActionEntry action_entries_photo[] = {
  {"TakePhoto", NULL, N_("_Take a photo"), "space", NULL, G_CALLBACK (cheese_window_action_button_clicked_cb)},
};

static const GtkToggleActionEntry action_entries_video[] = {
  {"TakeVideo", NULL, N_("_Recording"), "space", NULL, G_CALLBACK (cheese_window_action_button_clicked_cb), FALSE},
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
  if (strcmp (gtk_action_get_name (GTK_ACTION (current)), "Photo") == 0)
  {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cheese_window->button_photo), TRUE);
  }
  else
  {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cheese_window->button_video), TRUE);
  }
}

GtkActionGroup*
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

GtkActionGroup*
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

GtkActionGroup*
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
cheese_window_set_message_area (CheeseWindow *cheese_window,
                                GtkWidget *message_area)
{
  if (cheese_window->message_area == message_area)
    return;

  if (cheese_window->message_area != NULL)
    gtk_widget_destroy (cheese_window->message_area);

  cheese_window->message_area = message_area;

  if (message_area == NULL)
    return;

  gtk_container_add (GTK_CONTAINER (cheese_window->message_area_frame), cheese_window->message_area);
  gtk_widget_show (GTK_WIDGET(cheese_window->message_area));
}

static void
cheese_window_create_window (CheeseWindow *cheese_window)
{
  GError *error=NULL;
  char *path;
  GtkBuilder* builder;

#ifdef HILDON
  HildonProgram *program = hildon_program_get_instance();
  GtkWidget *menu;
  GtkWidget *menuitem;
#else
  GtkWidget *menubar;
#endif

  cheese_window->message_area = NULL;

  builder = gtk_builder_new ();
  gtk_builder_add_from_file (builder, PACKAGE_DATADIR"/cheese.ui", &error);

  if (error)
  {
    g_error ("building ui from %s failed: %s", PACKAGE_DATADIR"/cheese.ui", error->message);
    g_clear_error (&error);
  }

  cheese_window->window             = GTK_WIDGET (gtk_builder_get_object (builder, "cheese_window"));
  cheese_window->button_effects     = GTK_WIDGET (gtk_builder_get_object (builder, "button_effects"));
  cheese_window->button_photo       = GTK_WIDGET (gtk_builder_get_object (builder, "button_photo"));
  cheese_window->button_video       = GTK_WIDGET (gtk_builder_get_object (builder, "button_video"));
  cheese_window->image_take_photo   = GTK_WIDGET (gtk_builder_get_object (builder, "image_take_photo"));
  cheese_window->label_effects      = GTK_WIDGET (gtk_builder_get_object (builder, "label_effects"));
  cheese_window->label_photo        = GTK_WIDGET (gtk_builder_get_object (builder, "label_photo"));
  cheese_window->label_take_photo   = GTK_WIDGET (gtk_builder_get_object (builder, "label_take_photo"));
  cheese_window->label_video        = GTK_WIDGET (gtk_builder_get_object (builder, "label_video"));
  cheese_window->main_vbox          = GTK_WIDGET (gtk_builder_get_object (builder, "main_vbox"));
  cheese_window->video_vbox         = GTK_WIDGET (gtk_builder_get_object (builder, "video_vbox"));
  cheese_window->notebook           = GTK_WIDGET (gtk_builder_get_object (builder, "notebook"));
  cheese_window->notebook_bar       = GTK_WIDGET (gtk_builder_get_object (builder, "notebook_bar"));
  cheese_window->screen             = GTK_WIDGET (gtk_builder_get_object (builder, "video_screen"));
  cheese_window->take_picture       = GTK_WIDGET (gtk_builder_get_object (builder, "take_picture"));
  cheese_window->thumb_scrollwindow = GTK_WIDGET (gtk_builder_get_object (builder, "thumb_scrollwindow"));
  cheese_window->throbber_frame     = GTK_WIDGET (gtk_builder_get_object (builder, "throbber_frame"));
  cheese_window->countdown_frame    = GTK_WIDGET (gtk_builder_get_object (builder, "countdown_frame"));
  cheese_window->effect_frame       = GTK_WIDGET (gtk_builder_get_object (builder, "effect_frame"));
  cheese_window->message_area_frame = GTK_WIDGET (gtk_builder_get_object (builder, "message_area_frame"));

#ifdef HILDON
  /* Reparent widgets in case we use hildon. This saves us maintaining two
     GtkBuilder ui files
  */
  cheese_window->window = hildon_window_new();
  cheese_window->main_hbox = gtk_hbox_new(FALSE, 0);
  hildon_program_add_window (program, HILDON_WINDOW(cheese_window->window));
  gtk_container_add (GTK_CONTAINER(cheese_window->window), cheese_window->main_hbox);  
  gtk_widget_ref(cheese_window->thumb_scrollwindow);
  gtk_widget_ref(cheese_window->video_vbox);
  gtk_container_remove(GTK_CONTAINER(cheese_window->video_vbox), cheese_window->thumb_scrollwindow);
  gtk_container_remove(GTK_CONTAINER(cheese_window->main_vbox),cheese_window->video_vbox);
  gtk_box_pack_start (GTK_BOX(cheese_window->main_hbox), cheese_window->video_vbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX(cheese_window->main_hbox), GTK_WIDGET(cheese_window->thumb_scrollwindow), FALSE, FALSE, 0);
  gtk_widget_destroy(cheese_window->main_vbox);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(cheese_window->thumb_scrollwindow), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_unref(cheese_window->thumb_scrollwindow);
  gtk_widget_unref(cheese_window->video_vbox);
#endif

  g_object_unref (builder);

  char *str = g_strconcat ("<b>", _("_Take a photo"), "</b>", NULL);
  gtk_label_set_text_with_mnemonic (GTK_LABEL (cheese_window->label_take_photo), str);
  g_free (str);
  gtk_label_set_use_markup (GTK_LABEL (cheese_window->label_take_photo), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (cheese_window->take_picture), FALSE);

  cheese_window->thumb_view = cheese_thumb_view_new ();
  gtk_container_add (GTK_CONTAINER (cheese_window->thumb_scrollwindow), cheese_window->thumb_view);

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

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cheese_window->button_photo), TRUE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cheese_window->button_video), FALSE);

  g_signal_connect (cheese_window->button_photo, "toggled",
                    G_CALLBACK (cheese_window_photo_video_toggle_buttons_cb), cheese_window);
  g_signal_connect (cheese_window->button_video, "toggled",
                    G_CALLBACK (cheese_window_photo_video_toggle_buttons_cb), cheese_window);

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
    path = g_find_program_in_path ("gnome-open");
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

  gtk_ui_manager_add_ui_from_file (cheese_window->ui_manager, PACKAGE_DATADIR"/cheese-ui.xml", &error);

  if (error)
  {
    g_critical ("building menus from %s failed: %s", PACKAGE_DATADIR"/cheese-ui.xml", error->message);
    g_error_free (error);
  }

  GtkAction *action = gtk_ui_manager_get_action (cheese_window->ui_manager, "/MainMenu/Cheese/CountdownToggle");
  gboolean countdown;
  g_object_get (cheese_window->gconf, "gconf_prop_countdown", &countdown, NULL);
  if (countdown)
  {
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), TRUE);
  }

#ifdef HILDON
  menu = gtk_menu_new();
  menuitem = gtk_menu_item_new_with_label(_("Quit"));
  g_signal_connect(menuitem,"activate",
                   GTK_SIGNAL_FUNC(cheese_window_cmd_close),
                   cheese_window);
  gtk_menu_append(menu, menuitem);
  
  menuitem = gtk_menu_item_new_with_label(_("About"));
  g_signal_connect(menuitem,"activate",
                   GTK_SIGNAL_FUNC(cheese_window_cmd_about), 
                   cheese_window);
  gtk_menu_append(menu, menuitem);
  
  hildon_window_set_menu (HILDON_WINDOW(cheese_window->window),GTK_MENU(menu));
#else
  menubar = gtk_ui_manager_get_widget (cheese_window->ui_manager, "/MainMenu");
  gtk_box_pack_start (GTK_BOX (cheese_window->main_vbox), menubar, FALSE, FALSE, 0);
#endif

  cheese_window->thumb_view_popup_menu = gtk_ui_manager_get_widget (cheese_window->ui_manager, 
                                                                    "/ThumbnailPopup");

  gtk_window_add_accel_group (GTK_WINDOW (cheese_window->window), 
                              gtk_ui_manager_get_accel_group (cheese_window->ui_manager));
  gtk_accel_group_connect (gtk_ui_manager_get_accel_group (cheese_window->ui_manager),
                           GDK_Escape, 0, 0,
                           g_cclosure_new_swap (G_CALLBACK (cheese_window_cancel_cb),
                                                cheese_window, NULL));

  gtk_action_group_set_sensitive (cheese_window->actions_file, FALSE);

  action = gtk_ui_manager_get_action (cheese_window->ui_manager, "/MainMenu/Edit/Effects");
  gtk_action_connect_proxy (GTK_ACTION(action), GTK_WIDGET(cheese_window->button_effects));

  /* Default handlers for closing the application */
  g_signal_connect (cheese_window->window, "destroy",
                    G_CALLBACK (cheese_window_cmd_close), cheese_window);
  g_signal_connect (cheese_window->window, "delete_event", 
                    G_CALLBACK(cheese_window_delete_event_cb), NULL);

  g_signal_connect (cheese_window->take_picture, "clicked",
                    G_CALLBACK (cheese_window_action_button_clicked_cb), cheese_window);
  g_signal_connect (cheese_window->thumb_view, "button_press_event",
                    G_CALLBACK (cheese_window_button_press_event_cb), cheese_window);
}

void 
setup_camera (CheeseWindow *cheese_window)
{
  char *webcam_device = NULL;
  int x_resolution;
  int y_resolution;
  GtkWidget *message_area;
  
  GError *error;
  g_object_get (cheese_window->gconf, "gconf_prop_x_resolution", &x_resolution,
                "gconf_prop_y_resolution", &y_resolution, "gconf_prop_webcam", &webcam_device, NULL);

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

    gdk_threads_enter ();

    dialog = gtk_message_dialog_new (NULL,
                                     0,
                                     GTK_MESSAGE_ERROR, 
                                     GTK_BUTTONS_OK, 
                                     "%s", error->message);

    g_error_free(error);

    g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);
    gtk_window_set_title(GTK_WINDOW (dialog), "Critical Error");
    gtk_dialog_run (GTK_DIALOG (dialog));

    // Clean up and exit
    cheese_window_cmd_close(NULL, cheese_window);

    gdk_threads_leave ();
  }

  g_signal_connect (cheese_window->webcam, "photo-saved",
                    G_CALLBACK (cheese_window_photo_saved_cb), cheese_window);
  g_signal_connect (cheese_window->webcam, "video-saved",
                    G_CALLBACK (cheese_window_video_saved_cb), cheese_window);

  cheese_webcam_set_effect (cheese_window->webcam, 
                            cheese_effect_chooser_get_selection (CHEESE_EFFECT_CHOOSER (cheese_window->effect_chooser)));

  cheese_webcam_play (cheese_window->webcam);
  gdk_threads_enter ();
  gtk_notebook_set_current_page (GTK_NOTEBOOK(cheese_window->notebook), 0);
  ephy_spinner_stop (EPHY_SPINNER (cheese_window->throbber));
  if (cheese_webcam_get_num_webcam_devices (cheese_window->webcam) == 0)
  {
    message_area = cheese_no_camera_message_area();

    g_signal_connect (message_area,
                      "response",
                      G_CALLBACK (cheese_window_no_camera_message_area_response),
                      cheese_window);

    cheese_window_set_message_area (cheese_window, message_area);
  }
  gtk_widget_set_sensitive (GTK_WIDGET (cheese_window->take_picture), TRUE);
  gdk_threads_leave ();
}

void
cheese_window_init (char *hal_dev_udi)
{
  CheeseWindow *cheese_window;

  cheese_window = g_new0 (CheeseWindow, 1);

  cheese_window->startup_hal_dev_udi = hal_dev_udi;
  cheese_window->gconf = cheese_gconf_new ();
  cheese_window->audio_play_counter = 0;
  cheese_window->rand = g_rand_new ();
  cheese_window->fileutil = cheese_fileutil_new ();

  cheese_window_create_window (cheese_window);
 
  gtk_widget_show_all (cheese_window->window);
  ephy_spinner_start (EPHY_SPINNER (cheese_window->throbber));

  gtk_notebook_set_current_page (GTK_NOTEBOOK(cheese_window->notebook), 2);

  cheese_window->webcam_mode = WEBCAM_MODE_PHOTO;
  cheese_window->recording = FALSE;
  
  /* Run cam setup in its own thread */
  GError *error = NULL;
  if (!g_thread_create ((GThreadFunc) setup_camera, cheese_window, FALSE, &error))
  {
    g_error ("Failed to create setup thread: %s\n", error->message);
    g_error_free (error);
    return;
  }

  /* Make URLs and email clickable in about dialog */
  gtk_about_dialog_set_url_hook (cheese_about_dialog_handle_url, NULL, NULL);
  gtk_about_dialog_set_email_hook (cheese_about_dialog_handle_email, NULL, NULL);
}
