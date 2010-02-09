/*
 * Copyright © 2010 Filippo Argiolas <filippo.argiolas@gmail.com>
 * Copyright © 2007,2008 daniel g. siegel <dgsiegel@gnome.org>
 * Copyright © 2007,2008 Jaap Haitsma <jaap@haitsma.org>
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

#include "cheese-commands.h"

typedef enum
{
  CHEESE_RESPONSE_SKIP,
  CHEESE_RESPONSE_SKIP_ALL,
  CHEESE_RESPONSE_DELETE_ALL
} CheeseDeleteResponseType;

#define CHEESE_BUTTON_SKIP       _("_Skip")
#define CHEESE_BUTTON_SKIP_ALL   _("S_kip All")
#define CHEESE_BUTTON_DELETE_ALL _("Delete _All")

void
cheese_cmd_file_open (GtkWidget *widget, CheeseWindow *cheese_window)
{
  char      *uri;
  char      *filename;
  GError    *error = NULL;
  GtkWidget *dialog;
  GdkScreen *screen;

  filename = cheese_thumb_view_get_selected_image (cheese_window_get_thumbview (cheese_window));
  g_return_if_fail (filename);
  uri = g_filename_to_uri (filename, NULL, NULL);
  g_free (filename);

  screen = gtk_widget_get_screen (GTK_WIDGET (cheese_window));
  gtk_show_uri (screen, uri, gtk_get_current_event_time (), &error);

  if (error != NULL)
  {
    dialog = gtk_message_dialog_new (GTK_WINDOW (cheese_window),
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

void
cheese_cmd_file_save_as (GtkWidget *widget, CheeseWindow *cheese_window)
{
  GtkWidget *dialog;
  int        response;
  char      *filename;
  char      *basename;

  filename = cheese_thumb_view_get_selected_image (cheese_window_get_thumbview (cheese_window));
  g_return_if_fail (filename);

  dialog = gtk_file_chooser_dialog_new (_("Save File"),
                                        GTK_WINDOW (cheese_window),
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

      dlg = gtk_message_dialog_new (GTK_WINDOW (cheese_window),
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
_delete_error_dialog (CheeseWindow *cheese_window, GFile *file, gchar *message)
{
  gchar     *primary, *secondary;
  GtkWidget *error_dialog;

  primary   = g_strdup (_("Error while deleting"));
  secondary = g_strdup_printf (_("The file \"%s\" cannot be deleted. Details: %s"),
                               g_file_get_basename (file), message);
  error_dialog = gtk_message_dialog_new (GTK_WINDOW (cheese_window),
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
_really_delete (CheeseWindow *cheese_window, GList *files, gboolean batch)
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
    question_dialog = gtk_message_dialog_new (GTK_WINDOW (cheese_window),
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
      _delete_error_dialog (cheese_window, l->data,
                            error != NULL ? error->message : _("Unknown Error"));
      g_error_free (error);
      error = NULL;
    }
    g_object_unref (l->data);
  }
}

static void
_really_move_to_trash (CheeseWindow *cheese_window, GList *files)
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
      question_dialog = gtk_message_dialog_new (GTK_WINDOW (cheese_window),
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
          _really_delete (cheese_window, l, TRUE);
          return;

        case GTK_RESPONSE_ACCEPT:

          /* create a single file list for cmd_delete */
          d = g_list_append (d, g_object_ref (l->data));
          _really_delete (cheese_window, d, TRUE);
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
      cheese_thumb_view_remove_item (cheese_window_get_thumbview (cheese_window), l->data);
    }
    g_object_unref (l->data);
  }
}

void
cheese_cmd_file_move_all_to_trash (GtkWidget *widget, CheeseWindow *cheese_window)
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
  dlg    = gtk_message_dialog_new_with_markup (GTK_WINDOW (cheese_window),
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
  path_videos = cheese_fileutil_get_video_path (cheese_window_get_fileutil (cheese_window));
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
  path_photos = cheese_fileutil_get_photo_path (cheese_window_get_fileutil (cheese_window));
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
  _really_move_to_trash (cheese_window, files_list);
  g_list_free (files_list);
  g_dir_close (dir_photos);
}

void
cheese_cmd_file_delete (GtkWidget *widget, CheeseWindow *cheese_window)
{
  GList *files_list = NULL;

  files_list = cheese_thumb_view_get_selected_images_list (cheese_window_get_thumbview (cheese_window));

  _really_delete (cheese_window, files_list, FALSE);
  g_list_free (files_list);
}

void
cheese_cmd_file_move_to_trash (GtkWidget *widget, CheeseWindow *cheese_window)
{
  GList *files_list = NULL;

  files_list = cheese_thumb_view_get_selected_images_list (cheese_window_get_thumbview (cheese_window));

  _really_move_to_trash (cheese_window, files_list);
  g_list_free (files_list);
}
