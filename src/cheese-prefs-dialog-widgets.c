/*
 * Copyright © 2008 James Liggett <jrliggett@cox.net>
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

#include "cheese-prefs-dialog-widgets.h"

typedef struct
{
  GList *widgets;
  CheeseGConf *gconf;
} CheesePrefsDialogWidgetsPrivate;

#define CHEESE_PREFS_DIALOG_WIDGETS_GET_PRIVATE(o)                     \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHEESE_TYPE_PREFS_DIALOG_WIDGETS, \
                                CheesePrefsDialogWidgetsPrivate))

G_DEFINE_TYPE (CheesePrefsDialogWidgets, cheese_prefs_dialog_widgets, G_TYPE_OBJECT);

static void
cheese_prefs_dialog_widgets_init (CheesePrefsDialogWidgets *self)
{
  CheesePrefsDialogWidgetsPrivate *priv = CHEESE_PREFS_DIALOG_WIDGETS_GET_PRIVATE (self);

  priv->widgets = NULL;
  priv->gconf   = NULL;
}

static void
cheese_prefs_dialog_widgets_finalize (GObject *object)
{
  CheesePrefsDialogWidgetsPrivate *priv = CHEESE_PREFS_DIALOG_WIDGETS_GET_PRIVATE (object);

  GList *current_widget;

  current_widget = priv->widgets;

  while (current_widget)
  {
    g_object_unref (current_widget->data);
    current_widget = g_list_next (current_widget);
  }

  g_list_free (priv->widgets);

  G_OBJECT_CLASS (cheese_prefs_dialog_widgets_parent_class)->finalize (object);
}

static void
cheese_prefs_dialog_widgets_class_init (CheesePrefsDialogWidgetsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CheesePrefsDialogWidgetsPrivate));

  object_class->finalize = cheese_prefs_dialog_widgets_finalize;
}

CheesePrefsDialogWidgets *
cheese_prefs_dialog_widgets_new (CheeseGConf *gconf)
{
  CheesePrefsDialogWidgets        *self;
  CheesePrefsDialogWidgetsPrivate *priv;

  self = g_object_new (CHEESE_TYPE_PREFS_DIALOG_WIDGETS, NULL);

  priv        = CHEESE_PREFS_DIALOG_WIDGETS_GET_PRIVATE (self);
  priv->gconf = gconf;

  return self;
}

void
cheese_prefs_dialog_widgets_add (CheesePrefsDialogWidgets *prefs_widgets,
                                 CheesePrefsWidget        *widget)
{
  CheesePrefsDialogWidgetsPrivate *priv = CHEESE_PREFS_DIALOG_WIDGETS_GET_PRIVATE (prefs_widgets);

  if (!g_list_find (priv->widgets, widget))
  {
    priv->widgets = g_list_append (priv->widgets, widget);
    widget->gconf = priv->gconf;
  }
}

void
cheese_prefs_dialog_widgets_synchronize (CheesePrefsDialogWidgets *prefs_widgets)
{
  CheesePrefsDialogWidgetsPrivate *priv = CHEESE_PREFS_DIALOG_WIDGETS_GET_PRIVATE (prefs_widgets);

  GList *current_widget;

  current_widget = priv->widgets;

  while (current_widget)
  {
    cheese_prefs_widget_synchronize (CHEESE_PREFS_WIDGET (current_widget->data));
    current_widget = g_list_next (current_widget);
  }
}
