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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <libgnomevfs/gnome-vfs.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libintl.h>
#include <gtk/gtk.h>

#include "cheese.h"
#include "window.h"
#include "thumbnails.h"
#include "cheese_config.h"
#include "pipeline-photo.h"
#include "effects-widget.h"

#define _(str) gettext(str)

struct _cheese_window cheese_window;
struct _thumbnails thumbnails;

void
set_effects_label(gchar *effect)
{
  gtk_label_set_text(GTK_LABEL(cheese_window.widgets.label_effects), effect);
}

void on_about_cb (GtkWidget *p_widget, gpointer user_data)
{
  static const char *authors[] = {
    "daniel g. siegel <dgsiegel@gmail.com>",
    NULL
  };

  const char *license[] = {
    _("This program is free software; you can redistribute it and/or modify "
      "it under the terms of the GNU General Public License as published by "
      "the Free Software Foundation; either version 2 of the License, or "
      "(at your option) any later version.\n"),
    _("This program is distributed in the hope that it will be useful, "
      "but WITHOUT ANY WARRANTY; without even the implied warranty of "
      "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
      "GNU General Public License for more details.\n"),
    _("You should have received a copy of the GNU General Public License "
      "along with this program; if not, write to the Free Software "
      "Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.")
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

void on_item_activated_cb (GtkIconView *iconview, GtkTreePath *tree_path, gpointer user_data) {
  GtkTreeIter iter;
  const gchar *file;
  gtk_tree_model_get_iter(GTK_TREE_MODEL(thumbnails.store), &iter, tree_path);
  gtk_tree_model_get(GTK_TREE_MODEL(thumbnails.store), &iter, 1, &file, -1);
  printf("opening file %s\n", file);
  file = g_strconcat ("file://", file, NULL);
  gnome_vfs_url_show(file);
}

void
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
  cheese_window.widgets.screen                = glade_xml_get_widget(cheese_window.gxml, "screen");
  cheese_window.widgets.notebook              = glade_xml_get_widget(cheese_window.gxml, "notebook");
  cheese_window.widgets.table                 = glade_xml_get_widget(cheese_window.gxml, "table");
  cheese_window.widgets.button_photo          = glade_xml_get_widget(cheese_window.gxml, "button_photo");
  cheese_window.widgets.button_video          = glade_xml_get_widget(cheese_window.gxml, "button_video");
  cheese_window.widgets.button_effects        = glade_xml_get_widget(cheese_window.gxml, "button_effects");
  cheese_window.widgets.label_effects         = glade_xml_get_widget(cheese_window.gxml, "label_effects");
  cheese_window.widgets.label_photo           = glade_xml_get_widget(cheese_window.gxml, "label_photo");
  cheese_window.widgets.label_video           = glade_xml_get_widget(cheese_window.gxml, "label_video");
  cheese_window.widgets.label_take_photo      = glade_xml_get_widget(cheese_window.gxml, "label_take_photo");
  thumbnails.iconview                         = glade_xml_get_widget(cheese_window.gxml, "previews");

  gtk_widget_set_size_request(cheese_window.widgets.screen, PHOTO_WIDTH, PHOTO_HEIGHT);
  gtk_widget_set_size_request(thumbnails.iconview, PHOTO_WIDTH, THUMB_HEIGHT + 20);

  gtk_widget_set_sensitive(GTK_WIDGET(cheese_window.widgets.button_photo), FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(cheese_window.widgets.button_video), FALSE);

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

void
window_change_effect(GtkWidget *widget, gpointer self)
{
  if (gtk_notebook_get_current_page(GTK_NOTEBOOK(cheese_window.widgets.notebook)) == 1) {
    gtk_notebook_set_current_page (GTK_NOTEBOOK(cheese_window.widgets.notebook), 0);
    gtk_label_set_text(GTK_LABEL(cheese_window.widgets.label_effects), _("Effects"));
    gtk_widget_set_sensitive(cheese_window.widgets.take_picture, TRUE);
    pipeline_change_effect(self);
  }
  else {
    if (cheese_window.widgets.effects_widget == NULL) {
      gtk_widget_show(cheese_window.widgets.effects_widget);
    }
    gtk_notebook_set_current_page (GTK_NOTEBOOK(cheese_window.widgets.notebook), 1);
    gtk_label_set_text(GTK_LABEL(cheese_window.widgets.label_effects), _("Back"));
    gtk_widget_set_sensitive(GTK_WIDGET(cheese_window.widgets.take_picture), FALSE);
  }
}
