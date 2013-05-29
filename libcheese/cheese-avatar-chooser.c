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

#include <glib/gi18n.h>
#include <canberra-gtk.h>

#include "cheese-camera.h"
#include "cheese-widget-private.h"
#include "cheese-flash.h"
#include "cheese-avatar-chooser.h"
#include "um-crop-area.h"

/**
 * SECTION:cheese-avatar-chooser
 * @short_description: A photo capture dialog for avatars
 * @stability: Unstable
 * @include: cheese/cheese-avatar-chooser.h
 *
 * #CheeseAvatarChooser presents a simple window to the user for taking a photo
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

struct _CheeseAvatarChooserPrivate
{
  GtkWidget *notebook;
  GtkWidget *camera;
  GtkWidget *image;
  GtkWidget *take_button;
  GtkWidget *take_again_button;
  CheeseFlash *flash;
  gulong photo_taken_id;
};

static GParamSpec *properties[PROP_LAST];

#define CHEESE_AVATAR_CHOOSER_GET_PRIVATE(o)                     \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHEESE_TYPE_AVATAR_CHOOSER, \
                                CheeseAvatarChooserPrivate))

G_DEFINE_TYPE (CheeseAvatarChooser, cheese_avatar_chooser, GTK_TYPE_DIALOG);

/*
 * cheese_widget_photo_taken_cb:
 * @camera: a #CheeseCamera
 * @pixbuf: the #GdkPixbuf of the image that was just taken
 * @choose: a #CheeseAvatarChooser
 *
 * Show the image that was just taken from the camera (as @pixbuf) in the
 * cropping tool.
 */
static void
cheese_widget_photo_taken_cb (CheeseCamera        *camera,
                              GdkPixbuf           *pixbuf,
                              CheeseAvatarChooser *chooser)
{
  CheeseAvatarChooserPrivate *priv = chooser->priv;
  GtkAllocation               allocation;

  gdk_threads_enter ();

  gtk_widget_get_allocation (priv->camera, &allocation);
  gtk_widget_set_size_request (priv->image, allocation.width, allocation.height);

  um_crop_area_set_picture (UM_CROP_AREA (priv->image), pixbuf);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), IMAGE_PAGE);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (chooser),
                                     GTK_RESPONSE_ACCEPT,
                                     TRUE);
  gtk_widget_set_sensitive (priv->take_button, TRUE);

  gdk_threads_leave ();

  g_object_notify_by_pspec (G_OBJECT (chooser), properties[PROP_PIXBUF]);
}

/*
 * take_button_clicked_cb:
 * @button: the #GtkButton that was clicked
 * @chooser: the #CheeseAvatarChooser
 *
 * Take a photo with the #CheeseCamera of @chooser. When the photo has been
 * taken, call cheese_widget_photo_taken_cb().
 */
static void
take_button_clicked_cb (GtkButton           *button,
                        CheeseAvatarChooser *chooser)
{
  CheeseAvatarChooserPrivate *priv = chooser->priv;
  GObject                    *camera;

  camera = cheese_widget_get_camera (CHEESE_WIDGET (priv->camera));
  if (priv->photo_taken_id == 0)
  {
    gtk_widget_set_sensitive (priv->take_button, FALSE);
    priv->photo_taken_id = g_signal_connect (G_OBJECT (camera), "photo-taken",
                                             G_CALLBACK (cheese_widget_photo_taken_cb), chooser);
  }
  if (cheese_camera_take_photo_pixbuf (CHEESE_CAMERA (camera)))
  {
    cheese_flash_fire (CHEESE_FLASH (priv->flash));
    ca_gtk_play_for_widget (GTK_WIDGET (chooser), 0,
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
 * @chooser: the #CheeseAvatarChooser
 *
 * Switch the @chooser back to the camera view, ready to take another photo.
 */
static void
take_again_button_clicked_cb (GtkButton           *button,
                              CheeseAvatarChooser *chooser)
{
  CheeseAvatarChooserPrivate *priv = chooser->priv;

  gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), WIDGET_PAGE);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (chooser),
                                     GTK_RESPONSE_ACCEPT,
                                     FALSE);

  um_crop_area_set_picture (UM_CROP_AREA (priv->image), NULL);

  g_object_notify_by_pspec (G_OBJECT (chooser), properties[PROP_PIXBUF]);
}

/* state_change_cb:
 * @object: the #CheeseWidget on which the state changed
 * @param_spec: the (unused) parameter specification
 * @chooser: a #CheeseAvatarChooser
 *
 * Handle state changes on the #CheeseWidget, and update the UI appropriately.
 */
static void
state_change_cb (GObject             *object,
                 GParamSpec          *param_spec,
                 CheeseAvatarChooser *chooser)
{
  CheeseAvatarChooserPrivate *priv = chooser->priv;
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
 * @extra: an extra #GtkWidget to pack alongside the @button, or NULL
 *
 * Create the widgets for the #CheeseAvatarChooser and pack them into a
 * container.
 *
 * Returns: a #GtkBox containing the individual #CheeseAvatarChooser widgets
 */
static GtkWidget *
create_page (GtkWidget *child,
             GtkWidget *button,
             GtkWidget *extra)
{
  GtkWidget *vgrid, *hgrid, *align;

  vgrid = gtk_grid_new ();
  gtk_grid_attach (GTK_GRID (vgrid),
                   child, 0, 0, 1, 1);
  gtk_widget_set_hexpand (child, TRUE);
  gtk_widget_set_vexpand (child, TRUE);
  align = gtk_alignment_new (0.5, 0, 0, 0);
  gtk_widget_set_valign (align, GTK_ALIGN_END);
  gtk_grid_attach (GTK_GRID (vgrid),
                   align, 0, 1, 1, 1);
  hgrid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (hgrid), 8);
  gtk_container_add(GTK_CONTAINER(align), hgrid);
  gtk_grid_attach (GTK_GRID (hgrid),
                   button, 0, 0, 1, 1);
  gtk_widget_set_halign (button, GTK_ALIGN_START);
  if (extra != NULL)
  {
    gtk_grid_attach (GTK_GRID (hgrid),
                     extra, 1, 0, 1, 1);
    gtk_widget_set_hexpand (extra, TRUE);
    gtk_widget_set_vexpand (extra, TRUE);
  }

  return vgrid;
}

