/*
 * Copyright © 2011 Lucas Baudin <xapantu@gmail.com>
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

#include <stdlib.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include <clutter-gtk/clutter-gtk.h>
#include "cheese-avatar-chooser.h"
#include "cheese-flash.h"
#include "cheese-widget.h"
#include "um-crop-area.h"
#include "cheese-gtk.h"

/* CheeseAvatarChooser */
static void
avatar_chooser (void)
{
    GtkWidget *chooser, *select_button;

    chooser = gtk_test_create_widget (CHEESE_TYPE_AVATAR_CHOOSER, NULL);
    g_assert (chooser != NULL);

    /* Check that all the child widgets were successfully instantiated. */
    select_button = gtk_test_find_widget (chooser, "Select", GTK_TYPE_BUTTON);
    g_assert (select_button != NULL);
    g_assert (GTK_IS_BUTTON (select_button));
}

/* CheeseFlash */
static void
flash (void)
{
    GtkWidget *flash, *window;

    window = gtk_test_create_simple_window ("CheeseFlash", "CheeseFlash test");
    g_assert (window != NULL);

    /* Window must be realised to have a GdkWindow. */
    gtk_widget_show (window);

    flash = gtk_test_create_widget (CHEESE_TYPE_FLASH, "parent", window, NULL);
    g_assert (flash != NULL);

    cheese_flash_fire (CHEESE_FLASH (flash));
}

/* UmCropArea. */
static void
um_crop_area (void)
{
    GtkWidget *crop_area;
    GdkPixbuf *pixbuf, *pixbuf2;

    crop_area = gtk_test_create_widget (UM_TYPE_CROP_AREA, NULL);
    pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 800, 480);
    um_crop_area_set_min_size (UM_CROP_AREA (crop_area), 200, 100);
    um_crop_area_set_picture (UM_CROP_AREA (crop_area), pixbuf);
    pixbuf2 = um_crop_area_get_picture (UM_CROP_AREA (crop_area));

    /* It must be 2*min_size. */
    g_assert_cmpint (gdk_pixbuf_get_width (pixbuf2), ==, 400);
    g_assert_cmpint (gdk_pixbuf_get_height (pixbuf2), ==, 200);

    g_object_unref (pixbuf);
    g_object_unref (pixbuf2);
}

/* CheeseWidget */
static void widget (void)
{
    GtkWidget *widget;

    widget = gtk_test_create_widget (CHEESE_TYPE_WIDGET, NULL);
    g_assert (widget != NULL);
}

int main (int argc, gchar *argv[])
{
    gtk_test_init (&argc, &argv, NULL);
    if (!cheese_gtk_init (&argc, &argv))
        return EXIT_FAILURE;

    g_test_add_func ("/libcheese-gtk/avatar_chooser", avatar_chooser);
    g_test_add_func ("/libcheese-gtk/flash", flash);
    g_test_add_func ("/libcheese-gtk/um_crop_area", um_crop_area);
    g_test_add_func ("/libcheese-gtk/widget", widget);

    return g_test_run ();
}
