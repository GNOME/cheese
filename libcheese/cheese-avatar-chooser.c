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

enum
{
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_PIXBUF
};

enum
{
  WIDGET_PAGE = 0,
  IMAGE_PAGE  = 1,
};

typedef struct
{
  GtkWidget *notebook;
  GtkWidget *camera;
  GtkWidget *image;
  GtkWidget *take_button;
  GtkWidget *take_again_button;
  CheeseFlash *flash;
  gulong photo_taken_id;
} CheeseAvatarChooserPrivate;

#define CHEESE_AVATAR_CHOOSER_GET_PRIVATE(o)                     \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHEESE_TYPE_AVATAR_CHOOSER, \
                                CheeseAvatarChooserPrivate))

G_DEFINE_TYPE (CheeseAvatarChooser, cheese_avatar_chooser, GTK_TYPE_DIALOG);

static void
cheese_widget_photo_taken_cb (CheeseCamera        *camera,
                              GdkPixbuf           *pixbuf,
                              CheeseAvatarChooser *chooser)
{
  CheeseAvatarChooserPrivate *priv = CHEESE_AVATAR_CHOOSER_GET_PRIVATE (chooser);
  GtkAllocation               allocation;

  gdk_threads_enter ();

  gtk_widget_get_allocation (priv->camera, &allocation);
  gtk_widget_set_size_request (priv->image, allocation.width, allocation.height);

  um_crop_area_set_picture (UM_CROP_AREA (priv->image), pixbuf);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), IMAGE_PAGE);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (chooser),
                                     GTK_RESPONSE_ACCEPT,
                                     TRUE);

  gdk_threads_leave ();

  g_object_notify (G_OBJECT (chooser), "pixbuf");
}

static void
take_button_clicked_cb (GtkButton           *button,
                        CheeseAvatarChooser *chooser)
{
  CheeseAvatarChooserPrivate *priv = CHEESE_AVATAR_CHOOSER_GET_PRIVATE (chooser);
  GObject                    *camera;

  camera = cheese_widget_get_camera (CHEESE_WIDGET (priv->camera));
  if (priv->photo_taken_id == 0)
  {
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

static void
take_again_button_clicked_cb (GtkButton           *button,
                              CheeseAvatarChooser *chooser)
{
  CheeseAvatarChooserPrivate *priv = CHEESE_AVATAR_CHOOSER_GET_PRIVATE (chooser);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), WIDGET_PAGE);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (chooser),
                                     GTK_RESPONSE_ACCEPT,
                                     FALSE);

  um_crop_area_set_picture (UM_CROP_AREA (priv->image), NULL);
  g_object_notify (G_OBJECT (chooser), "pixbuf");
}

static void
state_change_cb (GObject             *object,
                 GParamSpec          *param_spec,
                 CheeseAvatarChooser *chooser)
{
  CheeseAvatarChooserPrivate *priv = CHEESE_AVATAR_CHOOSER_GET_PRIVATE (chooser);
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

static GtkWidget *
create_page (GtkWidget *child,
             GtkWidget *button,
             GtkWidget *extra)
{
  GtkWidget *vbox, *hbox;

  vbox = gtk_vbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (vbox),
                      child,
                      TRUE,
                      TRUE,
                      0);
  hbox = gtk_hbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (vbox),
                      hbox,
                      FALSE,
                      TRUE,
                      0);
  gtk_box_pack_start (GTK_BOX (hbox),
                      button,
                      FALSE,
                      TRUE,
                      0);
  if (extra != NULL)
  {
    gtk_box_pack_start (GTK_BOX (hbox),
                        extra,
                        TRUE,
                        TRUE,
                        0);
  }

  return vbox;
}

static void
cheese_avatar_chooser_init (CheeseAvatarChooser *chooser)
{
  GtkWidget *frame;

  CheeseAvatarChooserPrivate *priv = CHEESE_AVATAR_CHOOSER_GET_PRIVATE (chooser);

  priv->flash = cheese_flash_new (GTK_WIDGET (chooser));

  gtk_dialog_add_buttons (GTK_DIALOG (chooser),
                          GTK_STOCK_CANCEL,
                          GTK_RESPONSE_REJECT,
                          "Select",
                          GTK_RESPONSE_ACCEPT,
                          NULL);
  gtk_window_set_title (GTK_WINDOW (chooser), _("Take a photo"));

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
  priv->take_button = gtk_button_new_with_mnemonic (_("_Take a photo"));
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
  CheeseAvatarChooserPrivate *priv = CHEESE_AVATAR_CHOOSER_GET_PRIVATE (object);

  if (priv->flash != NULL)
  {
    g_object_unref (priv->flash);
    priv->flash = NULL;
  }

  G_OBJECT_CLASS (cheese_avatar_chooser_parent_class)->finalize (object);
}

static void
cheese_avatar_chooser_get_property (GObject *object, guint prop_id,
                                    GValue *value, GParamSpec *pspec)
{
  CheeseAvatarChooserPrivate *priv = CHEESE_AVATAR_CHOOSER_GET_PRIVATE (object);

  g_return_if_fail (CHEESE_IS_AVATAR_CHOOSER (object));

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
   * a #GdkPixbuf object representing the cropped area of the picture, or %NULL.
   *
   **/
  g_object_class_install_property (object_class, PROP_PIXBUF,
                                   g_param_spec_object ("pixbuf",
                                                        "pixbuf",
                                                        "a GdkPixbuf object",
                                                        GDK_TYPE_PIXBUF,
                                                        G_PARAM_READABLE));

  g_type_class_add_private (klass, sizeof (CheeseAvatarChooserPrivate));
}

/**
 * cheese_avatar_chooser_new:
 *
 * Returns a new #CheeseAvatarChooser dialogue.
 *
 * Return value: a #CheeseAvatarChooser widget.
 **/
GtkWidget *
cheese_avatar_chooser_new (void)
{
  return g_object_new (CHEESE_TYPE_AVATAR_CHOOSER, NULL);
}

/**
 * cheese_avatar_chooser_get_picture:
 * @chooser: a #CheeseAvatarChooser dialogue.
 *
 * Returns the portion of image selected through the builtin
 * cropping tool, after a picture has been captured on the webcam.
 *
 * Return value: a #GdkPixbuf object, or %NULL if no picture has been taken yet.
 **/
GdkPixbuf *
cheese_avatar_chooser_get_picture (CheeseAvatarChooser *chooser)
{
  CheeseAvatarChooserPrivate *priv;

  g_return_val_if_fail (CHEESE_IS_AVATAR_CHOOSER (chooser), NULL);

  priv = CHEESE_AVATAR_CHOOSER_GET_PRIVATE (chooser);

  return um_crop_area_get_picture (UM_CROP_AREA (priv->image));
}

/*
 * vim: sw=2 ts=8 cindent noai bs=2
 */
