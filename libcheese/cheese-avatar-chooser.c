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
#include "cheese-widget.h"
#include "cheese-countdown.h"
#include "cheese-flash.h"
#include "cheese-avatar-chooser.h"

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
  GtkWidget *countdown;
  GdkPixbuf *pixbuf;
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

  gtk_widget_get_allocation (priv->camera, &allocation);
  gtk_widget_set_size_request (priv->image, allocation.width, allocation.height);

  gtk_image_set_from_pixbuf (GTK_IMAGE (priv->image), pixbuf);
  g_assert (priv->pixbuf == NULL);
  priv->pixbuf = g_object_ref (pixbuf);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), IMAGE_PAGE);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (chooser),
                                     GTK_RESPONSE_ACCEPT,
                                     TRUE);

  g_object_notify (G_OBJECT (chooser), "pixbuf");
}

static void
picture_cb (gpointer data)
{
  CheeseAvatarChooser        *chooser = CHEESE_AVATAR_CHOOSER (data);
  CheeseAvatarChooserPrivate *priv    = CHEESE_AVATAR_CHOOSER_GET_PRIVATE (data);
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
hide_cb (gpointer data)
{
  CheeseAvatarChooserPrivate *priv = CHEESE_AVATAR_CHOOSER_GET_PRIVATE (data);

  gtk_widget_hide (priv->countdown);
  gtk_widget_show (priv->take_button);
}

static void
take_button_clicked_cb (GtkButton           *button,
                        CheeseAvatarChooser *chooser)
{
  CheeseAvatarChooserPrivate *priv = CHEESE_AVATAR_CHOOSER_GET_PRIVATE (chooser);
  GtkAllocation               allocation;

  gtk_widget_get_allocation (priv->take_button, &allocation);
  gtk_widget_hide (priv->take_button);
  gtk_widget_set_size_request (priv->countdown, -1, allocation.height);
  gtk_widget_show (priv->countdown);
  cheese_countdown_start (CHEESE_COUNTDOWN (priv->countdown),
                          picture_cb,
                          hide_cb,
                          (gpointer) chooser);
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

  gtk_image_set_from_pixbuf (GTK_IMAGE (priv->image), NULL);
  g_object_unref (priv->pixbuf);
  priv->pixbuf = NULL;
  g_object_notify (G_OBJECT (chooser), "pixbuf");
}

static void
ready_cb (CheeseWidget        *widget,
          gboolean             is_ready,
          CheeseAvatarChooser *chooser)
{
  CheeseAvatarChooserPrivate *priv = CHEESE_AVATAR_CHOOSER_GET_PRIVATE (chooser);

  gtk_widget_set_sensitive (priv->take_button, is_ready);
  gtk_widget_set_sensitive (priv->take_again_button, is_ready);
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
  CheeseAvatarChooserPrivate *priv = CHEESE_AVATAR_CHOOSER_GET_PRIVATE (chooser);

  priv->flash = cheese_flash_new (GTK_WIDGET (chooser));

  gtk_dialog_add_buttons (GTK_DIALOG (chooser),
                          GTK_STOCK_CANCEL,
                          GTK_RESPONSE_REJECT,
                          "Select",
                          GTK_RESPONSE_ACCEPT,
                          NULL);
  gtk_window_set_title (GTK_WINDOW (chooser), _("Take a photo"));
  gtk_dialog_set_has_separator (GTK_DIALOG (chooser), FALSE);

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
  g_signal_connect (G_OBJECT (priv->camera), "ready",
                    G_CALLBACK (ready_cb), chooser);
  priv->take_button = gtk_button_new_with_mnemonic (_("_Take a photo"));
  g_signal_connect (G_OBJECT (priv->take_button), "clicked",
                    G_CALLBACK (take_button_clicked_cb), chooser);
  gtk_widget_set_sensitive (priv->take_button, FALSE);
  priv->countdown = cheese_countdown_new ();
  gtk_widget_set_no_show_all (priv->countdown, TRUE);
  gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook),
                            create_page (priv->camera, priv->take_button, priv->countdown),
                            gtk_label_new ("webcam"));

  /* Image tab */
  priv->image             = gtk_image_new ();
  priv->take_again_button = gtk_button_new_with_mnemonic (_("_Discard photo"));
  g_signal_connect (G_OBJECT (priv->take_again_button), "clicked",
                    G_CALLBACK (take_again_button_clicked_cb), chooser);
  gtk_widget_set_sensitive (priv->take_again_button, FALSE);
  gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook),
                            create_page (priv->image, priv->take_again_button, NULL),
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
  if (priv->pixbuf != NULL)
  {
    g_object_unref (priv->pixbuf);
    priv->pixbuf = NULL;
  }

  G_OBJECT_CLASS (cheese_avatar_chooser_parent_class)->finalize (object);
}

static void
cheese_avatar_chooser_response (GtkDialog *dialog, gint response_id)
{
  CheeseAvatarChooserPrivate *priv = CHEESE_AVATAR_CHOOSER_GET_PRIVATE (dialog);

  cheese_countdown_cancel (CHEESE_COUNTDOWN (priv->countdown));
}

static void
cheese_avatar_chooser_get_property (GObject *object, guint prop_id,
                                    GValue *value, GParamSpec *pspec)
{
  CheeseAvatarChooserPrivate *priv = CHEESE_AVATAR_CHOOSER_GET_PRIVATE (object);

  g_return_if_fail (CHEESE_IS_WIDGET (object));

  switch (prop_id)
  {
    case PROP_PIXBUF:
      g_value_set_object (value, priv->pixbuf);
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
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

  object_class->finalize     = cheese_avatar_chooser_finalize;
  dialog_class->response     = cheese_avatar_chooser_response;
  object_class->get_property = cheese_avatar_chooser_get_property;

  g_object_class_install_property (object_class, PROP_PIXBUF,
                                   g_param_spec_object ("pixbuf",
                                                        NULL,
                                                        NULL,
                                                        GDK_TYPE_PIXBUF,
                                                        G_PARAM_READABLE));

  g_type_class_add_private (klass, sizeof (CheeseAvatarChooserPrivate));
}

GtkWidget *
cheese_avatar_chooser_new (void)
{
  return g_object_new (CHEESE_TYPE_AVATAR_CHOOSER, NULL);
}

GdkPixbuf *
cheese_avatar_chooser_get_picture (CheeseAvatarChooser *chooser)
{
  CheeseAvatarChooserPrivate *priv;

  g_return_val_if_fail (CHEESE_IS_AVATAR_CHOOSER (chooser), NULL);

  priv = CHEESE_AVATAR_CHOOSER_GET_PRIVATE (chooser);

  if (priv->pixbuf == NULL)
    return NULL;

  return g_object_ref (priv->pixbuf);
}

/*
 * vim: sw=2 ts=8 cindent noai bs=2
 */
