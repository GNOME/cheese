/*
 * Copyright (C) 2007 daniel g. siegel <dgsiegel@gmail.com>
 * Copyright (C) 2007 Jaap Haitsma <jaap@haitsma.org>
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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <glib/gi18n.h>

#include <gst/interfaces/xoverlay.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libebook/e-book.h>
#include <glade/glade.h>

#include "cheese-effect-chooser.h"
#include "cheese-fileutil.h"
#include "cheese-gconf.h"
#include "cheese-thumb-view.h"
#include "cheese-window.h"

#define GLADE_FILE CHEESE_DATA_DIR"/cheese.glade"
#define UI_FILE CHEESE_DATA_DIR"/cheese-ui.xml"

typedef enum
{
  WEBCAM_MODE_PHOTO,
  WEBCAM_MODE_VIDEO
} WebcamMode;


typedef struct 
{
  gboolean recording;

  char *video_filename;
  char *photo_filename;

  CheeseWebcam *webcam;
  WebcamMode webcam_mode;
  CheeseGConf *gconf;

  GtkWidget *window;
  GtkWidget *notebook;

  GtkWidget *main_vbox;

  GtkWidget *effect_frame;
  GtkWidget *effect_chooser;

  GtkWidget *button_effects;
// TODO: button_video and button_photo are toggle buttons now. Still have to 
// implement that behavior correctly
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

  GtkActionGroup *actions_main;
  GtkActionGroup *actions_file;
  GtkActionGroup *actions_photo;
  GtkActionGroup *actions_account_photo;
  GtkActionGroup *actions_mail;
  GtkActionGroup *actions_fspot;
  GtkActionGroup *actions_flickr;

  GtkUIManager *ui_manager;

} CheeseWindow;

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
  gtk_widget_set_sensitive (cheese_window->button_effects, TRUE);
  gtk_widget_set_sensitive (cheese_window->take_picture, TRUE);
}

static void
cheese_window_cmd_close (GtkWidget *widget, CheeseWindow *cheese_window)
{
  g_object_unref (cheese_window->webcam);
  g_object_unref (cheese_window->actions_main);
  g_object_unref (cheese_window->actions_photo);
  g_object_unref (cheese_window->ui_manager);
  g_free (cheese_window);
  gtk_main_quit ();
}

static void
cheese_window_cmd_open (GtkWidget *widget, CheeseWindow *cheese_window)
{
  char *uri;
  char *filename;

  filename = cheese_thumb_view_get_selected_image (CHEESE_THUMB_VIEW (cheese_window->thumb_view));
  g_return_if_fail (filename);

  uri = gnome_vfs_get_uri_from_local_path (filename);
  g_free (filename);
  gnome_vfs_url_show (uri);
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

  dialog = gtk_file_chooser_dialog_new ("Save File",
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

    target_filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

    char *source_uri = g_filename_to_uri (filename, NULL, NULL);
    GnomeVFSURI *source = gnome_vfs_uri_new (source_uri);
    g_free (source_uri);

    char *target_uri = g_filename_to_uri (target_filename, NULL, NULL);
    GnomeVFSURI *target = gnome_vfs_uri_new (target_uri);
    g_free (target_uri);

    response = gnome_vfs_xfer_uri (source, target,
                                   GNOME_VFS_XFER_DEFAULT | GNOME_VFS_XFER_FOLLOW_LINKS,
                                   GNOME_VFS_XFER_ERROR_MODE_ABORT,
                                   GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE,
                                   NULL, NULL);
    gnome_vfs_uri_unref (source);
    gnome_vfs_uri_unref (target);

    if (response != GNOME_VFS_OK)
    {
      char *header;
      GtkWidget *dlg;

      header = g_strdup_printf (_("Could not save %s"), target_filename);

      dlg = gtk_message_dialog_new (GTK_WINDOW (cheese_window->window),
                                    GTK_DIALOG_MODAL |
          			    GTK_DIALOG_DESTROY_WITH_PARENT,
          			    GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
          			    header);
      gtk_dialog_run (GTK_DIALOG (dlg));
      gtk_widget_destroy (dlg);
      g_free (header);
    }
  }
  gtk_widget_destroy (dialog);
}

static int
cheese_window_move_to_trash_confirm_dialog (CheeseWindow *cheese_window, char *filename)
{
  GtkWidget *dlg;
  char *prompt;
  int response;
  char *basename;

  basename = g_path_get_basename (filename);
  prompt = g_strdup_printf (_("Are you sure you want to move\n\"%s\" to the trash?"), basename);
  g_free (basename);
  dlg = gtk_message_dialog_new_with_markup (GTK_WINDOW (cheese_window->window),
                                            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
                                            "<span weight=\"bold\" size=\"larger\">%s</span>",
                                            prompt);
  g_free (prompt);
  gtk_dialog_add_button (GTK_DIALOG (dlg), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
  gtk_dialog_add_button (GTK_DIALOG (dlg), _("Move to Trash"), GTK_RESPONSE_OK);
  gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
  gtk_window_set_title (GTK_WINDOW (dlg), "");
  gtk_widget_show_all (dlg);

  response = gtk_dialog_run (GTK_DIALOG (dlg));
  gtk_widget_destroy (dlg);

  return response;
}


static void
cheese_window_cmd_move_to_trash (GtkWidget *widget, CheeseWindow *cheese_window)
{
  GnomeVFSURI *uri;
  GnomeVFSURI *trash_dir;
  GnomeVFSURI *trash_uri;
  int result;
  char *name;
  GError *error = NULL;
  char *filename;

  // FIXME?: if you delete a file, and then delete again with the shortcut
  // "delete", g_return_if_fail is called, as that file is no more
  filename = cheese_thumb_view_get_selected_image (CHEESE_THUMB_VIEW (cheese_window->thumb_view));
  g_return_if_fail (filename);
  
  result = cheese_window_move_to_trash_confirm_dialog (cheese_window, filename);
  if (result != GTK_RESPONSE_OK)
    return;

  uri = gnome_vfs_uri_new (g_filename_to_uri (filename, NULL, NULL));
  result = gnome_vfs_find_directory (uri, GNOME_VFS_DIRECTORY_KIND_TRASH,
                                     &trash_dir, FALSE, FALSE, 0777);
  if (result != GNOME_VFS_OK)
  {
    gnome_vfs_uri_unref (uri);
    char *header;
    GtkWidget *dlg;

    header = g_strdup_printf (_("Could not find the Trash"));
    dlg = gtk_message_dialog_new (GTK_WINDOW (cheese_window->window),
                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, header);
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), error->message);
    gtk_dialog_run (GTK_DIALOG (dlg));
    gtk_widget_destroy (dlg);
    g_free (header);
    return;
  }

  name = gnome_vfs_uri_extract_short_name (uri);
  trash_uri = gnome_vfs_uri_append_file_name (trash_dir, name);
  g_free (name);

  result = gnome_vfs_move_uri (uri, trash_uri, TRUE);

  gnome_vfs_uri_unref (uri);
  gnome_vfs_uri_unref (trash_uri);
  gnome_vfs_uri_unref (trash_dir);

  if (result != GNOME_VFS_OK)
  {
    char *header;
    GtkWidget *dlg;
    char *basename = g_path_get_basename (filename);

    header = g_strdup_printf (_("Error on deleting %s"), basename);
    g_free (basename);
    dlg = gtk_message_dialog_new (GTK_WINDOW (cheese_window->window),
                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, header);
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), error->message);
    gtk_dialog_run (GTK_DIALOG (dlg));
    gtk_widget_destroy (dlg);
    g_free (header);
  }
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
      char **data;
      gsize *length;

      photo.type = E_CONTACT_PHOTO_TYPE_INLINED;
      photo.data.inlined.mime_type = "image/jpeg";
      data = (char **)&photo.data.inlined.data;
      length = &photo.data.inlined.length;

      gdk_pixbuf_save_to_buffer (pixbuf, data, length, "png", NULL, 
                                 "compression", "9", NULL); 
      e_contact_set (contact, E_CONTACT_PHOTO, &photo);

      if (!e_book_commit_contact(book, contact, &error)) 
      {
        char *header;
        GtkWidget *dlg;

        header = g_strdup_printf (_("Could not set the Account Photo"));
        dlg = gtk_message_dialog_new (GTK_WINDOW (cheese_window->window),
                                      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                      GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, header);
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), error->message);
        gtk_dialog_run (GTK_DIALOG (dlg));
        gtk_widget_destroy (dlg);
        g_free (header);
      }
      g_free (*data);
    }
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

  license_trans = g_strconcat (_(license[0]), "\n", _(license[1]), "\n",
      _(license[2]), "\n", NULL);

  gtk_show_about_dialog (GTK_WINDOW (cheese_window->window),
                         "version", CHEESE_VERSION,
                         "copyright", "Copyright \xc2\xa9 2007\n daniel g. siegel <dgsiegel@gmail.com>",
                         "comments", _("A cheesy program to take pictures and videos from your webcam"),
                         "authors", authors,
                         "translator-credits", translators,
                         "website", "http://live.gnome.org/Cheese",
                         "logo-icon-name", "cheese",
                         "wrap-license", TRUE,
                         "license", license_trans,
                         NULL);

  g_free (license_trans);
}

/*
static gboolean
cheese_window_key_press_event_cb (GtkWidget *widget, GdkEventKey *event, 
                                  CheeseWindow *cheese_window)
{
  if (event->type == GDK_KEY_PRESS)
  {
    switch (event->keyval) {
      case GDK_space:
        g_signal_emit_by_name (cheese_window->take_picture, "clicked");
        return TRUE;
    }
  }
  return FALSE;
}
*/

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
    cheese_webcam_set_effect (cheese_window->webcam, 
                              cheese_effect_chooser_get_selection (CHEESE_EFFECT_CHOOSER (cheese_window->effect_chooser)));
  }
  else
  {
    gtk_notebook_set_current_page (GTK_NOTEBOOK (cheese_window->notebook), 1);
    gtk_label_set_text_with_mnemonic (GTK_LABEL (cheese_window->label_effects), _("_Back"));
    gtk_widget_set_sensitive (GTK_WIDGET (cheese_window->take_picture), FALSE);
  }
}

