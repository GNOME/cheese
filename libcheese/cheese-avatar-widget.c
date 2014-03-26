/*
 * Copyright © 2009 Bastien Nocera <hadess@hadess.net>
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

#include <glib/gi18n-lib.h>
#include <canberra-gtk.h>

#include "cheese-camera.h"
#include "cheese-widget-private.h"
#include "cheese-flash.h"
#include "cheese-avatar-widget.h"
#include "um-crop-area.h"

/**
 * SECTION:cheese-avatar-widget
 * @short_description: A photo capture dialog for avatars
 * @stability: Unstable
 * @include: cheese/cheese-avatar-widget.h
 *
 * #CheeseAvatarWidget presents a simple window to the user for taking a photo
 * for use as an avatar.
 */

enum
{
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_PIXBUF,
  PROP_LAST
};

enum
{
  WIDGET_PAGE = 0,
  IMAGE_PAGE  = 1,
};

struct _CheeseAvatarWidgetPrivate
{
  GtkWidget *notebook;
  GtkWidget *camera;
  GtkWidget *image;
  GtkWidget *take_button;
  GtkWidget *take_again_button;
  GtkSizeGroup *sizegroup;
  CheeseFlash *flash;
  gulong photo_taken_id;
};

static GParamSpec *properties[PROP_LAST];

G_DEFINE_TYPE_WITH_PRIVATE (CheeseAvatarWidget, cheese_avatar_widget, GTK_TYPE_BIN);

/*
 * cheese_widget_photo_taken_cb:
 * @camera: a #CheeseCamera
 * @pixbuf: the #GdkPixbuf of the image that was just taken
 * @choose: a #CheeseAvatarWidget
 *
 * Show the image that was just taken from the camera (as @pixbuf) in the
 * cropping tool.
 */
static void
cheese_widget_photo_taken_cb (CheeseCamera        *camera,
                              GdkPixbuf           *pixbuf,
                              CheeseAvatarWidget  *widget)
{
  CheeseAvatarWidgetPrivate *priv = widget->priv;
  GtkAllocation               allocation;

  gtk_widget_get_allocation (priv->camera, &allocation);
  gtk_widget_set_size_request (priv->image, allocation.width, allocation.height);

  um_crop_area_set_picture (UM_CROP_AREA (priv->image), pixbuf);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), IMAGE_PAGE);

  gtk_widget_set_sensitive (priv->take_button, TRUE);

  g_object_notify_by_pspec (G_OBJECT (widget), properties[PROP_PIXBUF]);
}

/*
 * take_button_clicked_cb:
 * @button: the #GtkButton that was clicked
 * @widget: the #CheeseAvatarWidget
 *
 * Take a photo with the #CheeseCamera of @widget. When the photo has been
 * taken, call cheese_widget_photo_taken_cb().
 */
static void
take_button_clicked_cb (GtkButton           *button,
                        CheeseAvatarWidget *widget)
{
  CheeseAvatarWidgetPrivate *priv = widget->priv;
  GObject                    *camera;

  camera = cheese_widget_get_camera (CHEESE_WIDGET (priv->camera));
  if (priv->photo_taken_id == 0)
  {
    gtk_widget_set_sensitive (priv->take_button, FALSE);
    priv->photo_taken_id = g_signal_connect (G_OBJECT (camera), "photo-taken",
                                             G_CALLBACK (cheese_widget_photo_taken_cb), widget);
  }
  if (cheese_camera_take_photo_pixbuf (CHEESE_CAMERA (camera)))
  {
    cheese_flash_fire (CHEESE_FLASH (priv->flash));
    ca_gtk_play_for_widget (GTK_WIDGET (widget), 0,
                            CA_PROP_EVENT_ID, "camera-shutter",
                            CA_PROP_MEDIA_ROLE, "event",
                            CA_PROP_EVENT_DESCRIPTION, _("Shutter sound"),
                            NULL);
  }
  else
  {
    g_assert_not_reached ();
  }
}

