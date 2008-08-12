/* Eye Of Gnome - Thumbnail Navigator
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <cheese-config.h>
#endif

#include "eog-thumb-nav.h"
#include "cheese-thumb-view.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <string.h>

#define EOG_THUMB_NAV_GET_PRIVATE(object) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((object), EOG_TYPE_THUMB_NAV, EogThumbNavPrivate))

G_DEFINE_TYPE (EogThumbNav, eog_thumb_nav, GTK_TYPE_HBOX);

#define EOG_THUMB_NAV_SCROLL_INC      20
#define EOG_THUMB_NAV_SCROLL_MOVE     20
#define EOG_THUMB_NAV_SCROLL_TIMEOUT  20 

enum
{
	PROP_SHOW_BUTTONS = 1,
	PROP_THUMB_VIEW,
	PROP_MODE
};

struct _EogThumbNavPrivate {
	gboolean          show_buttons;
	gboolean          scroll_dir;
	gint              scroll_pos;
	gint              scroll_id;

	GtkWidget        *button_left;
	GtkWidget        *button_right;
	GtkWidget        *sw;
	GtkWidget        *thumbview;
	GtkAdjustment    *adj;
};

static gboolean
eog_thumb_nav_scroll_event (GtkWidget *widget, GdkEventScroll *event, gpointer user_data)
{
	EogThumbNav *nav = EOG_THUMB_NAV (user_data);
	gint inc = EOG_THUMB_NAV_SCROLL_INC * 3;

	switch (event->direction) {
	case GDK_SCROLL_UP:
	case GDK_SCROLL_LEFT:
		inc *= -1;	
		break;

	case GDK_SCROLL_DOWN:
	case GDK_SCROLL_RIGHT:
		break;

	default:
		g_assert_not_reached ();
		return FALSE;
	}

	if (inc < 0)
		nav->priv->adj->value = MAX (0, nav->priv->adj->value + inc);
	else
		nav->priv->adj->value = MIN (nav->priv->adj->upper - nav->priv->adj->page_size, nav->priv->adj->value + inc);

	gtk_adjustment_value_changed (nav->priv->adj);

	return TRUE;
}

static void
eog_thumb_nav_adj_changed (GtkAdjustment *adj, gpointer user_data)
{
	EogThumbNav *nav;
	EogThumbNavPrivate *priv;

	nav = EOG_THUMB_NAV (user_data);
	priv = EOG_THUMB_NAV_GET_PRIVATE (nav);

	gtk_widget_set_sensitive (priv->button_right,
				  adj->value < adj->upper - adj->page_size);
}

static void
eog_thumb_nav_adj_value_changed (GtkAdjustment *adj, gpointer user_data)
{
	EogThumbNav *nav;
	EogThumbNavPrivate *priv;

	nav = EOG_THUMB_NAV (user_data);
	priv = EOG_THUMB_NAV_GET_PRIVATE (nav);

	gtk_widget_set_sensitive (priv->button_left, adj->value > 0);

	gtk_widget_set_sensitive (priv->button_right, 
				  adj->value < adj->upper - adj->page_size);
}

static gboolean
eog_thumb_nav_scroll_step (gpointer user_data)
{
	EogThumbNav *nav = EOG_THUMB_NAV (user_data);
	gint delta;

	if (nav->priv->scroll_pos < 10)
		delta = EOG_THUMB_NAV_SCROLL_INC;
	else if (nav->priv->scroll_pos < 20)
		delta = EOG_THUMB_NAV_SCROLL_INC * 2;
	else if (nav->priv->scroll_pos < 30)
		delta = EOG_THUMB_NAV_SCROLL_INC * 2 + 5;
	else
		delta = EOG_THUMB_NAV_SCROLL_INC * 2 + 12;

	if (!nav->priv->scroll_dir)
		delta *= -1;

	if ((gint) (nav->priv->adj->value + (gdouble) delta) >= 0 &&
	    (gint) (nav->priv->adj->value + (gdouble) delta) <= nav->priv->adj->upper - nav->priv->adj->page_size) {
		nav->priv->adj->value += (gdouble) delta; 
		nav->priv->scroll_pos++;
		gtk_adjustment_value_changed (nav->priv->adj);
	} else {
		if (delta > 0)
		      nav->priv->adj->value = nav->priv->adj->upper - nav->priv->adj->page_size;
		else
		      nav->priv->adj->value = 0;

		nav->priv->scroll_pos = 0;
		
		gtk_adjustment_value_changed (nav->priv->adj);

		return FALSE;
	}
	
	return TRUE;
}

static void
eog_thumb_nav_button_clicked (GtkButton *button, EogThumbNav *nav)
{
	nav->priv->scroll_pos = 0;

	nav->priv->scroll_dir = (GTK_WIDGET (button) == nav->priv->button_right);

	eog_thumb_nav_scroll_step (nav);
}

static void
eog_thumb_nav_start_scroll (GtkButton *button, EogThumbNav *nav)
{
	nav->priv->scroll_dir = (GTK_WIDGET (button) == nav->priv->button_right);

	nav->priv->scroll_id = g_timeout_add (EOG_THUMB_NAV_SCROLL_TIMEOUT, 
					      eog_thumb_nav_scroll_step, 
					      nav);
}

static void
eog_thumb_nav_stop_scroll (GtkButton *button, EogThumbNav *nav)
{
	if (nav->priv->scroll_id > 0) {
		g_source_remove (nav->priv->scroll_id);
		nav->priv->scroll_id = 0;
		nav->priv->scroll_pos = 0;
	}
}

static void 
eog_thumb_nav_get_property (GObject    *object, 
			    guint       property_id, 
			    GValue     *value, 
			    GParamSpec *pspec)
{
	EogThumbNav *nav = EOG_THUMB_NAV (object);

	switch (property_id)
	{
	case PROP_SHOW_BUTTONS:
		g_value_set_boolean (value, 
			eog_thumb_nav_get_show_buttons (nav));
		break;

	case PROP_THUMB_VIEW:
		g_value_set_object (value, nav->priv->thumbview);
		break;
	}
}

static void 
eog_thumb_nav_set_property (GObject      *object, 
			    guint         property_id, 
			    const GValue *value, 
			    GParamSpec   *pspec)
{
	EogThumbNav *nav = EOG_THUMB_NAV (object);

	switch (property_id)
	{
	case PROP_SHOW_BUTTONS:
		eog_thumb_nav_set_show_buttons (nav, 
			g_value_get_boolean (value));
		break;

	case PROP_THUMB_VIEW:
		nav->priv->thumbview = 
			GTK_WIDGET (g_value_get_object (value));
		break;
	}
}

static GObject *
eog_thumb_nav_constructor (GType type,
			   guint n_construct_properties,
			   GObjectConstructParam *construct_params)
{
	GObject *object;
	EogThumbNavPrivate *priv;

	object = G_OBJECT_CLASS (eog_thumb_nav_parent_class)->constructor
			(type, n_construct_properties, construct_params);

	priv = EOG_THUMB_NAV (object)->priv;

	if (priv->thumbview != NULL) {
		gtk_container_add (GTK_CONTAINER (priv->sw), priv->thumbview);
		gtk_widget_show_all (priv->sw);
	}

	return object;
}

static void
eog_thumb_nav_class_init (EogThumbNavClass *class)
{
	GObjectClass *g_object_class = (GObjectClass *) class;

	g_object_class->constructor  = eog_thumb_nav_constructor;
	g_object_class->get_property = eog_thumb_nav_get_property;
	g_object_class->set_property = eog_thumb_nav_set_property;

	g_object_class_install_property (g_object_class,
	                                 PROP_SHOW_BUTTONS,
	                                 g_param_spec_boolean ("show-buttons",
	                                                       "Show Buttons",
	                                                       "Whether to show navigation buttons or not",
	                                                       TRUE,
	                                                       (G_PARAM_READABLE | G_PARAM_WRITABLE)));

	g_object_class_install_property (g_object_class,
	                                 PROP_THUMB_VIEW,
	                                 g_param_spec_object ("thumbview",
	                                                       "Thumbnail View",
	                                                       "The internal thumbnail viewer widget",
	                                                       CHEESE_TYPE_THUMB_VIEW,
	                                                       (G_PARAM_CONSTRUCT_ONLY |
								G_PARAM_READABLE | 
								G_PARAM_WRITABLE)));

	g_type_class_add_private (g_object_class, sizeof (EogThumbNavPrivate));
}

static void
eog_thumb_nav_init (EogThumbNav *nav)
{
	EogThumbNavPrivate *priv;
	GtkWidget *arrow;

	nav->priv = EOG_THUMB_NAV_GET_PRIVATE (nav);

	priv = nav->priv;

	priv->show_buttons = TRUE;

	priv->button_left = gtk_button_new ();
	gtk_button_set_relief (GTK_BUTTON (priv->button_left), GTK_RELIEF_NONE);

	arrow = gtk_arrow_new (GTK_ARROW_LEFT, GTK_SHADOW_ETCHED_IN); 
	gtk_container_add (GTK_CONTAINER (priv->button_left), arrow);

	gtk_widget_set_size_request (GTK_WIDGET (priv->button_left), 25, 0);

	gtk_box_pack_start (GTK_BOX (nav), priv->button_left, FALSE, FALSE, 0);

	g_signal_connect (priv->button_left, 
			  "clicked", 
			  G_CALLBACK (eog_thumb_nav_button_clicked), 
			  nav);

	g_signal_connect (priv->button_left, 
			  "pressed", 
			  G_CALLBACK (eog_thumb_nav_start_scroll), 
			  nav);

	g_signal_connect (priv->button_left, 
			  "released", 
			  G_CALLBACK (eog_thumb_nav_stop_scroll), 
			  nav);

	priv->sw = gtk_scrolled_window_new (NULL, NULL);

	gtk_widget_set_name (GTK_SCROLLED_WINDOW (priv->sw)->hscrollbar, 
			     "eog-image-collection-scrollbar");

	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (priv->sw), 
					     GTK_SHADOW_IN);

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_NEVER);

	g_signal_connect (priv->sw, 
			  "scroll-event", 
			  G_CALLBACK (eog_thumb_nav_scroll_event), 
			  nav);

	priv->adj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (priv->sw));

	g_signal_connect (priv->adj, 
			  "changed", 
			  G_CALLBACK (eog_thumb_nav_adj_changed), 
			  nav);

	g_signal_connect (priv->adj, 
			  "value-changed", 
			  G_CALLBACK (eog_thumb_nav_adj_value_changed), 
			  nav);

	gtk_box_pack_start (GTK_BOX (nav), priv->sw, TRUE, TRUE, 0);

	priv->button_right = gtk_button_new ();
	gtk_button_set_relief (GTK_BUTTON (priv->button_right), GTK_RELIEF_NONE);

	arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_NONE); 
	gtk_container_add (GTK_CONTAINER (priv->button_right), arrow);

	gtk_widget_set_size_request (GTK_WIDGET (priv->button_right), 25, 0);

	gtk_box_pack_start (GTK_BOX (nav), priv->button_right, FALSE, FALSE, 0);

	g_signal_connect (priv->button_right, 
			  "clicked", 
			  G_CALLBACK (eog_thumb_nav_button_clicked), 
			  nav);

	g_signal_connect (priv->button_right, 
			  "pressed", 
			  G_CALLBACK (eog_thumb_nav_start_scroll), 
			  nav);

	g_signal_connect (priv->button_right, 
			  "released", 
			  G_CALLBACK (eog_thumb_nav_stop_scroll), 
			  nav);

	gtk_adjustment_value_changed (priv->adj);
}

/**
 * eog_thumb_nav_new:
 * @thumbview: a #CheeseThumbView to embed in the navigation widget.
 * @mode: The navigation mode.
 * @show_buttons: Whether to show the navigation buttons.
 *
 * Creates a new thumbnail navigation widget.
 *
 * Returns: a new #EogThumbNav object.
 **/