static void
cheese_window_button_video_cb (GtkWidget *widget, CheeseWindow *cheese_window)
{
  char *str;

  cheese_window->webcam_mode = WEBCAM_MODE_VIDEO;

  gtk_widget_set_sensitive (cheese_window->button_photo, TRUE);
  gtk_widget_set_sensitive (cheese_window->button_video, FALSE);
  str = g_strconcat ("<b>", _("_Start recording"), "</b>", NULL);
  gtk_label_set_text_with_mnemonic (GTK_LABEL (cheese_window->label_take_photo), str);
  g_free (str);
  gtk_label_set_use_markup (GTK_LABEL (cheese_window->label_take_photo), TRUE);
  gtk_action_group_set_sensitive (cheese_window->actions_photo, FALSE);
}

static void
cheese_window_button_photo_cb (GtkWidget *widget, CheeseWindow *cheese_window)
{
  char *str;

  cheese_window->webcam_mode = WEBCAM_MODE_PHOTO;

  gtk_widget_set_sensitive (cheese_window->button_photo, FALSE);
  gtk_widget_set_sensitive (cheese_window->button_video, TRUE);
  str = g_strconcat ("<b>", _("_Take a photo"), "</b>", NULL);
  gtk_label_set_text_with_mnemonic (GTK_LABEL (cheese_window->label_take_photo), str);
  g_free (str);
  gtk_label_set_use_markup (GTK_LABEL (cheese_window->label_take_photo), TRUE);
  gtk_action_group_set_sensitive (cheese_window->actions_photo, TRUE);
}