/*
 * take_again_button_clicked_cb:
 * @button: the #GtkButton that was clicked
 * @widget: the #CheeseAvatarWidget
 *
 * Switch the @widget back to the camera view, ready to take another photo.
 */
static void
take_again_button_clicked_cb (GtkButton           *button,
                              CheeseAvatarWidget *widget)
{
  CheeseAvatarWidgetPrivate *priv = widget->priv;

  gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), WIDGET_PAGE);

  um_crop_area_set_picture (UM_CROP_AREA (priv->image), NULL);

  g_object_notify_by_pspec (G_OBJECT (widget), properties[PROP_PIXBUF]);
}

/* state_change_cb:
 * @object: the #CheeseWidget on which the state changed
 * @param_spec: the (unused) parameter specification
 * @widget: a #CheeseAvatarWidget
 *
 * Handle state changes on the #CheeseWidget, and update the UI appropriately.
 */
static void
state_change_cb (GObject             *object,
                 GParamSpec          *param_spec,
                 CheeseAvatarWidget  *widget)
{
  CheeseAvatarWidgetPrivate *priv = widget->priv;
  CheeseWidgetState           state;

  g_object_get (object, "state", &state, NULL);

  switch (state)
  {
    case CHEESE_WIDGET_STATE_READY:
      gtk_widget_set_sensitive (priv->take_button, TRUE);
      gtk_widget_set_sensitive (priv->take_again_button, TRUE);
      break;
    case CHEESE_WIDGET_STATE_ERROR:
      gtk_widget_set_sensitive (priv->take_button, FALSE);
      gtk_widget_set_sensitive (priv->take_again_button, FALSE);
      break;
    case CHEESE_WIDGET_STATE_NONE:
      break;
    default:
      g_assert_not_reached ();
  }
}

/*
 * create_page:
 * @child: the #CheeseWidget to pack into the container
 * @button: the #GtkButton for taking a photo
 *
 * Create the widgets for the #CheeseAvatarWidget and pack them into a
 * container.
 *
 * Returns: a #GtkBox containing the individual #CheeseAvatarWidget widgets
 */
static GtkWidget *
create_page (GtkWidget *child,
             GtkWidget *button)
{
  GtkWidget *vgrid, *bar;
  GtkStyleContext *context;

  vgrid = gtk_grid_new ();
  gtk_grid_attach (GTK_GRID (vgrid),
                   child, 0, 0, 1, 1);
  gtk_widget_set_hexpand (child, TRUE);
  gtk_widget_set_vexpand (child, TRUE);

  bar = gtk_header_bar_new ();
  context = gtk_widget_get_style_context (GTK_WIDGET (bar));
  gtk_style_context_remove_class (context, "header-bar");
  gtk_style_context_add_class (context, "inline-toolbar");
  gtk_style_context_add_class (context, "toolbar");
  gtk_style_context_add_class (context, GTK_STYLE_CLASS_HORIZONTAL);

  g_object_set (G_OBJECT (button), "margin-top", 6, "margin-bottom", 6, NULL);
  gtk_header_bar_set_custom_title (GTK_HEADER_BAR (bar), button);
  gtk_grid_attach (GTK_GRID (vgrid),
                   bar, 0, 1, 1, 1);

  return vgrid;
}

