/*
 * Copyright © 2005 Paolo Maggi
 * Copyright © 2008 Sebastian Keller <sebastian-keller@gmx.de>
 * Copyright © 2008 daniel g. siegel <dgsiegel@gnome.org>
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
 *
 *---------
 *
 * Based on gedit-io-error-message-area.c by Paolo Maggi
 */

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "cheese-no-camera.h"

static void
cheese_no_camera_set_info_bar_text_and_icon (GtkInfoBar  *info_bar,
                                             const gchar *icon_stock_id,
                                             const gchar *primary_text,
                                             const gchar *secondary_text)
{
  GtkWidget *content_area;
  GtkWidget *image;
  GtkWidget *vbox;
  gchar     *primary_markup;
  gchar     *secondary_markup;
  GtkWidget *primary_label;
  GtkWidget *secondary_label;

  content_area = gtk_info_bar_get_content_area (GTK_INFO_BAR (info_bar));

  image = gtk_image_new_from_stock (icon_stock_id, GTK_ICON_SIZE_DIALOG);
  gtk_widget_show (image);
  gtk_box_pack_start (GTK_BOX (content_area), image, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_widget_show (vbox);
  gtk_box_pack_start (GTK_BOX (content_area), vbox, TRUE, TRUE, 0);

  primary_markup = g_strdup_printf ("<b>%s</b>", primary_text);
  primary_label  = gtk_label_new (primary_markup);
  g_free (primary_markup);
  gtk_widget_show (primary_label);
  gtk_box_pack_start (GTK_BOX (vbox), primary_label, TRUE, TRUE, 0);
  gtk_label_set_use_markup (GTK_LABEL (primary_label), TRUE);
  gtk_label_set_line_wrap (GTK_LABEL (primary_label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (primary_label), 0, 0.5);
  GTK_WIDGET_SET_FLAGS (primary_label, GTK_CAN_FOCUS);
  gtk_label_set_selectable (GTK_LABEL (primary_label), TRUE);

  if (secondary_text != NULL)
  {
    secondary_markup = g_strdup_printf ("<small>%s</small>",
                                        secondary_text);
    secondary_label = gtk_label_new (secondary_markup);
    g_free (secondary_markup);
    gtk_widget_show (secondary_label);
    gtk_box_pack_start (GTK_BOX (vbox), secondary_label, TRUE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (secondary_label, GTK_CAN_FOCUS);
    gtk_label_set_use_markup (GTK_LABEL (secondary_label), TRUE);
    gtk_label_set_line_wrap (GTK_LABEL (secondary_label), TRUE);
    gtk_label_set_selectable (GTK_LABEL (secondary_label), TRUE);
    gtk_misc_set_alignment (GTK_MISC (secondary_label), 0, 0.5);
  }
}

GtkWidget *
cheese_no_camera_info_bar_new ()
{
  GtkWidget *info_bar;

  info_bar = gtk_info_bar_new_with_buttons (GTK_STOCK_HELP, GTK_RESPONSE_HELP, NULL);
  gtk_info_bar_set_message_type (GTK_INFO_BAR (info_bar),
                                 GTK_MESSAGE_WARNING);
  cheese_no_camera_set_info_bar_text_and_icon (GTK_INFO_BAR (info_bar),
                                               "gtk-dialog-error",
                                               _("No camera found!"),
                                               _("Please refer to the help for further information."));

  return info_bar;
}