static void
cheese_window_action_button_clicked_cb (GtkWidget *widget, CheeseWindow *cheese_window)
{
  char *str;

  if (cheese_window->webcam_mode == WEBCAM_MODE_PHOTO)
  {
    gtk_widget_set_sensitive (cheese_window->button_photo, FALSE);
    gtk_widget_set_sensitive (cheese_window->take_picture, FALSE);

    cheese_window->photo_filename = cheese_fileutil_get_new_media_filename (WEBCAM_MODE_PHOTO);
    cheese_webcam_take_photo (cheese_window->webcam, cheese_window->photo_filename);
  }
  else if (cheese_window->webcam_mode == WEBCAM_MODE_VIDEO)
  {
    if (!cheese_window->recording)
    {
      gtk_widget_set_sensitive (cheese_window->button_effects, FALSE);
      gtk_widget_set_sensitive (cheese_window->button_photo, FALSE);
      str = g_strconcat ("<b>", _("_Stop recording"), "</b>", NULL);
      gtk_label_set_text_with_mnemonic (GTK_LABEL (cheese_window->label_take_photo), str);
      g_free (str);
      gtk_label_set_use_markup (GTK_LABEL (cheese_window->label_take_photo), TRUE);
      gtk_image_set_from_stock (GTK_IMAGE (cheese_window->image_take_photo), GTK_STOCK_MEDIA_STOP, GTK_ICON_SIZE_BUTTON);

      cheese_window->video_filename = cheese_fileutil_get_new_media_filename (WEBCAM_MODE_VIDEO);
      cheese_webcam_start_video_recording (cheese_window->webcam, cheese_window->video_filename);
    }
    else
    {
      cheese_webcam_stop_video_recording (cheese_window->webcam);
      gtk_widget_set_sensitive (cheese_window->take_picture, FALSE);
      str = g_strconcat ("<b>", _("_Start recording"), "</b>", NULL);
      gtk_label_set_text_with_mnemonic (GTK_LABEL (cheese_window->label_take_photo), str);
      g_free (str);
      gtk_label_set_use_markup (GTK_LABEL (cheese_window->label_take_photo), TRUE);
      gtk_image_set_from_stock (GTK_IMAGE (cheese_window->image_take_photo), GTK_STOCK_MEDIA_RECORD, GTK_ICON_SIZE_BUTTON);
    }
    cheese_window->recording = !cheese_window->recording;
  }
}

