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

#include <libgnomevfs/gnome-vfs.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gst/interfaces/xoverlay.h>
#include <gdk/gdkx.h>

#include "cheese.h"
#include "cheese-window.h"
#include "cheese-thumbnails.h"
#include "cheese-config.h"
#include "cheese-pipeline.h"
#include "cheese-effects-widget.h"

#define GLADE_FILE CHEESE_DATA_DIR"/cheese.glade"


struct _cheese_window cheese_window;
struct _thumbnails thumbnails;

static void create_window();
static void on_about_cb (GtkWidget *p_widget, gpointer user_data);

void cheese_window_init() {
  create_window();
}

void cheese_window_finalize() {
}

void on_item_activated_cb (GtkIconView *iconview, GtkTreePath *tree_path, gpointer user_data) {
  GtkTreeIter iter;
  const gchar *file;
  gtk_tree_model_get_iter(GTK_TREE_MODEL(thumbnails.store), &iter, tree_path);
  gtk_tree_model_get(GTK_TREE_MODEL(thumbnails.store), &iter, 1, &file, -1);
  g_print("opening file %s\n", file);
  file = g_strconcat ("file://", file, NULL);
  gnome_vfs_url_show(file);
}

void
cheese_window_change_effect(GtkWidget *widget, gpointer self)
{
  if (gtk_notebook_get_current_page(GTK_NOTEBOOK(cheese_window.widgets.notebook)) == 1) {
    gtk_notebook_set_current_page (GTK_NOTEBOOK(cheese_window.widgets.notebook), 0);
    gtk_label_set_text_with_mnemonic(GTK_LABEL(cheese_window.widgets.label_effects), _("_Effects"));
    gtk_widget_set_sensitive(cheese_window.widgets.take_picture, TRUE);
    cheese_pipeline_change_effect();
  }
  else {
    gtk_notebook_set_current_page (GTK_NOTEBOOK(cheese_window.widgets.notebook), 1);
    gtk_label_set_text_with_mnemonic(GTK_LABEL(cheese_window.widgets.label_effects), _("_Back"));
    gtk_widget_set_sensitive(GTK_WIDGET(cheese_window.widgets.take_picture), FALSE);
  }
}

gboolean
cheese_window_expose_cb(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  GstElement *tmp = gst_bin_get_by_interface(GST_BIN(cheese_pipeline_get_pipeline()),
      GST_TYPE_X_OVERLAY);
  gst_x_overlay_set_xwindow_id(GST_X_OVERLAY(tmp),
      GDK_WINDOW_XWINDOW(widget->window));
  // this is for using x(v)imagesink natively:
  //gst_x_overlay_set_xwindow_id(GST_X_OVERLAY(data),
  //    GDK_WINDOW_XWINDOW(widget->window));
  return FALSE;
}

void
cheese_window_button_video_cb(GtkWidget *widget, gpointer self)
{
  gtk_widget_set_sensitive(GTK_WIDGET(cheese_window.widgets.button_photo), TRUE);
  gtk_widget_set_sensitive(GTK_WIDGET(cheese_window.widgets.button_video), FALSE);
  cheese_pipeline_change_pipeline_type();
  cheese_window_expose_cb(cheese_window.widgets.screen, NULL, NULL);
}

void
cheese_window_button_photo_cb(GtkWidget *widget, gpointer self)
{
  gtk_widget_set_sensitive(GTK_WIDGET(cheese_window.widgets.button_photo), FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(cheese_window.widgets.button_video), TRUE);
  cheese_pipeline_change_pipeline_type();
  cheese_window_expose_cb(cheese_window.widgets.screen, NULL, NULL);
}