GtkWidget *
eog_thumb_nav_new (GtkWidget       *thumbview, 
		   gboolean         show_buttons)
{
	EogThumbNav *nav;
	EogThumbNavPrivate *priv;

	nav = g_object_new (EOG_TYPE_THUMB_NAV, 
	                    "show-buttons", show_buttons,
	                    "thumbview", thumbview,
	                    "homogeneous", FALSE,
	                    "spacing", 0,
	                    NULL); 

	priv = nav->priv;

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_NEVER);

	eog_thumb_nav_set_show_buttons (nav, priv->show_buttons);

	return GTK_WIDGET (nav);
}

/**
 * eog_thumb_nav_get_show_buttons:
 * @nav: an #EogThumbNav.
 *
 * Gets whether the navigation buttons are visible.
 *
 * Returns: %TRUE if the navigation buttons are visible,
 * %FALSE otherwise.
 **/
gboolean
eog_thumb_nav_get_show_buttons (EogThumbNav *nav)
{
	g_return_val_if_fail (EOG_IS_THUMB_NAV (nav), FALSE);

	return nav->priv->show_buttons; 
}

/**
 * eog_thumb_nav_set_show_buttons:
 * @nav: an #EogThumbNav.
 * @show_buttons: %TRUE to show the buttons, %FALSE to hide them.
 *
 * Sets whether the navigation buttons to the left and right of the
 * widget should be visible.
 **/
void 
eog_thumb_nav_set_show_buttons (EogThumbNav *nav, gboolean show_buttons)
{
	g_return_if_fail (EOG_IS_THUMB_NAV (nav));
	g_return_if_fail (nav->priv->button_left  != NULL);
	g_return_if_fail (nav->priv->button_right != NULL);

	nav->priv->show_buttons = show_buttons;

	if (show_buttons) {
		gtk_widget_show_all (nav->priv->button_left);
		gtk_widget_show_all (nav->priv->button_right);
	} else {
		gtk_widget_hide_all (nav->priv->button_left);
		gtk_widget_hide_all (nav->priv->button_right);
	}
}
