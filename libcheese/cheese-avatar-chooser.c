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
#include "cheese-avatar-chooser.h"
#include "cheese-avatar-widget.h"
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
  GtkWidget *widget;
};

static GParamSpec *properties[PROP_LAST];

G_DEFINE_TYPE_WITH_PRIVATE (CheeseAvatarChooser, cheese_avatar_chooser, GTK_TYPE_DIALOG);

static void
update_select_button (CheeseAvatarWidget  *widget,
                      GParamSpec          *pspec,
                      CheeseAvatarChooser *chooser)
{
  GdkPixbuf *pixbuf;

  g_object_get (G_OBJECT (widget), "pixbuf", &pixbuf, NULL);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (chooser),
                                     GTK_RESPONSE_ACCEPT,
                                     pixbuf != NULL);
  if (pixbuf)
    g_object_unref (pixbuf);
}

static void
cheese_avatar_chooser_init (CheeseAvatarChooser *chooser)
{
  GtkWidget *button;
  CheeseAvatarChooserPrivate *priv = chooser->priv = cheese_avatar_chooser_get_instance_private (chooser);

  gtk_dialog_add_buttons (GTK_DIALOG (chooser),
                          _("_Cancel"),
                          GTK_RESPONSE_CANCEL,
                          _("_Select"),
                          GTK_RESPONSE_ACCEPT,
                          NULL);
  gtk_window_set_title (GTK_WINDOW (chooser), _("Take a Photo"));

  button = gtk_dialog_get_widget_for_response (GTK_DIALOG (chooser),
                                               GTK_RESPONSE_ACCEPT);
  gtk_style_context_add_class (gtk_widget_get_style_context (button),
                               GTK_STYLE_CLASS_SUGGESTED_ACTION);

  gtk_dialog_set_response_sensitive (GTK_DIALOG (chooser),
                                     GTK_RESPONSE_ACCEPT,
                                     FALSE);

  priv->widget = cheese_avatar_widget_new ();
  gtk_widget_show (priv->widget);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (chooser))),
                      priv->widget,
                      TRUE, TRUE, 0);

  g_signal_connect (G_OBJECT (priv->widget), "notify::pixbuf",
                    G_CALLBACK (update_select_button), chooser);

}

static void
cheese_avatar_chooser_get_property (GObject *object, guint prop_id,
                                    GValue *value, GParamSpec *pspec)
{
  CheeseAvatarChooserPrivate *priv = ((CheeseAvatarChooser *) object)->priv;

  switch (prop_id)
  {
    case PROP_PIXBUF:
      g_value_set_object (value, cheese_avatar_widget_get_picture (CHEESE_AVATAR_WIDGET (priv->widget)));
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
}

static gboolean
dialogs_use_header (void)
{
    GtkSettings *settings;
    gboolean use_header;

    settings = gtk_settings_get_default ();

    g_object_get (G_OBJECT (settings),
                  "gtk-dialogs-use-header", &use_header,
                  NULL);

    return use_header;
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
  return g_object_new (CHEESE_TYPE_AVATAR_CHOOSER, "use-header-bar",
                       dialogs_use_header (), NULL);
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

  return cheese_avatar_widget_get_picture (CHEESE_AVATAR_WIDGET (chooser->priv->widget));
}
