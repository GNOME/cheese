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

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include <clutter-gtk/clutter-gtk.h>
#include "cheese-avatar-chooser.h"
#include "cheese-widget.h"
#include "cheese-fileutil.h"
#include "um-crop-area.h"

/* Test widget instantiation. */
/* TODO: add widgets to a window and show them. */
static void test_widget ()
{
    GtkWidget *widget, *select_button;

    widget = cheese_widget_new ();
    g_assert (widget != NULL);
    gtk_widget_destroy (widget);

    widget = cheese_avatar_chooser_new ();
    g_assert (widget != NULL);
    g_assert (GTK_IS_WIDGET (widget));

    /* Check that all the child widgets were successfully instantiated. */
    g_assert (gtk_test_find_widget (widget, "Take a photo", GTK_TYPE_BUTTON)
        != NULL);
    select_button = gtk_test_find_widget (widget, "Select", GTK_TYPE_BUTTON);
    g_assert (select_button != NULL);
    g_assert (GTK_IS_BUTTON (select_button));
}

/* Test CheeseFileUtil */
static void test_file_utils ()
{
    CheeseFileUtil *file_util;
    gchar *first_path, *second_path, *third_path, *stub;
    gchar *real_second, *real_third;
    gchar **split;

    file_util = cheese_fileutil_new ();
    g_assert (file_util != NULL);

    first_path = cheese_fileutil_get_new_media_filename (file_util,
        CHEESE_MEDIA_MODE_BURST);
    split = g_strsplit (first_path, "_1.jpg", -1);
    stub = g_strdup (split[0]);

    second_path = g_strdup_printf ("%s_2.jpg", stub);
    third_path = g_strdup_printf ("%s_3.jpg", stub);

    real_second = cheese_fileutil_get_new_media_filename (file_util,
        CHEESE_MEDIA_MODE_BURST);
    real_third = cheese_fileutil_get_new_media_filename (file_util,
        CHEESE_MEDIA_MODE_BURST);

    g_assert_cmpstr (real_second, ==, second_path);
    g_assert_cmpstr (real_third, ==, third_path);

    g_strfreev (split);
    g_free (real_second);
    g_free (real_third);
    g_free (second_path);
    g_free (third_path);
    g_free (stub);
    g_object_unref (file_util);
}

/* Test UmCropArea. */
/* TODO: Test widget instantiation. */
static void test_crop_area ()
{
    GtkWidget *crop_area;
    GdkPixbuf *pixbuf, *pixbuf2;

    crop_area = um_crop_area_new ();
    pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 800, 480);
    um_crop_area_set_min_size (UM_CROP_AREA (crop_area), 200, 100);
    um_crop_area_set_picture (UM_CROP_AREA (crop_area), pixbuf);
    pixbuf2 = um_crop_area_get_picture (UM_CROP_AREA (crop_area));

    /* It must be 2*min_size. */
    g_assert_cmpint (gdk_pixbuf_get_width (pixbuf2), ==, 400);
    g_assert_cmpint (gdk_pixbuf_get_height (pixbuf2), ==, 200);

    g_object_unref (pixbuf);
    g_object_unref (pixbuf2);
    g_object_unref (crop_area);
}

int main (int argc, gchar *argv[])
{
    g_thread_init (NULL);
    gdk_threads_init ();
    gst_init (&argc, &argv);
    gtk_test_init (&argc, &argv, NULL);
    if (gtk_clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
        return 1;

    g_test_add_func ("/cheese/widgets", test_widget);
    g_test_add_func ("/cheese/file_util", test_file_utils);
    g_test_add_func ("/cheese/crop_area", test_crop_area);

    return g_test_run ();
}