static void
cheese_avatar_chooser_init (CheeseAvatarChooser *chooser)
{
  GtkWidget *frame;

  CheeseAvatarChooserPrivate *priv = chooser->priv = CHEESE_AVATAR_CHOOSER_GET_PRIVATE (chooser);

  priv->flash = cheese_flash_new (GTK_WIDGET (chooser));

  gtk_dialog_add_buttons (GTK_DIALOG (chooser),
                          GTK_STOCK_CANCEL,
                          GTK_RESPONSE_REJECT,
                          _("Select"),
                          GTK_RESPONSE_ACCEPT,
                          NULL);
  gtk_window_set_title (GTK_WINDOW (chooser), _("Take a Photo"));

  gtk_dialog_set_response_sensitive (GTK_DIALOG (chooser),
                                     GTK_RESPONSE_ACCEPT,
                                     FALSE);

  priv->notebook = gtk_notebook_new ();
  gtk_notebook_set_show_border (GTK_NOTEBOOK (priv->notebook), FALSE);
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (priv->notebook), FALSE);

  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (chooser))),
                      priv->notebook,
                      TRUE, TRUE, 8);

  /* Camera tab */
  priv->camera = cheese_widget_new ();
  g_signal_connect (G_OBJECT (priv->camera), "notify::state",
                    G_CALLBACK (state_change_cb), chooser);
  priv->take_button = gtk_button_new_with_mnemonic (_("_Take a Photo"));
  g_signal_connect (G_OBJECT (priv->take_button), "clicked",
                    G_CALLBACK (take_button_clicked_cb), chooser);
  gtk_widget_set_sensitive (priv->take_button, FALSE);
  gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook),
                            create_page (priv->camera, priv->take_button, NULL),
                            gtk_label_new ("webcam"));

  /* Image tab */
  priv->image = um_crop_area_new ();
  frame       = gtk_frame_new (NULL);
  gtk_container_add (GTK_CONTAINER (frame), priv->image);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  priv->take_again_button = gtk_button_new_with_mnemonic (_("_Discard photo"));
  g_signal_connect (G_OBJECT (priv->take_again_button), "clicked",
                    G_CALLBACK (take_again_button_clicked_cb), chooser);
  gtk_widget_set_sensitive (priv->take_again_button, FALSE);
  gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook),
                            create_page (frame, priv->take_again_button, NULL),
                            gtk_label_new ("image"));

  gtk_window_set_default_size (GTK_WINDOW (chooser), 400, 300);

  gtk_widget_show_all (gtk_dialog_get_content_area (GTK_DIALOG (chooser)));
}

static void
cheese_avatar_chooser_finalize (GObject *object)
{
  CheeseAvatarChooserPrivate *priv = ((CheeseAvatarChooser *) object)->priv;

  g_clear_object (&priv->flash);

  G_OBJECT_CLASS (cheese_avatar_chooser_parent_class)->finalize (object);
}

static void
cheese_avatar_chooser_get_property (GObject *object, guint prop_id,
                                    GValue *value, GParamSpec *pspec)
{
  CheeseAvatarChooserPrivate *priv = ((CheeseAvatarChooser *) object)->priv;

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
cheese_avatar_chooser_class_init (CheeseAvatarChooserClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = cheese_avatar_chooser_finalize;
  object_class->get_property = cheese_avatar_chooser_get_property;

  /**
   * CheeseAvatarChooser:pixbuf:
   *
   * A #GdkPixbuf object representing the cropped area of the picture, or %NULL.
   */
  properties[PROP_PIXBUF] = g_param_spec_object ("pixbuf",
                                                 "Pixbuf",
                                                 "A #GdkPixbuf object representing the cropped area of the picture, or %NULL.",
                                                 GDK_TYPE_PIXBUF,
                                                 G_PARAM_READABLE);

  g_object_class_install_properties (object_class, PROP_LAST, properties);

  g_type_class_add_private (klass, sizeof (CheeseAvatarChooserPrivate));
}

/**
 * cheese_avatar_chooser_new:
 *
 * Creates a new #CheeseAvatarChooser dialogue.
 *
 * Returns: a #CheeseAvatarChooser
 */
GtkWidget *
cheese_avatar_chooser_new (void)
{
  return g_object_new (CHEESE_TYPE_AVATAR_CHOOSER, NULL);
}

/**
 * cheese_avatar_chooser_get_picture:
 * @chooser: a #CheeseAvatarChooser dialogue
 *
 * Returns the portion of image selected through the builtin cropping tool,
 * after a picture has been captured on the webcam.
 *
 * Return value: a #GdkPixbuf object, or %NULL if no picture has been taken yet
 */
GdkPixbuf *
cheese_avatar_chooser_get_picture (CheeseAvatarChooser *chooser)
{
  g_return_val_if_fail (CHEESE_IS_AVATAR_CHOOSER (chooser), NULL);

  return um_crop_area_get_picture (UM_CROP_AREA (chooser->priv->image));
}
