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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gst/interfaces/xoverlay.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs.h>

#include "cheese.h"
#include "cheese-command-handler.h"
#include "cheese-config.h"
#include "cheese-effects-widget.h"
#include "cheese-pipeline.h"
#include "cheese-thumbnails.h"
#include "cheese-window.h"

#define GLADE_FILE CHEESE_DATA_DIR"/cheese.glade"

struct _cheese_window cheese_window;
struct _thumbnails thumbnails;

static void create_window (void);
static void on_about_cb (GtkWidget *, gpointer);
static void cheese_window_create_popup_menu (GtkTreePath *);
static void cheese_window_save_as_dialog (GtkWidget *, gpointer);
static gboolean on_button_press_event_cb (GtkWidget *, GdkEventButton *, gpointer);
static gboolean on_key_press_event_cb (GtkWidget *, GdkEventKey *, gpointer);

void
cheese_window_init ()
{
  create_window ();
}

void
cheese_window_finalize ()
{
}

static gboolean
on_key_press_event_cb (GtkWidget *widget, GdkEventKey *event,
    gpointer user_data)
{
  if (event->type == GDK_KEY_PRESS) {
    switch (event->keyval) {
      case GDK_t: // take a photo
      case GDK_T:
      case GDK_space:
        g_signal_emit_by_name (cheese_window.widgets.take_picture, "clicked");
        return TRUE;
      case GDK_Delete:
        gtk_icon_view_selected_foreach (GTK_ICON_VIEW (thumbnails.iconview),
                                        cheese_command_handler_move_to_trash, NULL);
        return TRUE;
    }
  }
  return FALSE;
}

static gboolean
on_button_press_event_cb (GtkWidget *iconview, GdkEventButton *event,
    gpointer user_data)
{
  GtkTreePath *path;

  if (event->type == GDK_BUTTON_PRESS || event->type == GDK_2BUTTON_PRESS)
  {
    path = gtk_icon_view_get_path_at_pos (GTK_ICON_VIEW (iconview),
        (gint) event->x, (gint) event->y);
    if (path == NULL)
    {
      return FALSE;
    }

    //gtk_icon_view_unselect_all (GTK_ICON_VIEW (iconview));
    gtk_icon_view_select_path (GTK_ICON_VIEW (thumbnails.iconview), path);

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

      cheese_window_create_popup_menu (path);

      gtk_menu_popup (GTK_MENU (cheese_window.widgets.popup_menu),
          NULL, iconview, NULL, NULL, button, event_time);
      return TRUE;
    }
    else if (event->type == GDK_2BUTTON_PRESS && event->button == 1)
    {
      cheese_command_handler_url_show (NULL, path);
      return TRUE;
    }
  }
  return FALSE;
}