static void
cheese_avatar_widget_init (CheeseAvatarWidget *widget)
{
  GtkWidget *frame;
  GtkWidget *image;

  CheeseAvatarWidgetPrivate *priv = widget->priv = cheese_avatar_widget_get_instance_private (widget);

  priv->flash = cheese_flash_new (GTK_WIDGET (widget));

  priv->notebook = gtk_notebook_new ();
  g_object_set(G_OBJECT (priv->notebook), "margin", 12, NULL);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (priv->notebook), FALSE);
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (priv->notebook), FALSE);

  gtk_container_add (GTK_CONTAINER (widget), priv->notebook);

  /* Camera tab */
  priv->camera = cheese_widget_new ();
  g_signal_connect (G_OBJECT (priv->camera), "notify::state",
                    G_CALLBACK (state_change_cb), widget);
  image = gtk_image_new_from_icon_name ("camera-photo-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
  priv->take_button = gtk_button_new ();
  gtk_button_set_image (GTK_BUTTON (priv->take_button), image);
  g_signal_connect (G_OBJECT (priv->take_button), "clicked",
                    G_CALLBACK (take_button_clicked_cb), widget);
  gtk_widget_set_sensitive (priv->take_button, FALSE);
  gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook),
                            create_page (priv->camera, priv->take_button),
                            gtk_label_new ("webcam"));

  /* Image tab */
  priv->image = um_crop_area_new ();
  frame = gtk_frame_new (NULL);
  gtk_container_add (GTK_CONTAINER (frame), priv->image);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  priv->take_again_button = gtk_button_new_with_mnemonic (_("_Take Another Picture"));
  g_signal_connect (G_OBJECT (priv->take_again_button), "clicked",
                    G_CALLBACK (take_again_button_clicked_cb), widget);
  gtk_widget_set_sensitive (priv->take_again_button, FALSE);
  gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook),
                            create_page (frame, priv->take_again_button),
                            gtk_label_new ("image"));

  priv->sizegroup = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  gtk_size_group_add_widget (priv->sizegroup, priv->take_button);
  gtk_size_group_add_widget (priv->sizegroup, priv->take_again_button);

  gtk_widget_show_all (GTK_WIDGET (widget));
}

static void
cheese_avatar_widget_finalize (GObject *object)
{
  CheeseAvatarWidgetPrivate *priv = ((CheeseAvatarWidget *) object)->priv;

  g_clear_object (&priv->flash);
  g_clear_object (&priv->sizegroup);

  G_OBJECT_CLASS (cheese_avatar_widget_parent_class)->finalize (object);
}

static void
cheese_avatar_widget_get_property (GObject *object, guint prop_id,
                                    GValue *value, GParamSpec *pspec)
{
  CheeseAvatarWidgetPrivate *priv = ((CheeseAvatarWidget *) object)->priv;

  switch (prop_id)
  {
    case PROP_PIXBUF:
      g_value_set_object (value, um_crop_area_get_picture (UM_CROP_AREA (priv->image)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
cheese_avatar_widget_class_init (CheeseAvatarWidgetClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = cheese_avatar_widget_finalize;
  object_class->get_property = cheese_avatar_widget_get_property;

  /**
   * CheeseAvatarWidget:pixbuf:
   *
   * A #GdkPixbuf object representing the cropped area of the picture, or %NULL.
   */
  properties[PROP_PIXBUF] = g_param_spec_object ("pixbuf",
                                                 "Pixbuf",
                                                 "A #GdkPixbuf object representing the cropped area of the picture, or %NULL.",
                                                 GDK_TYPE_PIXBUF,
                                                 G_PARAM_READABLE);

  g_object_class_install_properties (object_class, PROP_LAST, properties);
}

/**
 * cheese_avatar_widget_new:
 *
 * Creates a new #CheeseAvatarWidget dialogue.
 *
 * Returns: a #CheeseAvatarWidget
 */
GtkWidget *
cheese_avatar_widget_new (void)
{
  return g_object_new (CHEESE_TYPE_AVATAR_WIDGET, NULL);
}

/**
 * cheese_avatar_widget_get_picture:
 * @widget: a #CheeseAvatarWidget dialogue
 *
 * Returns the portion of image selected through the builtin cropping tool,
 * after a picture has been captured on the webcam.
 *
 * Return value: a #GdkPixbuf object, or %NULL if no picture has been taken yet
 */
GdkPixbuf *
cheese_avatar_widget_get_picture (CheeseAvatarWidget *widget)
{
  g_return_val_if_fail (CHEESE_IS_AVATAR_WIDGET (widget), NULL);

  return um_crop_area_get_picture (UM_CROP_AREA (widget->priv->image));
}
