/*
 * Copyright (C) 2007 daniel g. siegel <dgsiegel@gmail.com>
 * All rights reserved. This software is released under the GPL2 licence.
 */

#include "cheese.h"
#include "window.h"
#include "thumbnails.h"

struct _cheese_window cheese_window;
struct _thumbnails thumbnails;

void
create_window()
{
  cheese_window.gxml    = glade_xml_new(GLADE_FILE, NULL, NULL);
  cheese_window.window  = glade_xml_get_widget(cheese_window.gxml, "cheese_window");
  cheese_window.widgets.take_picture          = glade_xml_get_widget(cheese_window.gxml, "take_picture");
  cheese_window.widgets.screen                = glade_xml_get_widget(cheese_window.gxml, "screen");
  cheese_window.widgets.button_photo          = glade_xml_get_widget(cheese_window.gxml, "button_photo");
  cheese_window.widgets.button_video          = glade_xml_get_widget(cheese_window.gxml, "button_video");
  cheese_window.widgets.button_effects_left   = glade_xml_get_widget(cheese_window.gxml, "button_effects_left");
  cheese_window.widgets.button_effects_right  = glade_xml_get_widget(cheese_window.gxml, "button_effects_right");
  thumbnails.iconview   = glade_xml_get_widget(cheese_window.gxml, "previews");

  gtk_window_set_title(GTK_WINDOW(cheese_window.window), "Cheese");

  gtk_widget_set_size_request(cheese_window.widgets.screen, PHOTO_WIDTH, PHOTO_HEIGHT);
  gtk_widget_set_size_request(thumbnails.iconview, PHOTO_WIDTH, THUMB_HEIGHT + 20);

  gtk_widget_set_sensitive(GTK_WIDGET(cheese_window.widgets.button_photo), FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(cheese_window.widgets.button_video), FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(cheese_window.widgets.button_effects_left), FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(cheese_window.widgets.button_effects_right), FALSE);

  gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(thumbnails.iconview), 0);
  gtk_icon_view_set_columns(GTK_ICON_VIEW(thumbnails.iconview), G_MAXINT);
}