static const GtkActionEntry action_entries_main[] = {
  {"Cheese", NULL, N_("_Cheese")},
  {"Edit", NULL, N_("_Edit")},
  {"Help", NULL, N_("_Help")},

  {"Quit", GTK_STOCK_QUIT, NULL, NULL, NULL, G_CALLBACK (cheese_window_cmd_close)},
  {"About", GTK_STOCK_ABOUT, NULL, NULL, NULL, G_CALLBACK (cheese_window_cmd_about)},
};

static const GtkActionEntry action_entries_edit_file[] = {
  {"Open", GTK_STOCK_OPEN, N_("_Open"), "<control>O", NULL, G_CALLBACK (cheese_window_cmd_open)},
  {"SaveAs", GTK_STOCK_SAVE_AS, N_("Save _As..."), "<control>S", NULL, G_CALLBACK (cheese_window_cmd_save_as)},
  {"MoveToTrash", "user-trash", N_("Move to _Trash"), "Delete", NULL, G_CALLBACK (cheese_window_cmd_move_to_trash)},
};

static const GtkActionEntry action_entries_photo[] = {
  {"TakePhoto", NULL, N_("_Take a photo"), "space", NULL, G_CALLBACK (cheese_window_action_button_clicked_cb)},
};

static const GtkActionEntry action_entries_account_photo[] = {
  {"SetAsAccountPhoto", NULL, N_("_Set As Account Photo"), NULL, NULL, G_CALLBACK (cheese_window_cmd_set_about_me_photo)},
};