static void
cheese_window_create_popup_menu (GtkTreePath * path)
{
  GtkWidget *menuitem;
  gchar *file = cheese_thumbnails_get_filename_from_path (path);
  cheese_window.widgets.popup_menu = gtk_menu_new ();

  menuitem = gtk_image_menu_item_new_from_stock (GTK_STOCK_OPEN, NULL);
  gtk_menu_append (GTK_MENU (cheese_window.widgets.popup_menu), menuitem);
  g_signal_connect (G_OBJECT (menuitem), "activate",
      G_CALLBACK (cheese_command_handler_url_show), path);
  gtk_widget_show (menuitem);

  menuitem = gtk_image_menu_item_new_from_stock (GTK_STOCK_SAVE_AS, NULL);
  gtk_menu_append (GTK_MENU (cheese_window.widgets.popup_menu), menuitem);
  g_signal_connect_data (G_OBJECT (menuitem), "activate",
      G_CALLBACK (cheese_window_save_as_dialog),
      g_strdup_printf ("%s", file),
      (GClosureNotify) g_free, 0);
  gtk_widget_show (menuitem);

  menuitem = gtk_separator_menu_item_new ();
  gtk_menu_append (GTK_MENU (cheese_window.widgets.popup_menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_image_menu_item_new_from_stock (GTK_STOCK_DELETE, NULL);
  gtk_menu_append (GTK_MENU (cheese_window.widgets.popup_menu), menuitem);
  g_signal_connect (G_OBJECT (menuitem), "activate",
      G_CALLBACK (cheese_command_handler_move_to_trash), path);
  gtk_widget_show (menuitem);

  menuitem = gtk_separator_menu_item_new ();
  gtk_menu_append (GTK_MENU (cheese_window.widgets.popup_menu), menuitem);
  gtk_widget_show (menuitem);

  if (g_find_program_in_path ("gnome-open"))
  {
    menuitem = gtk_image_menu_item_new_with_mnemonic (_("Send by _Mail"));
    gtk_menu_append (GTK_MENU (cheese_window.widgets.popup_menu), menuitem);
    g_signal_connect_data (G_OBJECT (menuitem), "activate",
        G_CALLBACK (cheese_command_handler_run_command_from_string),
        g_strdup_printf ("gnome-open mailto:?subject=%s&attachment=%s", g_basename (file), file),
        (GClosureNotify) g_free, 0);
    gtk_widget_show (menuitem);
  }

  if (!g_str_has_suffix (file, VIDEO_NAME_SUFFIX_DEFAULT))
  {
    menuitem = gtk_image_menu_item_new_with_mnemonic (_("Set as Account _Photo"));
    gtk_menu_append (GTK_MENU (cheese_window.widgets.popup_menu), menuitem);
    g_signal_connect_data (G_OBJECT (menuitem), "activate",
        G_CALLBACK (cheese_command_handler_about_me_update_photo),
        g_strdup_printf ("%s", file), (GClosureNotify) g_free, 0);
    gtk_widget_show (menuitem);

    if (g_find_program_in_path ("f-spot"))
    {
      menuitem = gtk_image_menu_item_new_with_mnemonic (_("Export to F-_Spot"));
      gtk_menu_append (GTK_MENU (cheese_window.widgets.popup_menu), menuitem);
      g_signal_connect_data (G_OBJECT (menuitem), "activate",
          G_CALLBACK (cheese_command_handler_run_command_from_string),
          g_strdup_printf ("f-spot -i %s", g_path_get_dirname (file)), (GClosureNotify) g_free, 0);
      gtk_widget_show (menuitem);
    }

    if (g_find_program_in_path ("postr"))
    {
      menuitem = gtk_image_menu_item_new_with_mnemonic (_("Export to _Flickr"));
      gtk_menu_append (GTK_MENU (cheese_window.widgets.popup_menu), menuitem);
      g_signal_connect_data (G_OBJECT (menuitem), "activate",
          G_CALLBACK (cheese_command_handler_run_command_from_string),
          g_strdup_printf ("postr %s", file), (GClosureNotify) g_free, 0);
      gtk_widget_show (menuitem);
    }
  }
  g_free (file);
}

static void
cheese_window_save_as_dialog (GtkWidget *widget, gpointer data)
{
  GtkWidget *dialog;
  gint response;
  gchar *file = data;
  printf("AAA %s", file);

  dialog = gtk_file_chooser_dialog_new ("Save File",
      GTK_WINDOW (cheese_window.window),
      GTK_FILE_CHOOSER_ACTION_SAVE,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
      NULL);

  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), g_basename (file));
  response = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_hide (dialog);

  if (response == GTK_RESPONSE_ACCEPT)
  {
    char *filename, *dirname;

    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    dirname = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));

    GnomeVFSURI const *source_uri =
      gnome_vfs_uri_new (g_filename_to_uri (file, NULL, NULL));
    GnomeVFSURI const *target_uri =
      gnome_vfs_uri_new (g_filename_to_uri (filename, NULL, NULL));

    response = gnome_vfs_xfer_uri (source_uri, target_uri,
        GNOME_VFS_XFER_DEFAULT |
        GNOME_VFS_XFER_FOLLOW_LINKS,
        GNOME_VFS_XFER_ERROR_MODE_ABORT,
        GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE,
        NULL, NULL);

    if (response != GNOME_VFS_OK)
    {
      char *header;
      GtkWidget *dlg;

      header = g_strdup_printf (_("Could not save %s"), filename);

      dlg = gtk_message_dialog_new (GTK_WINDOW (cheese_window.window),
          GTK_DIALOG_MODAL |
          GTK_DIALOG_DESTROY_WITH_PARENT,
          GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
          header);

      gtk_dialog_run (GTK_DIALOG (dlg));

      gtk_widget_destroy (dlg);

      g_free (header);

    }

    g_free (filename);
    g_free (dirname);
  }

  gtk_widget_destroy (dialog);
}

