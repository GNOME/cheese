/*
 * Copyright (C) 2007 daniel g. siegel <dgsiegel@gmail.com>
 * All rights reserved. This software is released under the GPL2 licence.
 */

#include <libgnomevfs/gnome-vfs.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libintl.h>

#include "cheese.h"
#include "window.h"
#include "thumbnails.h"

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
	static GtkWidget *about = NULL;
	//char             *authors [] = {
	//	"daniel g. siegel <dgsiegel@gmail.com>",
	//	NULL
	//};

	if (about) {
		gtk_window_present(GTK_WINDOW(about));
		return;
	}

	about = gtk_about_dialog_new();
	g_object_set (about,
		      "name",  CHEESE_PACKAGE_NAME,
		      "version", CHEESE_VERSION,
		      "copyright", "Copyright (c) 2007\n daniel g. siegel <dgsiegel@gmail.com>",
		      "comments", _("A cheesy program to take pictures from your webcam"),
		      //"authors", authors,
		      //"logo-icon-name", "face-grin",
		      "logo-icon-name", "cheese",
		      NULL);

	gtk_window_set_wmclass(GTK_WINDOW(about), "about_dialog", "Panel");
	gtk_window_set_screen(GTK_WINDOW(about),
			       gdk_screen_get_default());
  gtk_window_set_default_icon_name ("cheese");
	g_signal_connect(about, "destroy",
			  G_CALLBACK(gtk_widget_destroyed),
			  &about);
// 	g_signal_connect (about, "event",
// 			  G_CALLBACK (panel_context_menu_check_for_screen),
// 			  NULL);

	g_signal_connect (about, "response",
			  G_CALLBACK (gtk_widget_destroy),
			  NULL);

	gtk_widget_show (about);
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
      GTK_SIGNAL_FUNC(on_about_cb), NULL);

  gtk_signal_connect(GTK_OBJECT(thumbnails.iconview), "item-activated",
      GTK_SIGNAL_FUNC(on_item_activated_cb), NULL);

}