static const GtkActionEntry action_entries_mail[] = {
  {"SendByMail", NULL, N_("Send by _Mail"), NULL, NULL, G_CALLBACK (cheese_window_cmd_command_line)},
};

static const GtkActionEntry action_entries_fspot[] = {
  {"ExportToFSpot", NULL, N_("Export to F-_Spot"), NULL, NULL, G_CALLBACK (cheese_window_cmd_command_line)},
};

static const GtkActionEntry action_entries_flickr[] = {
  {"ExportToFlickr", NULL, N_("Export to _Flickr"), NULL, NULL, G_CALLBACK (cheese_window_cmd_command_line)},
};


GtkActionGroup*
cheese_window_action_group_new (CheeseWindow *cheese_window, char *name, 
                                const GtkActionEntry *action_entries, int num_action_entries)
{
  GtkActionGroup *action_group;

  action_group = gtk_action_group_new (name);
  gtk_action_group_add_actions (action_group, action_entries,
                                num_action_entries, cheese_window);
  gtk_ui_manager_insert_action_group (cheese_window->ui_manager, action_group, 0);

  return action_group;
}

static void
cheese_window_create_window (CheeseWindow *cheese_window)
{
  GladeXML *gxml;
  GtkWidget *menubar;
  GError *error=NULL;
  char *path;

  gxml = glade_xml_new (GLADE_FILE, NULL, NULL);
  cheese_window->window              = glade_xml_get_widget (gxml, "cheese_window");
  cheese_window->button_effects      = glade_xml_get_widget (gxml, "button_effects");
  cheese_window->button_photo        = glade_xml_get_widget (gxml, "button_photo");
  cheese_window->button_video        = glade_xml_get_widget (gxml, "button_video");
  cheese_window->image_take_photo    = glade_xml_get_widget (gxml, "image_take_photo");
  cheese_window->label_effects       = glade_xml_get_widget (gxml, "label_effects");
  cheese_window->label_photo         = glade_xml_get_widget (gxml, "label_photo");
  cheese_window->label_take_photo    = glade_xml_get_widget (gxml, "label_take_photo");
  cheese_window->label_video         = glade_xml_get_widget (gxml, "label_video");
  cheese_window->main_vbox           = glade_xml_get_widget (gxml, "main_vbox");
  cheese_window->notebook            = glade_xml_get_widget (gxml, "notebook");
  cheese_window->screen              = glade_xml_get_widget (gxml, "video_screen");
  cheese_window->take_picture        = glade_xml_get_widget (gxml, "take_picture");

  cheese_window->thumb_scrollwindow  = glade_xml_get_widget (gxml, "thumb_scrollwindow");
  cheese_window->thumb_view          = cheese_thumb_view_new ();
  gtk_container_add (GTK_CONTAINER (cheese_window->thumb_scrollwindow), cheese_window->thumb_view);

  cheese_window->effect_frame        = glade_xml_get_widget (gxml, "effect_frame");
  cheese_window->effect_chooser      = cheese_effect_chooser_new ();
  gtk_container_add (GTK_CONTAINER (cheese_window->effect_frame), cheese_window->effect_chooser);

  gtk_widget_set_sensitive (cheese_window->button_photo, FALSE);
  gtk_widget_set_sensitive (cheese_window->button_video, TRUE);

  g_signal_connect (cheese_window->button_photo, "clicked",
                    G_CALLBACK (cheese_window_button_photo_cb), cheese_window);
  g_signal_connect (cheese_window->button_video, "clicked",
                    G_CALLBACK (cheese_window_button_video_cb), cheese_window);

  cheese_window->ui_manager = gtk_ui_manager_new ();

  cheese_window->actions_main = cheese_window_action_group_new (cheese_window, 
                                                                "ActionsMain", 
                                                                action_entries_main, 
                                                                G_N_ELEMENTS (action_entries_main));
  cheese_window->actions_file = cheese_window_action_group_new (cheese_window, 
                                                                "ActionsMain", 
                                                                action_entries_edit_file, 
                                                                G_N_ELEMENTS (action_entries_edit_file));
  cheese_window->actions_photo = cheese_window_action_group_new (cheese_window, 
                                                                 "ActionsPhoto", 
                                                                 action_entries_photo, 
                                                                 G_N_ELEMENTS (action_entries_photo));
  cheese_window->actions_account_photo = cheese_window_action_group_new (cheese_window, 
                                                                         "ActionsAccountPhoto", 
                                                                         action_entries_account_photo, 
                                                                         G_N_ELEMENTS (action_entries_account_photo));
  cheese_window->actions_mail = cheese_window_action_group_new (cheese_window, 
                                                                "ActionsMail", 
                                                                action_entries_mail, 
                                                                G_N_ELEMENTS (action_entries_mail));
  path = g_find_program_in_path ("gnome-open");
  gtk_action_group_set_visible (cheese_window->actions_mail, path != NULL);
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

  gtk_ui_manager_add_ui_from_file (cheese_window->ui_manager, UI_FILE, &error);

  if (error)
  {
    g_critical ("building menus from %s failed: %s", UI_FILE, error->message);
    g_error_free (error);
  }

  menubar = gtk_ui_manager_get_widget (cheese_window->ui_manager, "/MainMenu");
  gtk_box_pack_start (GTK_BOX (cheese_window->main_vbox), menubar, FALSE, FALSE, 0);

  cheese_window->thumb_view_popup_menu = gtk_ui_manager_get_widget (cheese_window->ui_manager, 
                                                                    "/ThumbnailPopup");

  gtk_window_add_accel_group (GTK_WINDOW (cheese_window->window), 
		              gtk_ui_manager_get_accel_group (cheese_window->ui_manager));
  gtk_action_group_set_sensitive (cheese_window->actions_file, FALSE);

  /* Default handlers for closing the application */
  g_signal_connect (cheese_window->window, "destroy",
                    G_CALLBACK (cheese_window_cmd_close), cheese_window);
  g_signal_connect(cheese_window->window, "delete_event", 
                   G_CALLBACK(cheese_window_delete_event_cb), NULL);

  g_signal_connect (cheese_window->take_picture, "clicked",
                    G_CALLBACK (cheese_window_action_button_clicked_cb), cheese_window);

  g_signal_connect (cheese_window->button_effects,
                    "clicked", G_CALLBACK (cheese_window_effect_button_pressed_cb), cheese_window);

  //g_signal_connect_after (cheese_window->window, "key_press_event",
  //                        G_CALLBACK (cheese_window_key_press_event_cb), cheese_window);
  // TODO: look at button press event
  g_signal_connect (cheese_window->thumb_view, "button_press_event",
                          G_CALLBACK (cheese_window_button_press_event_cb), cheese_window);
}

void
cheese_window_init ()
{
  CheeseWindow *cheese_window;

  cheese_window = g_new (CheeseWindow, 1);
  cheese_window_create_window (cheese_window);

  cheese_window->gconf = cheese_gconf_new ();

  cheese_window->webcam_mode = WEBCAM_MODE_PHOTO;
  cheese_window->recording = FALSE;
  cheese_window->webcam = cheese_webcam_new (cheese_window->screen);

  g_signal_connect (cheese_window->webcam, "photo-saved",
                    G_CALLBACK (cheese_window_photo_saved_cb), cheese_window);
  g_signal_connect (cheese_window->webcam, "video-saved",
                    G_CALLBACK (cheese_window_video_saved_cb), cheese_window);

  cheese_webcam_play (cheese_window->webcam);
  gtk_widget_show_all (cheese_window->window);
}