int
show_move_to_trash_confirm_dialog (gchar *file)
{
  GtkWidget *dlg;
  char *prompt;
  int response;

  prompt = g_strdup_printf (_("Are you sure you want to move\n\"%s\" to the trash?"), g_basename (file));

  dlg = gtk_message_dialog_new_with_markup (GTK_WINDOW (cheese_window.window),
      GTK_DIALOG_MODAL |
      GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_MESSAGE_QUESTION,
      GTK_BUTTONS_NONE,
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

void
cheese_window_change_effect (GtkWidget *widget, gpointer self)
{
  if (gtk_notebook_get_current_page
      (GTK_NOTEBOOK (cheese_window.widgets.notebook)) == 1)
  {
    gtk_notebook_set_current_page (GTK_NOTEBOOK
        (cheese_window.widgets.notebook), 0);
    gtk_label_set_text_with_mnemonic (GTK_LABEL
        (cheese_window.widgets.label_effects),
        _("_Effects"));
    gtk_widget_set_sensitive (cheese_window.widgets.take_picture, TRUE);
    cheese_pipeline_change_effect ();
  }
  else
  {
    gtk_notebook_set_current_page (GTK_NOTEBOOK
        (cheese_window.widgets.notebook), 1);
    gtk_label_set_text_with_mnemonic (GTK_LABEL
        (cheese_window.widgets.label_effects),
        _("_Back"));
    gtk_widget_set_sensitive (GTK_WIDGET (cheese_window.widgets.take_picture), FALSE);
  }
}

gboolean
cheese_window_expose_cb (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  GstElement *tmp = gst_bin_get_by_interface (GST_BIN (cheese_pipeline_get_pipeline ()),
        GST_TYPE_X_OVERLAY);
  gst_x_overlay_set_xwindow_id (GST_X_OVERLAY (tmp),
      GDK_WINDOW_XWINDOW (widget->window));
  // this is for using x(v)imagesink natively:
  //gst_x_overlay_set_xwindow_id(GST_X_OVERLAY(data),
  //    GDK_WINDOW_XWINDOW(widget->window));
  return FALSE;
}

void
cheese_window_button_video_cb (GtkWidget * widget, gpointer self)
{
  gtk_widget_set_sensitive (GTK_WIDGET (cheese_window.widgets.button_photo), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (cheese_window.widgets.button_video), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (cheese_window.widgets.check_item_countdown), FALSE);
  gtk_label_set_text_with_mnemonic (GTK_LABEL
      (cheese_window.widgets.label_take_photo),
      _("<b>_Start recording</b>"));
  gtk_label_set_use_markup (GTK_LABEL (cheese_window.widgets.label_take_photo), TRUE);
  cheese_pipeline_change_pipeline_type ();
  cheese_window_expose_cb (cheese_window.widgets.screen, NULL, NULL);
}

void
cheese_window_button_photo_cb (GtkWidget *widget, gpointer self)
{
  gtk_widget_set_sensitive (GTK_WIDGET (cheese_window.widgets.button_photo), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (cheese_window.widgets.button_video), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (cheese_window.widgets.check_item_countdown), TRUE);
  gtk_label_set_text_with_mnemonic (GTK_LABEL
      (cheese_window.widgets.label_take_photo),
      _("<b>_Take a photo</b>"));
  gtk_label_set_use_markup (GTK_LABEL
      (cheese_window.widgets.label_take_photo), TRUE);
  cheese_pipeline_change_pipeline_type ();
  cheese_window_expose_cb (cheese_window.widgets.screen, NULL, NULL);
}

void
cheese_window_pipeline_button_clicked_cb (GtkWidget *widget, gpointer self)
{
  if (!GTK_WIDGET_SENSITIVE(cheese_window.widgets.button_photo)) {
    if (!cheese_pipeline_countdown_is_active()) {
      cheese_pipeline_button_clicked (widget);
    }
  } else if (!GTK_WIDGET_SENSITIVE(cheese_window.widgets.button_video)) {
    cheese_pipeline_button_clicked (widget);
    cheese_window_expose_cb (cheese_window.widgets.screen, NULL, NULL);
  }
}

void
cheese_window_countdown_item_clicked_cb (GtkWidget *widget, gpointer self)
{
  if (gtk_check_menu_item_get_active (
        GTK_CHECK_MENU_ITEM (cheese_window.widgets.check_item_countdown))) {
      cheese_pipeline_set_countdown(TRUE);
      g_print("Setting countdown to TRUE\n");
  } else {
      cheese_pipeline_set_countdown(FALSE);
      g_print("Setting countdown to FALSE\n");
  }
}

static void
create_window ()
{
  GtkWidget *file_menu;
  GtkWidget *help_menu;
  GtkWidget *menu;
  GtkWidget *menuitem;

  cheese_window.gxml                        = glade_xml_new (GLADE_FILE, NULL, NULL);
  cheese_window.window                      = glade_xml_get_widget (cheese_window.gxml, "cheese_window");
  cheese_window.widgets.button_effects      = glade_xml_get_widget (cheese_window.gxml, "button_effects");
  cheese_window.widgets.button_photo        = glade_xml_get_widget (cheese_window.gxml, "button_photo");
  cheese_window.widgets.button_video        = glade_xml_get_widget (cheese_window.gxml, "button_video");
  cheese_window.widgets.effects_widget      = glade_xml_get_widget (cheese_window.gxml, "effects_screen");
  cheese_window.widgets.image_take_photo    = glade_xml_get_widget (cheese_window.gxml, "image_take_photo");
  cheese_window.widgets.label_effects       = glade_xml_get_widget (cheese_window.gxml, "label_effects");
  cheese_window.widgets.label_photo         = glade_xml_get_widget (cheese_window.gxml, "label_photo");
  cheese_window.widgets.label_take_photo    = glade_xml_get_widget (cheese_window.gxml, "label_take_photo");
  cheese_window.widgets.label_video         = glade_xml_get_widget (cheese_window.gxml, "label_video");
  cheese_window.widgets.menubar             = glade_xml_get_widget (cheese_window.gxml, "menubar");
  cheese_window.widgets.notebook            = glade_xml_get_widget (cheese_window.gxml, "notebook");
  cheese_window.widgets.screen              = glade_xml_get_widget (cheese_window.gxml, "video_screen");
  cheese_window.widgets.table               = glade_xml_get_widget (cheese_window.gxml, "table");
  cheese_window.widgets.take_picture        = glade_xml_get_widget (cheese_window.gxml, "take_picture");
  thumbnails.iconview                       = glade_xml_get_widget (cheese_window.gxml, "previews");

  gtk_widget_set_size_request (thumbnails.iconview, -1, THUMB_HEIGHT + 25);

  gtk_widget_set_sensitive (GTK_WIDGET (cheese_window.widgets.button_photo), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (cheese_window.widgets.button_video), TRUE);

  g_signal_connect (G_OBJECT (cheese_window.widgets.button_photo), "clicked",
      G_CALLBACK (cheese_window_button_photo_cb), NULL);
  g_signal_connect (G_OBJECT (cheese_window.widgets.button_video), "clicked",
      G_CALLBACK (cheese_window_button_video_cb), NULL);

  gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (thumbnails.iconview), 0);
  gtk_icon_view_set_columns (GTK_ICON_VIEW (thumbnails.iconview), G_MAXINT);

  // menubar
  file_menu = gtk_menu_item_new_with_mnemonic (_("_Cheese"));
  help_menu = gtk_menu_item_new_with_mnemonic (_("_Help"));
  gtk_menu_bar_append (GTK_MENU_BAR (cheese_window.widgets.menubar), file_menu);
  gtk_menu_bar_append (GTK_MENU_BAR (cheese_window.widgets.menubar), help_menu);

  // file menu
  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (file_menu), menu);

  cheese_window.widgets.check_item_countdown =
    gtk_check_menu_item_new_with_mnemonic (_("_Countdown on taking a photo"));
  gtk_menu_append (GTK_MENU (menu), cheese_window.widgets.check_item_countdown);
  gtk_check_menu_item_set_active
    (GTK_CHECK_MENU_ITEM (cheese_window.widgets.check_item_countdown), TRUE);
  g_signal_connect (GTK_OBJECT (cheese_window.widgets.check_item_countdown), "toggled",
      GTK_SIGNAL_FUNC (cheese_window_countdown_item_clicked_cb), NULL);

  menuitem = gtk_separator_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_image_menu_item_new_from_stock (GTK_STOCK_QUIT, NULL);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  g_signal_connect (GTK_OBJECT (menuitem), "activate",
      GTK_SIGNAL_FUNC (on_cheese_window_close_cb), NULL);

  // help menu
  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (help_menu), menu);

  menuitem = gtk_image_menu_item_new_from_stock (GTK_STOCK_ABOUT, NULL);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  g_signal_connect (GTK_OBJECT (menuitem), "activate",
      GTK_SIGNAL_FUNC (on_about_cb), cheese_window.window);

  g_signal_connect (G_OBJECT (cheese_window.window), "destroy",
      G_CALLBACK (on_cheese_window_close_cb), NULL);
  g_signal_connect (G_OBJECT (cheese_window.widgets.take_picture), "clicked",
      G_CALLBACK (cheese_window_pipeline_button_clicked_cb), NULL);
  g_signal_connect (G_OBJECT (cheese_window.widgets.button_effects),
      "clicked", G_CALLBACK (cheese_window_change_effect), NULL);

  g_signal_connect_after (GTK_OBJECT (thumbnails.iconview), "key_press_event",
      G_CALLBACK (on_key_press_event_cb), NULL);
  g_signal_connect_after (GTK_OBJECT (thumbnails.iconview), "button_press_event",
      G_CALLBACK (on_button_press_event_cb), NULL);
  g_signal_connect_after (GTK_OBJECT (thumbnails.iconview), "item_activated",
      G_CALLBACK (cheese_command_handler_url_show), NULL);

}

static void
on_about_cb (GtkWidget *p_widget, gpointer user_data)
{
  static const char *authors[] = {
    "daniel g. siegel <dgsiegel@gmail.com>",
    NULL
  };

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

  gtk_show_about_dialog (GTK_WINDOW (user_data),
      "version", CHEESE_VERSION,
      "copyright", "Copyright \xc2\xa9 2007\n daniel g. siegel <dgsiegel@gmail.com>",
      "comments", _("A cheesy program to take pictures and videos from your webcam"),
      "authors", authors,
      "website", "http://live.gnome.org/Cheese",
      "logo-icon-name", "cheese",
      "wrap-license", TRUE,
      "license", license_trans,
      NULL);

  g_free (license_trans);
}