static void
create_window()
{
  GtkWidget *file_menu;
  GtkWidget *help_menu;
  GtkWidget *menu;
  GtkWidget *menuitem;

  cheese_window.gxml                          = glade_xml_new(GLADE_FILE, NULL, NULL);
  cheese_window.window                        = glade_xml_get_widget(cheese_window.gxml, "cheese_window");
  cheese_window.widgets.menubar               = glade_xml_get_widget(cheese_window.gxml, "menubar");
  cheese_window.widgets.take_picture          = glade_xml_get_widget(cheese_window.gxml, "take_picture");
  cheese_window.widgets.screen                = glade_xml_get_widget(cheese_window.gxml, "video_screen");
  cheese_window.widgets.notebook              = glade_xml_get_widget(cheese_window.gxml, "notebook");
  cheese_window.widgets.table                 = glade_xml_get_widget(cheese_window.gxml, "table");
  cheese_window.widgets.button_photo          = glade_xml_get_widget(cheese_window.gxml, "button_photo");
  cheese_window.widgets.button_video          = glade_xml_get_widget(cheese_window.gxml, "button_video");
  cheese_window.widgets.button_effects        = glade_xml_get_widget(cheese_window.gxml, "button_effects");
  cheese_window.widgets.label_effects         = glade_xml_get_widget(cheese_window.gxml, "label_effects");
  cheese_window.widgets.label_photo           = glade_xml_get_widget(cheese_window.gxml, "label_photo");
  cheese_window.widgets.label_video           = glade_xml_get_widget(cheese_window.gxml, "label_video");
  cheese_window.widgets.label_take_photo      = glade_xml_get_widget(cheese_window.gxml, "label_take_photo");
  cheese_window.widgets.effects_widget        = glade_xml_get_widget(cheese_window.gxml, "effects_screen");
  thumbnails.iconview                         = glade_xml_get_widget(cheese_window.gxml, "previews");

  gtk_widget_set_size_request(thumbnails.iconview, -1 , THUMB_HEIGHT + 20);

  gtk_widget_set_sensitive(GTK_WIDGET(cheese_window.widgets.button_photo), FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(cheese_window.widgets.button_video), TRUE);

  g_signal_connect(G_OBJECT(cheese_window.widgets.button_photo), "clicked",
      G_CALLBACK(cheese_window_button_photo_cb), NULL);
  g_signal_connect(G_OBJECT(cheese_window.widgets.button_video), "clicked",
      G_CALLBACK(cheese_window_button_video_cb), NULL);

  gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(thumbnails.iconview), 0);
  gtk_icon_view_set_columns(GTK_ICON_VIEW(thumbnails.iconview), G_MAXINT);

  // menubar
  file_menu = gtk_menu_item_new_with_mnemonic(_("_File"));
  help_menu = gtk_menu_item_new_with_mnemonic(_("_Help"));
  gtk_menu_bar_append(GTK_MENU_BAR(cheese_window.widgets.menubar), file_menu);
  gtk_menu_bar_append(GTK_MENU_BAR(cheese_window.widgets.menubar), help_menu);

  // file menu
  menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_menu), menu);

  menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
  gtk_menu_append(GTK_MENU(menu), menuitem);
  gtk_signal_connect(GTK_OBJECT (menuitem), "activate",
      GTK_SIGNAL_FUNC(on_cheese_window_close_cb), NULL);

  // help menu
  menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_menu), menu);

  menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);
  gtk_menu_append(GTK_MENU(menu), menuitem);
  gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
      GTK_SIGNAL_FUNC(on_about_cb), cheese_window.window);

  gtk_signal_connect(GTK_OBJECT(thumbnails.iconview), "item-activated",
      GTK_SIGNAL_FUNC(on_item_activated_cb), NULL);

}

static void on_about_cb (GtkWidget *p_widget, gpointer user_data)
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

  gtk_show_about_dialog (GTK_WINDOW(user_data),
                         "version", CHEESE_VERSION,
		                     "copyright", "Copyright \xc2\xa9 2007\n daniel g. siegel <dgsiegel@gmail.com>",
                         "comments", _("A cheesy program to take pictures from your webcam"),
                         "authors", authors,
                         "website", "http://live.gnome.org/Cheese",
                         "logo-icon-name", "cheese",
                         "wrap-license", TRUE,
                         "license", license_trans,
                         NULL);

  g_free (license_trans);
}

