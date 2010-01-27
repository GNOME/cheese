/*
 * Copyright © 2008-2010 Filippo Argiolas <filippo.argiolas@gmail.com>
 * Copyright © 2007-2009 daniel g. siegel <dgsiegel@gnome.org>
 * Copyright © 2007,2008 Jaap Haitsma <jaap@haitsma.org>
 * Copyright © 2008 Patryk Zawadzki <patrys@pld-linux.org>
 * Copyright © 2008 Ryan Zeigler <zeiglerr@gmail.com>
 * Copyright © 2008 Felix Kaser <f.kaser@gmx.net>
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

#ifdef HAVE_CONFIG_H
  #include "cheese-config.h"
#endif

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include <gst/interfaces/xoverlay.h>
#include <gtk/gtk.h>
#include <canberra-gtk.h>

#include "cheese-window.h"
#include "cheese-commands.h"
#include "cheese-ui.h"

#include "cheese-countdown.h"
#include "cheese-effect-chooser.h"
#include "eog-thumb-nav.h"
#include "cheese-no-camera.h"
#include "cheese-prefs-dialog.h"
#include "cheese-flash.h"

#define FULLSCREEN_POPUP_HEIGHT    40
#define FULLSCREEN_TIMEOUT         5 * 1000
#define FULLSCREEN_EFFECTS_TIMEOUT 15
#define DEFAULT_WINDOW_WIDTH       600
#define DEFAULT_WINDOW_HEIGHT      450

enum
{
  PROP_0,
  PROP_STARTUP_WIDE
};

typedef enum
{
  CAMERA_MODE_PHOTO,
  CAMERA_MODE_VIDEO,
  CAMERA_MODE_BURST
} CameraMode;

typedef enum
{
  PAGE_WEBCAM  = 0,
  PAGE_EFFECTS = 1,
  PAGE_SPINNER = 2,
  PAGE_PROBLEM = 3,
} CheeseNotebookPage;

typedef struct
{
  gboolean recording;

  gboolean isFullscreen;
  gboolean startup_wide;

  char *video_filename;

  CheeseCamera *camera;
  CameraMode camera_mode;
  CheeseGConf *gconf;
  CheeseFileUtil *fileutil;

//  CheeseDbus *server;

  GtkWidget *window;
  GtkWidget *fullscreen_popup;

  GtkWidget *notebook;
  GtkWidget *notebook_bar;
  GtkWidget *fullscreen_bar;

  GtkWidget *main_vbox;
  GtkWidget *video_vbox;

  GtkWidget *netbook_alignment;
  GtkWidget *toolbar_alignment;
  GtkWidget *effect_button_alignment;
  GtkWidget *togglegroup_alignment;

  GtkWidget *effect_frame;
  GtkWidget *effect_alignment;
  GtkWidget *effect_chooser;
  GtkWidget *throbber_align;
  GtkWidget *throbber_box;
  GtkWidget *throbber;
  GtkWidget *countdown_frame;
  GtkWidget *countdown_frame_fullscreen;
  GtkWidget *countdown;
  GtkWidget *countdown_fullscreen;
  GtkWidget *problem_page;
  GtkWidget *problem_area;
  GtkWidget *problem_bar;

  GtkWidget *button_effects;
  GtkWidget *button_photo;
  GtkWidget *button_video;
  GtkWidget *button_burst;
  GtkWidget *button_effects_fullscreen;
  GtkWidget *button_photo_fullscreen;
  GtkWidget *button_video_fullscreen;
  GtkWidget *button_burst_fullscreen;
  GtkWidget *button_exit_fullscreen;

  GtkWidget *image_take_photo;
  GtkWidget *image_take_photo_fullscreen;
  GtkWidget *label_effects;
  GtkWidget *label_photo;
  GtkWidget *label_photo_fullscreen;
  GtkWidget *label_take_photo;
  GtkWidget *label_take_photo_fullscreen;
  GtkWidget *label_video;
  GtkWidget *label_video_fullscreen;

  GtkWidget *thumb_scrollwindow;
  GtkWidget *thumb_nav;
  GtkWidget *thumb_view;
  GtkWidget *thumb_view_popup_menu;

  GtkWidget *screen;
  GtkWidget *take_picture;
  GtkWidget *take_picture_fullscreen;

  GtkActionGroup *actions_countdown;
  GtkActionGroup *actions_effects;
  GtkActionGroup *actions_file;
  GtkActionGroup *actions_main;
  GtkActionGroup *actions_photo;
  GtkActionGroup *actions_toggle;
  GtkActionGroup *actions_video;
  GtkActionGroup *actions_burst;
  GtkActionGroup *actions_fullscreen;
  GtkActionGroup *actions_wide_mode;

  GtkUIManager *ui_manager;

  GSource *fullscreen_timeout_source;

  gint repeat_count;
  gboolean is_bursting;

  CheeseFlash *flash;
} CheeseWindowPrivate;

#define CHEESE_WINDOW_GET_PRIVATE(o)                     \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHEESE_TYPE_WINDOW, \
                                CheeseWindowPrivate))

G_DEFINE_TYPE (CheeseWindow, cheese_window, GTK_TYPE_WINDOW);

/* FIXME: some code borrowed from cheese-widget
 * We should really use it directly instead of duplicating stuff here */
static GdkPixbuf *
cheese_window_load_pixbuf (GtkWidget  *widget,
                           const char *icon_name,
                           guint       size,
                           GError    **error)
{
  GtkIconTheme *theme;
  GdkPixbuf    *pixbuf;

  theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (widget));
  if (!gtk_icon_theme_has_icon (theme, icon_name))
  {
    pixbuf = gtk_icon_theme_load_icon (theme, "error",
                                       size, 0, error);
  }
  else
  {
    pixbuf = gtk_icon_theme_load_icon (theme, icon_name,
                                       size, 0, error);
  }
  return pixbuf;
}

static gboolean
cheese_window_logo_expose (GtkWidget      *w,
                           GdkEventExpose *event,
                           gpointer        user_data)
{
  const char   *icon_name;
  GdkPixbuf    *pixbuf, *logo;
  GError       *error = NULL;
  cairo_t      *cr;
  GtkAllocation allocation;
  guint         s_width, s_height, d_width, d_height;
  float         ratio;

  gdk_draw_rectangle (w->window, w->style->black_gc, TRUE,
                      0, 0, w->allocation.width, w->allocation.height);
  icon_name = g_object_get_data (G_OBJECT (w), "icon-name");
  if (icon_name == NULL)
    return FALSE;

  cr = gdk_cairo_create (gtk_widget_get_window (w));
  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
  gtk_widget_get_allocation (w, &allocation);
  cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);

  d_width  = allocation.width;
  d_height = allocation.height - (allocation.height / 3);

  pixbuf = cheese_window_load_pixbuf (w, icon_name, d_height, &error);
  if (pixbuf == NULL)
  {
    g_warning ("Could not load icon '%s': %s", icon_name, error->message);
    g_error_free (error);
    return FALSE;
  }

  s_width  = gdk_pixbuf_get_width (pixbuf);
  s_height = gdk_pixbuf_get_height (pixbuf);

  if ((gfloat) d_width / s_width > (gfloat) d_height / s_height)
  {
    ratio = (gfloat) d_height / s_height;
  }
  else
  {
    ratio = (gfloat) d_width / s_width;
  }

  s_width  *= ratio;
  s_height *= ratio;

  logo = gdk_pixbuf_scale_simple (pixbuf, s_width, s_height, GDK_INTERP_BILINEAR);

  gdk_cairo_set_source_pixbuf (cr, logo, (allocation.width - s_width) / 2, (allocation.height - s_height) / 2);
  cairo_paint (cr);
  cairo_destroy (cr);

  g_object_unref (logo);
  g_object_unref (pixbuf);

  return FALSE;
}

static void
cheese_window_set_problem_page (CheeseWindow *window,
                                const char   *icon_name)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (window);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), PAGE_PROBLEM);
  g_object_set_data_full (G_OBJECT (priv->problem_area),
                          "icon-name", g_strdup (icon_name), g_free);
  g_signal_connect (priv->problem_area, "expose-event",
                    G_CALLBACK (cheese_window_logo_expose), window);
}

static void
cheese_window_spinner_invert (GtkWidget *spinner, GtkWidget *parent)
{
  GtkStyle *style;
  guint     i;

  for (i = GTK_STATE_NORMAL; i <= GTK_STATE_INSENSITIVE; i++)
  {
    GdkColor *fg, *bg;

    style = gtk_widget_get_style (spinner);
    fg    = gdk_color_copy (&style->fg[i]);
    bg    = gdk_color_copy (&style->bg[i]);

    gtk_widget_modify_fg (spinner, i, bg);
    gtk_widget_modify_bg (spinner, i, fg);

    gtk_widget_modify_fg (parent, i, bg);
    gtk_widget_modify_bg (parent, i, fg);

    gdk_color_free (fg);
    gdk_color_free (bg);
  }
}

void
cheese_window_bring_to_front (CheeseWindow *window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (window);
  guint32       startup_timestamp = gdk_x11_get_server_time (gtk_widget_get_window (GTK_WIDGET (window)));

  gdk_x11_window_set_user_time (gtk_widget_get_window (GTK_WIDGET (window)), startup_timestamp);

  gtk_window_present (GTK_WINDOW (window));
}

static gboolean
cheese_window_key_press_event_cb (GtkWidget *win, GdkEventKey *event, CheeseWindow *cheese_window)
{
  /* If we have modifiers, and either Ctrl, Mod1 (Alt), or any
   * of Mod3 to Mod5 (Mod2 is num-lock...) are pressed, we
   * let Gtk+ handle the key */
  if (event->state != 0
      && ((event->state & GDK_CONTROL_MASK)
          || (event->state & GDK_MOD1_MASK)
          || (event->state & GDK_MOD3_MASK)
          || (event->state & GDK_MOD4_MASK)
          || (event->state & GDK_MOD5_MASK)))
    return FALSE;

  switch (event->keyval)
  {
    case GDK_WebCam:

      /* do stuff */
      cheese_window_action_button_clicked_cb (NULL, cheese_window);
      return TRUE;
  }

  return FALSE;
}

static void
cheese_window_fullscreen_clear_timeout (CheeseWindow *cheese_window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);

  if (priv->fullscreen_timeout_source != NULL)
  {
    g_source_unref (priv->fullscreen_timeout_source);
    g_source_destroy (priv->fullscreen_timeout_source);
  }

  priv->fullscreen_timeout_source = NULL;
}

static gboolean
cheese_window_fullscreen_timeout_cb (gpointer data)
{
  CheeseWindow *cheese_window = data;
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);

  gtk_widget_hide_all (priv->fullscreen_popup);

  cheese_window_fullscreen_clear_timeout (cheese_window);

  return FALSE;
}

static void
cheese_window_fullscreen_set_timeout (CheeseWindow *cheese_window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);
  GSource *source;

  cheese_window_fullscreen_clear_timeout (cheese_window);

  source = g_timeout_source_new (FULLSCREEN_TIMEOUT);

  g_source_set_callback (source, cheese_window_fullscreen_timeout_cb, cheese_window, NULL);
  g_source_attach (source, NULL);

  priv->fullscreen_timeout_source = source;
}

static void
cheese_window_fullscreen_show_bar (CheeseWindow *cheese_window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);
  gtk_widget_show_all (priv->fullscreen_popup);

  /* show me the notebook with the buttons if the countdown was not triggered */
  if (cheese_countdown_get_state (CHEESE_COUNTDOWN (priv->countdown_fullscreen)) == 0)
    gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->fullscreen_bar), 0);
}

static gboolean
cheese_window_fullscreen_motion_notify_cb (GtkWidget      *widget,
                                           GdkEventMotion *event,
                                           CheeseWindow   *cheese_window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);

  if (priv->isFullscreen)
  {
    int height;
    int width;

    gtk_window_get_size (GTK_WINDOW (cheese_window), &width, &height);
    if (event->y > height - 5)
    {
      cheese_window_fullscreen_show_bar (cheese_window);
    }

    /* don't set the timeout in effect-chooser mode */
    if (gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->notebook)) != 1)
      cheese_window_fullscreen_set_timeout (cheese_window);
  }
  return FALSE;
}

void
cheese_window_toggle_wide_mode (GtkWidget *widget, CheeseWindow *cheese_window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);
  gboolean toggled = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (widget));

  gtk_widget_set_size_request (priv->notebook,
                               GTK_WIDGET (priv->notebook)->allocation.width,
                               GTK_WIDGET (priv->notebook)->allocation.height);

  /* set a single column in wide mode */
  gtk_icon_view_set_columns (GTK_ICON_VIEW (priv->thumb_view), toggled ? 1 : G_MAXINT);

  /* switch thumb_nav mode */
  eog_thumb_nav_set_vertical (EOG_THUMB_NAV (priv->thumb_nav), toggled);

  /* reparent thumb_view */
  g_object_ref (priv->thumb_scrollwindow);
  if (toggled)
  {
    gtk_container_remove (GTK_CONTAINER (priv->video_vbox), priv->thumb_scrollwindow);
    gtk_container_add (GTK_CONTAINER (priv->netbook_alignment), priv->thumb_scrollwindow);
    g_object_unref (priv->thumb_scrollwindow);
  }
  else
  {
    gtk_container_remove (GTK_CONTAINER (priv->netbook_alignment), priv->thumb_scrollwindow);
    gtk_box_pack_end (GTK_BOX (priv->video_vbox), priv->thumb_scrollwindow, FALSE, FALSE, 0);
    g_object_unref (priv->thumb_scrollwindow);
    eog_thumb_nav_set_policy (EOG_THUMB_NAV (priv->thumb_nav),
                              GTK_POLICY_AUTOMATIC,
                              GTK_POLICY_NEVER);
  }

  /* update spacing */

  /* NOTE: be really carefull when changing the ui file to update spacing
   * values here too! */
  if (toggled)
  {
    g_object_set (G_OBJECT (priv->toolbar_alignment),
                  "bottom-padding", 10, NULL);
    g_object_set (G_OBJECT (priv->togglegroup_alignment),
                  "left-padding", 6, NULL);
    g_object_set (G_OBJECT (priv->effect_button_alignment),
                  "right-padding", 0, NULL);
    g_object_set (G_OBJECT (priv->netbook_alignment),
                  "left-padding", 6, NULL);
  }
  else
  {
    g_object_set (G_OBJECT (priv->toolbar_alignment),
                  "bottom-padding", 6, NULL);
    g_object_set (G_OBJECT (priv->togglegroup_alignment),
                  "left-padding", 24, NULL);
    g_object_set (G_OBJECT (priv->effect_button_alignment),
                  "right-padding", 24, NULL);
    g_object_set (G_OBJECT (priv->netbook_alignment),
                  "left-padding", 0, NULL);
  }

  gtk_container_resize_children (GTK_CONTAINER (priv->thumb_scrollwindow));

  GtkRequisition req;
  gtk_widget_size_request (GTK_WIDGET (cheese_window), &req);
  gtk_window_resize (GTK_WINDOW (cheese_window), req.width, req.height);
  gtk_widget_set_size_request (priv->notebook, -1, -1);

  g_object_set (priv->gconf, "gconf_prop_wide_mode", toggled, NULL);
}

void
cheese_window_toggle_fullscreen (GtkWidget *widget, CheeseWindow *cheese_window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);
  GdkColor   bg_color = {0, 0, 0, 0};
  GtkWidget *menubar;

  menubar = gtk_ui_manager_get_widget (priv->ui_manager, "/MainMenu");

  if (!priv->isFullscreen)
  {
    gtk_widget_hide (priv->thumb_view);
    gtk_widget_hide (priv->thumb_scrollwindow);
    gtk_widget_hide (menubar);
    gtk_widget_hide (priv->notebook_bar);
    gtk_widget_modify_bg (GTK_WIDGET (cheese_window), GTK_STATE_NORMAL, &bg_color);

    gtk_widget_add_events (GTK_WIDGET (cheese_window), GDK_POINTER_MOTION_MASK);
    gtk_widget_add_events (priv->screen, GDK_POINTER_MOTION_MASK);

    g_signal_connect (cheese_window, "motion-notify-event",
                      G_CALLBACK (cheese_window_fullscreen_motion_notify_cb),
                      cheese_window);
    g_signal_connect (priv->screen, "motion-notify-event",
                      G_CALLBACK (cheese_window_fullscreen_motion_notify_cb),
                      cheese_window);

    gtk_window_fullscreen (GTK_WINDOW (cheese_window));

    gtk_widget_set_size_request (priv->effect_alignment, -1, FULLSCREEN_POPUP_HEIGHT);
    cheese_window_fullscreen_show_bar (cheese_window);

    /* don't set the timeout in effect-chooser mode */
    if (gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->notebook)) != 1)
      cheese_window_fullscreen_set_timeout (cheese_window);

    priv->isFullscreen = TRUE;
  }
  else
  {
    gtk_widget_show_all (GTK_WIDGET (cheese_window));
    gtk_widget_hide_all (priv->fullscreen_popup);
    gtk_widget_modify_bg (GTK_WIDGET (cheese_window), GTK_STATE_NORMAL, NULL);

    g_signal_handlers_disconnect_by_func (cheese_window,
                                          (gpointer) cheese_window_fullscreen_motion_notify_cb, cheese_window);
    g_signal_handlers_disconnect_by_func (priv->screen,
                                          (gpointer) cheese_window_fullscreen_motion_notify_cb, cheese_window);

    gtk_window_unfullscreen (GTK_WINDOW (cheese_window));
    gtk_widget_set_size_request (priv->effect_alignment, -1, -1);
    priv->isFullscreen = FALSE;

    cheese_window_fullscreen_clear_timeout (cheese_window);
  }
}

static void
cheese_window_exit_fullscreen_button_clicked_cb (GtkWidget *button, CheeseWindow *cheese_window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);
  GtkAction *action = gtk_ui_manager_get_action (priv->ui_manager, "/MainMenu/Cheese/Fullscreen");

  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), FALSE);
}

static gboolean
cheese_window_fullscreen_leave_notify_cb (GtkWidget        *widget,
                                          GdkEventCrossing *event,
                                          CheeseWindow     *cheese_window)
{
  cheese_window_fullscreen_clear_timeout (cheese_window);
  return FALSE;
}

static void
cheese_window_photo_saved_cb (CheeseCamera *camera, CheeseWindow *cheese_window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);
  gdk_threads_enter ();
  if (!priv->is_bursting)
  {
    gtk_action_group_set_sensitive (priv->actions_effects, TRUE);
    gtk_action_group_set_sensitive (priv->actions_toggle, TRUE);
    gtk_widget_set_sensitive (priv->take_picture, TRUE);
    gtk_widget_set_sensitive (priv->take_picture_fullscreen, TRUE);
  }
  gdk_flush ();
  gdk_threads_leave ();
}

static void
cheese_window_video_saved_cb (CheeseCamera *camera, CheeseWindow *cheese_window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);
  gdk_threads_enter ();

  /* TODO look at this g_free */
  g_free (priv->video_filename);
  priv->video_filename = NULL;
  gtk_action_group_set_sensitive (priv->actions_effects, TRUE);
  gtk_widget_set_sensitive (priv->take_picture, TRUE);
  gtk_widget_set_sensitive (priv->take_picture_fullscreen, TRUE);
  gdk_flush ();
  gdk_threads_leave ();
}

static void
cheese_window_selection_changed_cb (GtkIconView  *iconview,
                                    CheeseWindow *window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (window);
  if (cheese_thumb_view_get_n_selected (CHEESE_THUMB_VIEW (priv->thumb_view)) > 0)
  {
    gtk_action_group_set_sensitive (priv->actions_file, TRUE);
  }
  else
  {
    gtk_action_group_set_sensitive (priv->actions_file, FALSE);
  }
}

static gboolean
cheese_window_button_press_event_cb (GtkWidget *iconview, GdkEventButton *event,
                                     CheeseWindow *cheese_window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);
  GtkTreePath *path;

  if (event->type == GDK_BUTTON_PRESS || event->type == GDK_2BUTTON_PRESS)
  {
    path = gtk_icon_view_get_path_at_pos (GTK_ICON_VIEW (iconview),
                                          (int) event->x, (int) event->y);
    if (path == NULL) return FALSE;

    if (event->type == GDK_BUTTON_PRESS && event->button == 1)
    {
      if (cheese_thumb_view_get_n_selected (CHEESE_THUMB_VIEW (priv->thumb_view)) > 1)
      {
        gtk_icon_view_unselect_all (GTK_ICON_VIEW (priv->thumb_view));
        gtk_icon_view_select_path (GTK_ICON_VIEW (priv->thumb_view), path);
        gtk_icon_view_set_cursor (GTK_ICON_VIEW (priv->thumb_view), path, NULL, FALSE);
      }
    }
    else if (event->type == GDK_BUTTON_PRESS && event->button == 3)
    {
      int button, event_time;

      if (event)
      {
        button     = event->button;
        event_time = event->time;
      }
      else
      {
        button     = 0;
        event_time = gtk_get_current_event_time ();
      }

      if (!gtk_icon_view_path_is_selected (GTK_ICON_VIEW (priv->thumb_view), path) ||
          cheese_thumb_view_get_n_selected (CHEESE_THUMB_VIEW (priv->thumb_view)) <= 1)
      {
        gtk_icon_view_unselect_all (GTK_ICON_VIEW (priv->thumb_view));
        gtk_icon_view_select_path (GTK_ICON_VIEW (priv->thumb_view), path);
        gtk_icon_view_set_cursor (GTK_ICON_VIEW (priv->thumb_view), path, NULL, FALSE);
      }

      GList   *l, *files;
      gchar   *file;
      gboolean list_has_videos = FALSE;
      files = cheese_thumb_view_get_selected_images_list (CHEESE_THUMB_VIEW (priv->thumb_view));

      for (l = files; l != NULL; l = l->next)
      {
        file = g_file_get_path (l->data);
        if (g_str_has_suffix (file, VIDEO_NAME_SUFFIX))
        {
          list_has_videos = TRUE;
        }
        g_free (file);
        g_object_unref (l->data);
      }
      g_list_free (l);
      g_list_free (files);

      gtk_menu_popup (GTK_MENU (priv->thumb_view_popup_menu),
                      NULL, iconview, NULL, NULL, button, event_time);

      return TRUE;
    }
    else if (event->type == GDK_2BUTTON_PRESS && event->button == 1)
    {
      cheese_cmd_file_open (NULL, cheese_window);
      return TRUE;
    }
  }
  return FALSE;
}

void
cheese_window_countdown_hide_cb (gpointer data)
{
  CheeseWindow *cheese_window = (CheeseWindow *) data;
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook_bar), 0);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->fullscreen_bar), 0);
}

void
cheese_window_countdown_picture_cb (gpointer data)
{
  CheeseWindow *cheese_window = (CheeseWindow *) data;
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);
  char         *photo_filename;

  if (priv->camera_mode == CAMERA_MODE_BURST)
  {
    photo_filename = cheese_fileutil_get_new_media_filename (priv->fileutil, CHEESE_MEDIA_MODE_BURST);
  }
  else
  {
    photo_filename = cheese_fileutil_get_new_media_filename (priv->fileutil, CHEESE_MEDIA_MODE_PHOTO);
  }

  if (cheese_camera_take_photo (priv->camera, photo_filename))
  {
    cheese_flash_fire (priv->flash);
    ca_gtk_play_for_widget (priv->screen, 0,
                            CA_PROP_EVENT_ID, "camera-shutter",
                            CA_PROP_MEDIA_ROLE, "event",
                            CA_PROP_EVENT_DESCRIPTION, _("Shutter sound"),
                            NULL);
  }
  g_free (photo_filename);
}

static void
cheese_window_no_camera_info_bar_response (GtkWidget *widget, gint response_id, CheeseWindow *cheese_window)
{
  GError  *error = NULL;
  gboolean ret;

  if (response_id == GTK_RESPONSE_HELP)
  {
    ret = g_app_info_launch_default_for_uri ("ghelp:cheese?faq", NULL, &error);

    if (ret == FALSE)
    {
      GtkWidget *d;
      d = gtk_message_dialog_new (GTK_WINDOW (cheese_window),
                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                  _("Unable to open help file for Cheese"));
      gtk_dialog_run (GTK_DIALOG (d));
      gtk_widget_destroy (d);
      g_error_free (error);
    }
  }
}

static void
cheese_window_stop_recording (CheeseWindow *cheese_window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);
  if (priv->recording)
  {
    gtk_action_group_set_sensitive (priv->actions_effects, TRUE);
    gtk_action_group_set_sensitive (priv->actions_toggle, TRUE);
    gtk_widget_set_sensitive (priv->take_picture, FALSE);
    gchar *str = g_strconcat ("<b>", _("_Start Recording"), "</b>", NULL);
    gtk_label_set_text_with_mnemonic (GTK_LABEL (priv->label_take_photo), str);
    gtk_label_set_text_with_mnemonic (GTK_LABEL (priv->label_take_photo_fullscreen), str);
    g_free (str);
    gtk_label_set_use_markup (GTK_LABEL (priv->label_take_photo), TRUE);
    gtk_image_set_from_stock (GTK_IMAGE (priv->image_take_photo), GTK_STOCK_MEDIA_RECORD, GTK_ICON_SIZE_BUTTON);
    gtk_label_set_use_markup (GTK_LABEL (priv->label_take_photo_fullscreen), TRUE);
    gtk_image_set_from_stock (GTK_IMAGE (priv->image_take_photo_fullscreen),
                              GTK_STOCK_MEDIA_RECORD, GTK_ICON_SIZE_BUTTON);

    cheese_camera_stop_video_recording (priv->camera);
    priv->recording = FALSE;
  }
}

static gboolean
cheese_window_escape_key_cb (CheeseWindow *cheese_window,
                             GtkAccelGroup *accel_group,
                             guint keyval, GdkModifierType modifier)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);
  if (priv->isFullscreen)
  {
    if (cheese_countdown_get_state (CHEESE_COUNTDOWN (priv->countdown_fullscreen)) == 0)
    {
      GtkAction *action = gtk_ui_manager_get_action (priv->ui_manager, "/MainMenu/Cheese/Fullscreen");
      gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), FALSE);
      return TRUE;
    }
  }

  cheese_countdown_cancel ((CheeseCountdown *) priv->countdown);
  cheese_countdown_cancel ((CheeseCountdown *) priv->countdown_fullscreen);

  if (priv->camera_mode == CAMERA_MODE_PHOTO)
  {
    gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook_bar), 0);
    gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->fullscreen_bar), 0);

    gtk_widget_set_sensitive (priv->take_picture, TRUE);
    gtk_widget_set_sensitive (priv->take_picture_fullscreen, TRUE);
  }
  else if (priv->camera_mode == CAMERA_MODE_BURST)
  {
    priv->repeat_count = 0;
    priv->is_bursting  = FALSE;

    gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook_bar), 0);
    gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->fullscreen_bar), 0);

    gtk_action_group_set_sensitive (priv->actions_effects, TRUE);
    gtk_action_group_set_sensitive (priv->actions_toggle, TRUE);

    gtk_widget_set_sensitive (priv->take_picture, TRUE);
    gtk_widget_set_sensitive (priv->take_picture_fullscreen, TRUE);
  }
  else
  {
    cheese_window_stop_recording (cheese_window);
  }
  return TRUE;
}

static gboolean
cheese_window_take_photo (gpointer data)
{
  gboolean      countdown;
  CheeseWindow *cheese_window = (CheeseWindow *) data;
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);

  /* return if burst mode was cancelled */
  if (priv->camera_mode == CAMERA_MODE_BURST &&
      !priv->is_bursting && priv->repeat_count <= 0)
  {
    return FALSE;
  }

  g_object_get (priv->gconf, "gconf_prop_countdown", &countdown, NULL);
  if (countdown)
  {
    if (priv->isFullscreen)
    {
      cheese_countdown_start ((CheeseCountdown *) priv->countdown_fullscreen,
                              cheese_window_countdown_picture_cb,
                              cheese_window_countdown_hide_cb,
                              (gpointer) cheese_window);
      gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->fullscreen_bar), 1);

      /* show bar, start timeout
       * ATTENTION: if the countdown is longer than FULLSCREEN_TIMEOUT,
       * the bar will disapear before the countdown ends
       */
      cheese_window_fullscreen_show_bar (cheese_window);
      cheese_window_fullscreen_set_timeout (cheese_window);
    }
    else
    {
      cheese_countdown_start ((CheeseCountdown *) priv->countdown,
                              cheese_window_countdown_picture_cb,
                              cheese_window_countdown_hide_cb,
                              (gpointer) cheese_window);
      gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook_bar), 1);
    }
  }
  else
  {
    cheese_window_countdown_picture_cb (cheese_window);
  }

  gtk_widget_set_sensitive (priv->take_picture, FALSE);
  gtk_widget_set_sensitive (priv->take_picture_fullscreen, FALSE);

  if (priv->camera_mode == CAMERA_MODE_BURST)
  {
    guint    repeat_delay = 1000;
    gboolean countdown    = FALSE;

    g_object_get (priv->gconf, "gconf_prop_burst_delay", &repeat_delay, NULL);
    g_object_get (priv->gconf, "gconf_prop_countdown", &countdown, NULL);

    if (countdown && repeat_delay < 5000)
    {
      /* A countdown takes 4 seconds, leave some headroom before repeating it.
       * Magic number chosen via expiriment on Netbook */
      repeat_delay = 5000;
    }

    /* start burst mode photo series */
    if (!priv->is_bursting)
    {
      g_timeout_add (repeat_delay, cheese_window_take_photo, cheese_window);
      priv->is_bursting = TRUE;
    }
    priv->repeat_count--;
    if (priv->repeat_count > 0)
    {
      return TRUE;
    }
  }
  priv->is_bursting = FALSE;

  return FALSE;
}

void
cheese_window_action_button_clicked_cb (GtkWidget *widget, CheeseWindow *cheese_window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);
  char *str;

  switch (priv->camera_mode)
  {
    case CAMERA_MODE_BURST:

      /* ignore keybindings and other while bursting */
      if (priv->is_bursting)
      {
        break;
      }
      gtk_action_group_set_sensitive (priv->actions_effects, FALSE);
      gtk_action_group_set_sensitive (priv->actions_toggle, FALSE);
      g_object_get (priv->gconf, "gconf_prop_burst_repeat", &priv->repeat_count, NULL); /* reset burst counter */
      cheese_fileutil_reset_burst (priv->fileutil); /* reset filename counter */
    case CAMERA_MODE_PHOTO:
      cheese_window_take_photo (cheese_window);
      break;
    case CAMERA_MODE_VIDEO:
      if (!priv->recording)
      {
        gtk_action_group_set_sensitive (priv->actions_effects, FALSE);
        gtk_action_group_set_sensitive (priv->actions_toggle, FALSE);
        str = g_strconcat ("<b>", _("_Stop Recording"), "</b>", NULL);
        gtk_label_set_text_with_mnemonic (GTK_LABEL (priv->label_take_photo), str);
        gtk_label_set_text_with_mnemonic (GTK_LABEL (priv->label_take_photo_fullscreen), str);
        g_free (str);
        gtk_label_set_use_markup (GTK_LABEL (priv->label_take_photo), TRUE);
        gtk_image_set_from_stock (GTK_IMAGE (
                                    priv->image_take_photo), GTK_STOCK_MEDIA_STOP, GTK_ICON_SIZE_BUTTON);
        gtk_label_set_use_markup (GTK_LABEL (priv->label_take_photo_fullscreen), TRUE);
        gtk_image_set_from_stock (GTK_IMAGE (priv->image_take_photo_fullscreen),
                                  GTK_STOCK_MEDIA_STOP, GTK_ICON_SIZE_BUTTON);

        priv->video_filename = cheese_fileutil_get_new_media_filename (priv->fileutil,
                                                                                CAMERA_MODE_VIDEO);
        cheese_camera_start_video_recording (priv->camera, priv->video_filename);

        priv->recording = TRUE;
      }
      else
      {
        cheese_window_stop_recording (cheese_window);
      }
      break;
    default:
      g_assert_not_reached ();
      break;
  }
}

void
cheese_window_effect_button_pressed_cb (GtkWidget *widget, CheeseWindow *cheese_window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);
  if (gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->notebook)) == 1)
  {
    gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), PAGE_WEBCAM);
    gtk_label_set_text_with_mnemonic (GTK_LABEL (priv->label_effects), _("_Effects"));
    gtk_widget_set_sensitive (priv->take_picture, TRUE);
    gtk_widget_set_sensitive (priv->take_picture_fullscreen, TRUE);
    if (priv->camera_mode == CAMERA_MODE_PHOTO)
    {
      gtk_action_group_set_sensitive (priv->actions_photo, TRUE);
    }
    else if (priv->camera_mode == CAMERA_MODE_BURST)
    {
      gtk_action_group_set_sensitive (priv->actions_burst, TRUE);
    }
    else
    {
      gtk_action_group_set_sensitive (priv->actions_video, TRUE);
    }
    cheese_camera_set_effect (priv->camera,
                              cheese_effect_chooser_get_selection (CHEESE_EFFECT_CHOOSER (priv->effect_chooser)));
    g_object_set (priv->gconf, "gconf_prop_selected_effects",
                  cheese_effect_chooser_get_selection_string (CHEESE_EFFECT_CHOOSER (priv->effect_chooser)),
                  NULL);
  }
  else
  {
    gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), PAGE_EFFECTS);
    gtk_widget_set_sensitive (GTK_WIDGET (priv->take_picture), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (priv->take_picture_fullscreen), FALSE);
    gtk_action_group_set_sensitive (priv->actions_photo, FALSE);
    gtk_action_group_set_sensitive (priv->actions_video, FALSE);
    gtk_action_group_set_sensitive (priv->actions_burst, FALSE);
  }
}

void
cheese_window_preferences_cb (GtkAction *action, CheeseWindow *cheese_window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);
  cheese_prefs_dialog_run (GTK_WIDGET (cheese_window), priv->gconf,
                           priv->camera);
}

void
cheese_window_toggle_countdown (GtkWidget *widget, CheeseWindow *window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (window);
  gboolean countdown = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (widget));

  g_object_set (priv->gconf, "gconf_prop_countdown", countdown, NULL);
}

static void
cheese_window_activate_radio_action (GtkAction *action, GtkRadioAction *current, CheeseWindow *cheese_window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);
  gchar *str;

  if (strcmp (gtk_action_get_name (GTK_ACTION (current)), "Photo") == 0)
  {
    priv->camera_mode = CAMERA_MODE_PHOTO;

    str = g_strconcat ("<b>", _("_Take a Photo"), "</b>", NULL);
    gtk_label_set_text_with_mnemonic (GTK_LABEL (priv->label_take_photo), g_strdup (str));
    gtk_label_set_use_markup (GTK_LABEL (priv->label_take_photo), TRUE);
    gtk_label_set_text_with_mnemonic (GTK_LABEL (priv->label_take_photo_fullscreen), g_strdup (str));
    gtk_label_set_use_markup (GTK_LABEL (priv->label_take_photo_fullscreen), TRUE);
    gtk_action_group_set_sensitive (priv->actions_photo, TRUE);
    gtk_action_group_set_sensitive (priv->actions_video, FALSE);
    gtk_action_group_set_sensitive (priv->actions_burst, FALSE);
  }
  else if (strcmp (gtk_action_get_name (GTK_ACTION (current)), "Burst") == 0)
  {
    priv->camera_mode = CAMERA_MODE_BURST;

    str = g_strconcat ("<b>", _("_Take multiple Photos"), "</b>", NULL);
    gtk_label_set_text_with_mnemonic (GTK_LABEL (priv->label_take_photo), g_strdup (str));
    gtk_label_set_use_markup (GTK_LABEL (priv->label_take_photo), TRUE);
    gtk_label_set_text_with_mnemonic (GTK_LABEL (priv->label_take_photo_fullscreen), g_strdup (str));
    gtk_label_set_use_markup (GTK_LABEL (priv->label_take_photo_fullscreen), TRUE);
    gtk_action_group_set_sensitive (priv->actions_photo, FALSE);
    gtk_action_group_set_sensitive (priv->actions_video, FALSE);
    gtk_action_group_set_sensitive (priv->actions_burst, TRUE);
  }
  else
  {
    priv->camera_mode = CAMERA_MODE_VIDEO;

    str = g_strconcat ("<b>", _("_Start recording"), "</b>", NULL);
    gtk_label_set_text_with_mnemonic (GTK_LABEL (priv->label_take_photo), g_strdup (str));
    gtk_label_set_use_markup (GTK_LABEL (priv->label_take_photo), TRUE);
    gtk_label_set_text_with_mnemonic (GTK_LABEL (priv->label_take_photo_fullscreen), g_strdup (str));
    gtk_label_set_use_markup (GTK_LABEL (priv->label_take_photo_fullscreen), TRUE);
    gtk_action_group_set_sensitive (priv->actions_photo, FALSE);
    gtk_action_group_set_sensitive (priv->actions_video, TRUE);
    gtk_action_group_set_sensitive (priv->actions_burst, FALSE);
  }
  g_free (str);
}

GtkActionGroup *
cheese_window_action_group_new (CheeseWindow *cheese_window, char *name,
                                const GtkActionEntry *action_entries, int num_action_entries)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);
  GtkActionGroup *action_group;

  action_group = gtk_action_group_new (name);
  gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions (action_group, action_entries,
                                num_action_entries, cheese_window);
  gtk_ui_manager_insert_action_group (priv->ui_manager, action_group, 0);

  return action_group;
}

GtkActionGroup *
cheese_window_toggle_action_group_new (CheeseWindow *cheese_window, char *name,
                                       const GtkToggleActionEntry *action_entries, int num_action_entries)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);
  GtkActionGroup *action_group;

  action_group = gtk_action_group_new (name);
  gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_toggle_actions (action_group, action_entries,
                                       num_action_entries, cheese_window);
  gtk_ui_manager_insert_action_group (priv->ui_manager, action_group, 0);

  return action_group;
}

GtkActionGroup *
cheese_window_radio_action_group_new (CheeseWindow *cheese_window, char *name,
                                      const GtkRadioActionEntry *action_entries, int num_action_entries)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);
  GtkActionGroup *action_group;

  action_group = gtk_action_group_new (name);
  gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_radio_actions (action_group, action_entries,
                                      num_action_entries, 0,
                                      G_CALLBACK (cheese_window_activate_radio_action),
                                      cheese_window);
  gtk_ui_manager_insert_action_group (priv->ui_manager, action_group, 0);

  return action_group;
}


static void
setup_widgets_from_builder (CheeseWindow *cheese_window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);
  GError     *error = NULL;
  GtkBuilder *builder;

  builder = gtk_builder_new ();
  gtk_builder_add_from_file (builder, PACKAGE_DATADIR "/cheese.ui", &error);

  if (error)
  {
    g_error ("building ui from %s failed: %s", PACKAGE_DATADIR "/cheese.ui", error->message);
    g_clear_error (&error); /* bah... */
  }

  priv->button_effects              = GTK_WIDGET (gtk_builder_get_object (builder, "button_effects"));
  priv->button_photo                = GTK_WIDGET (gtk_builder_get_object (builder, "button_photo"));
  priv->button_video                = GTK_WIDGET (gtk_builder_get_object (builder, "button_video"));
  priv->button_burst                = GTK_WIDGET (gtk_builder_get_object (builder, "button_burst"));
  priv->image_take_photo            = GTK_WIDGET (gtk_builder_get_object (builder, "image_take_photo"));
  priv->label_effects               = GTK_WIDGET (gtk_builder_get_object (builder, "label_effects"));
  priv->label_photo                 = GTK_WIDGET (gtk_builder_get_object (builder, "label_photo"));
  priv->label_take_photo            = GTK_WIDGET (gtk_builder_get_object (builder, "label_take_photo"));
  priv->label_video                 = GTK_WIDGET (gtk_builder_get_object (builder, "label_video"));
  priv->main_vbox                   = GTK_WIDGET (gtk_builder_get_object (builder, "main_vbox"));
  priv->netbook_alignment           = GTK_WIDGET (gtk_builder_get_object (builder, "netbook_alignment"));
  priv->togglegroup_alignment       = GTK_WIDGET (gtk_builder_get_object (builder, "togglegroup_alignment"));
  priv->effect_button_alignment     = GTK_WIDGET (gtk_builder_get_object (builder, "effect_button_alignment"));
  priv->toolbar_alignment           = GTK_WIDGET (gtk_builder_get_object (builder, "toolbar_alignment"));
  priv->video_vbox                  = GTK_WIDGET (gtk_builder_get_object (builder, "video_vbox"));
  priv->notebook                    = GTK_WIDGET (gtk_builder_get_object (builder, "notebook"));
  priv->notebook_bar                = GTK_WIDGET (gtk_builder_get_object (builder, "notebook_bar"));
  priv->screen                      = GTK_WIDGET (gtk_builder_get_object (builder, "video_screen"));
  priv->take_picture                = GTK_WIDGET (gtk_builder_get_object (builder, "take_picture"));
  priv->thumb_scrollwindow          = GTK_WIDGET (gtk_builder_get_object (builder, "thumb_scrollwindow"));
  priv->countdown_frame             = GTK_WIDGET (gtk_builder_get_object (builder, "countdown_frame"));
  priv->effect_frame                = GTK_WIDGET (gtk_builder_get_object (builder, "effect_frame"));
  priv->effect_alignment            = GTK_WIDGET (gtk_builder_get_object (builder, "effect_alignment"));
  priv->fullscreen_popup            = GTK_WIDGET (gtk_builder_get_object (builder, "fullscreen_popup"));
  priv->fullscreen_bar              = GTK_WIDGET (gtk_builder_get_object (builder, "fullscreen_notebook_bar"));
  priv->button_effects_fullscreen   = GTK_WIDGET (gtk_builder_get_object (builder, "button_effects_fullscreen"));
  priv->button_photo_fullscreen     = GTK_WIDGET (gtk_builder_get_object (builder, "button_photo_fullscreen"));
  priv->button_video_fullscreen     = GTK_WIDGET (gtk_builder_get_object (builder, "button_video_fullscreen"));
  priv->button_burst_fullscreen     = GTK_WIDGET (gtk_builder_get_object (builder, "button_burst_fullscreen"));
  priv->take_picture_fullscreen     = GTK_WIDGET (gtk_builder_get_object (builder, "take_picture_fullscreen"));
  priv->label_take_photo_fullscreen =
    GTK_WIDGET (gtk_builder_get_object (builder, "label_take_photo_fullscreen"));
  priv->image_take_photo_fullscreen =
    GTK_WIDGET (gtk_builder_get_object (builder, "image_take_photo_fullscreen"));
  priv->label_photo_fullscreen     = GTK_WIDGET (gtk_builder_get_object (builder, "label_photo_fullscreen"));
  priv->label_video_fullscreen     = GTK_WIDGET (gtk_builder_get_object (builder, "label_video_fullscreen"));
  priv->countdown_frame_fullscreen =
    GTK_WIDGET (gtk_builder_get_object (builder, "countdown_frame_fullscreen"));
  priv->button_exit_fullscreen = GTK_WIDGET (gtk_builder_get_object (builder, "button_exit_fullscreen"));

  gtk_container_add (GTK_CONTAINER (cheese_window), priv->main_vbox);

  g_object_unref (builder);
}

static void
setup_menubar_and_actions (CheeseWindow *cheese_window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);
  GError     *error = NULL;
  GtkWidget  *menubar;

  priv->ui_manager = gtk_ui_manager_new ();

  priv->actions_main = cheese_window_action_group_new (cheese_window,
                                                                "ActionsMain",
                                                                action_entries_main,
                                                                G_N_ELEMENTS (action_entries_main));

  priv->actions_toggle = cheese_window_radio_action_group_new (cheese_window,
                                                                        "ActionsRadio",
                                                                        action_entries_toggle,
                                                                        G_N_ELEMENTS (action_entries_toggle));

  priv->actions_countdown = cheese_window_toggle_action_group_new (cheese_window,
                                                                            "ActionsCountdown",
                                                                            action_entries_countdown,
                                                                            G_N_ELEMENTS (action_entries_countdown));


  priv->actions_effects = cheese_window_toggle_action_group_new (cheese_window,
                                                                          "ActionsEffects",
                                                                          action_entries_effects,
                                                                          G_N_ELEMENTS (action_entries_effects));

  priv->actions_fullscreen = cheese_window_toggle_action_group_new (cheese_window,
                                                                             "ActionsFullscreen",
                                                                             action_entries_fullscreen,
                                                                             G_N_ELEMENTS (action_entries_fullscreen));

  priv->actions_wide_mode = cheese_window_toggle_action_group_new (cheese_window,
                                                                            "ActionsWideMode",
                                                                            action_entries_wide_mode,
                                                                            G_N_ELEMENTS (action_entries_fullscreen));

  priv->actions_file = cheese_window_action_group_new (cheese_window,
                                                                "ActionsFile",
                                                                action_entries_file,
                                                                G_N_ELEMENTS (action_entries_file));
  gtk_action_group_set_sensitive (priv->actions_file, FALSE);

  priv->actions_photo = cheese_window_action_group_new (cheese_window,
                                                                 "ActionsPhoto",
                                                                 action_entries_photo,
                                                                 G_N_ELEMENTS (action_entries_photo));

  priv->actions_video = cheese_window_toggle_action_group_new (cheese_window,
                                                                        "ActionsVideo",
                                                                        action_entries_video,
                                                                        G_N_ELEMENTS (action_entries_video));
  gtk_action_group_set_sensitive (priv->actions_video, FALSE);
  priv->actions_burst = cheese_window_action_group_new (cheese_window,
                                                                 "ActionsBurst",
                                                                 action_entries_burst,
                                                                 G_N_ELEMENTS (action_entries_burst));
  gtk_action_group_set_sensitive (priv->actions_burst, FALSE);


  gtk_ui_manager_add_ui_from_file (priv->ui_manager, PACKAGE_DATADIR "/cheese-ui.xml", &error);

  if (error)
  {
    g_critical ("building menus from %s failed: %s", PACKAGE_DATADIR "/cheese-ui.xml", error->message);
    g_error_free (error);
  }

  GtkAction *action = gtk_ui_manager_get_action (priv->ui_manager, "/MainMenu/Cheese/CountdownToggle");
  gboolean   countdown;
  g_object_get (priv->gconf, "gconf_prop_countdown", &countdown, NULL);
  if (countdown)
  {
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), TRUE);
  }

  action = gtk_ui_manager_get_action (priv->ui_manager, "/ThumbnailPopup/Delete");
  gboolean enable_delete;
  g_object_get (priv->gconf, "gconf_prop_enable_delete", &enable_delete, NULL);
  gtk_action_set_visible (GTK_ACTION (action), enable_delete);


  menubar = gtk_ui_manager_get_widget (priv->ui_manager, "/MainMenu");
  gtk_box_pack_start (GTK_BOX (priv->main_vbox), menubar, FALSE, FALSE, 0);

  priv->thumb_view_popup_menu = gtk_ui_manager_get_widget (priv->ui_manager,
                                                                    "/ThumbnailPopup");

  gtk_window_add_accel_group (GTK_WINDOW (cheese_window),
                              gtk_ui_manager_get_accel_group (priv->ui_manager));
  gtk_accel_group_connect (gtk_ui_manager_get_accel_group (priv->ui_manager),
                           GDK_Escape, 0, 0,
                           g_cclosure_new_swap (G_CALLBACK (cheese_window_escape_key_cb),
                                                cheese_window, NULL));

  action = gtk_ui_manager_get_action (priv->ui_manager, "/MainMenu/Edit/Effects");
  gtk_activatable_set_related_action (GTK_ACTIVATABLE (priv->button_effects), action);
  gtk_activatable_set_related_action (GTK_ACTIVATABLE (priv->button_effects_fullscreen), action);

  action = gtk_ui_manager_get_action (priv->ui_manager, "/MainMenu/Cheese/Photo");
  gtk_activatable_set_related_action (GTK_ACTIVATABLE (priv->button_photo), action);
  gtk_activatable_set_related_action (GTK_ACTIVATABLE (priv->button_photo_fullscreen), action);

  action = gtk_ui_manager_get_action (priv->ui_manager, "/MainMenu/Cheese/Video");
  gtk_activatable_set_related_action (GTK_ACTIVATABLE (priv->button_video), action);
  gtk_activatable_set_related_action (GTK_ACTIVATABLE (priv->button_video_fullscreen), action);

  action = gtk_ui_manager_get_action (priv->ui_manager, "/MainMenu/Cheese/Burst");
  gtk_activatable_set_related_action (GTK_ACTIVATABLE (priv->button_burst), action);
  gtk_activatable_set_related_action (GTK_ACTIVATABLE (priv->button_burst_fullscreen), action);
}

static void
cheese_window_create_window (CheeseWindow *cheese_window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);
  setup_widgets_from_builder (cheese_window);
  setup_menubar_and_actions (cheese_window);

  /* Problem page */
  priv->problem_page = gtk_vbox_new (FALSE, 0);
  priv->problem_bar  = cheese_no_camera_info_bar_new ();

  g_signal_connect (priv->problem_bar,
                    "response",
                    G_CALLBACK (cheese_window_no_camera_info_bar_response),
                    cheese_window);

  priv->problem_area = gtk_drawing_area_new ();
  gtk_box_pack_start (GTK_BOX (priv->problem_page),
                      priv->problem_bar,
                      FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (priv->problem_page),
                      priv->problem_area,
                      TRUE, TRUE, 0);
  gtk_notebook_insert_page (GTK_NOTEBOOK (priv->notebook),
                            priv->problem_page,
                            gtk_label_new ("got problems"),
                            PAGE_PROBLEM);


  /* configure the popup position and size */
  GdkScreen *screen = gtk_window_get_screen (GTK_WINDOW (priv->fullscreen_popup));
  gtk_window_set_default_size (GTK_WINDOW (priv->fullscreen_popup),
                               gdk_screen_get_width (screen), FULLSCREEN_POPUP_HEIGHT);
  gtk_window_move (GTK_WINDOW (priv->fullscreen_popup), 0,
                   gdk_screen_get_height (screen) - FULLSCREEN_POPUP_HEIGHT);

  g_signal_connect (priv->fullscreen_popup,
                    "enter-notify-event",
                    G_CALLBACK (cheese_window_fullscreen_leave_notify_cb),
                    cheese_window);

  g_signal_connect (priv->button_exit_fullscreen, "clicked",
                    G_CALLBACK (cheese_window_exit_fullscreen_button_clicked_cb),
                    cheese_window);


  char *str = g_strconcat ("<b>", _("_Take a photo"), "</b>", NULL);
  gtk_label_set_text_with_mnemonic (GTK_LABEL (priv->label_take_photo), str);
  gtk_label_set_text_with_mnemonic (GTK_LABEL (priv->label_take_photo_fullscreen), str);
  g_free (str);
  gtk_label_set_use_markup (GTK_LABEL (priv->label_take_photo), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (priv->take_picture), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (priv->label_take_photo_fullscreen), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (priv->take_picture_fullscreen), FALSE);

  priv->thumb_view = cheese_thumb_view_new ();
  priv->thumb_nav  = eog_thumb_nav_new (priv->thumb_view, FALSE);

  gtk_container_add (GTK_CONTAINER (priv->thumb_scrollwindow), priv->thumb_nav);

  /* show the scroll window to get it included in the size requisition done later */
  gtk_widget_show_all (priv->thumb_scrollwindow);

  char *gconf_effects;
  g_object_get (priv->gconf, "gconf_prop_selected_effects", &gconf_effects, NULL);
  priv->effect_chooser = cheese_effect_chooser_new (gconf_effects);
  gtk_container_add (GTK_CONTAINER (priv->effect_frame), priv->effect_chooser);
  g_free (gconf_effects);

/* uncomment to debug */

/*
 * gtk_notebook_set_show_tabs (GTK_NOTEBOOK (priv->notebook), TRUE);
 * gtk_notebook_set_show_border (GTK_NOTEBOOK (priv->notebook), TRUE);
 */

  priv->throbber       = gtk_spinner_new ();
  priv->throbber_box   = gtk_event_box_new ();
  priv->throbber_align = gtk_alignment_new (0.5, 0.5, 0.6, 0.6);
  gtk_container_add (GTK_CONTAINER (priv->throbber_box), priv->throbber_align);
  gtk_container_add (GTK_CONTAINER (priv->throbber_align), priv->throbber);
  gtk_notebook_insert_page (GTK_NOTEBOOK (priv->notebook),
                            priv->throbber_box,
                            gtk_label_new ("spinner"),
                            PAGE_SPINNER);
  cheese_window_spinner_invert (priv->throbber, priv->throbber_box);
  gtk_widget_show_all (priv->throbber_box);

  priv->countdown = cheese_countdown_new ();
  gtk_container_add (GTK_CONTAINER (priv->countdown_frame), priv->countdown);
  gtk_widget_show (priv->countdown);

  priv->countdown_fullscreen = cheese_countdown_new ();
  gtk_container_add (GTK_CONTAINER (priv->countdown_frame_fullscreen), priv->countdown_fullscreen);

  gtk_widget_realize (priv->screen);
  GdkWindow *win = gtk_widget_get_window (priv->screen);
  if (!gdk_window_ensure_native (win))
  {
    /* FIXME: this breaks offscreen stuff, we should really find
     * another way to embed video that doesn't require an XID */

    /* abort: no native window, no xoverlay, no cheese. */
    g_error ("Could not create a native X11 window for the drawing area");
  }
  gdk_window_set_back_pixmap (gtk_widget_get_window (priv->screen), NULL, FALSE);
  gtk_widget_set_app_paintable (priv->screen, TRUE);
  gtk_widget_set_double_buffered (priv->screen, FALSE);
  gtk_widget_add_events (priv->screen, GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);

  /* Listen for key presses */
  gtk_widget_add_events (GTK_WIDGET (cheese_window), GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
  g_signal_connect (cheese_window, "key_press_event",
                    G_CALLBACK (cheese_window_key_press_event_cb), cheese_window);
  g_signal_connect (priv->take_picture, "clicked",
                    G_CALLBACK (cheese_window_action_button_clicked_cb), cheese_window);
  g_signal_connect (priv->take_picture_fullscreen, "clicked",
                    G_CALLBACK (cheese_window_action_button_clicked_cb), cheese_window);
  g_signal_connect (priv->thumb_view, "selection_changed",
                    G_CALLBACK (cheese_window_selection_changed_cb), cheese_window);
  g_signal_connect (priv->thumb_view, "button_press_event",
                    G_CALLBACK (cheese_window_button_press_event_cb), cheese_window);
}

void
setup_camera (CheeseWindow *cheese_window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (cheese_window);
  char   *camera_device = NULL;
  int     x_resolution;
  int     y_resolution;
  gdouble brightness;
  gdouble contrast;
  gdouble saturation;
  gdouble hue;

  GError *error;

  g_object_get (priv->gconf,
                "gconf_prop_x_resolution", &x_resolution,
                "gconf_prop_y_resolution", &y_resolution,
                "gconf_prop_camera", &camera_device,
                "gconf_prop_brightness", &brightness,
                "gconf_prop_contrast", &contrast,
                "gconf_prop_saturation", &saturation,
                "gconf_prop_hue", &hue,
                NULL);

  gdk_threads_enter ();
  priv->camera = cheese_camera_new (priv->screen,
                                             camera_device, x_resolution,
                                             y_resolution);
  gdk_threads_leave ();

  g_free (camera_device);

  error = NULL;
  cheese_camera_setup (priv->camera, NULL, &error);
  if (error != NULL)
  {
    if (error->code == CHEESE_CAMERA_ERROR_NO_DEVICE)
    {
      gdk_threads_enter ();
      gtk_spinner_stop (GTK_SPINNER (priv->throbber));
      cheese_window_set_problem_page (cheese_window, "cheese-no-camera");
      gdk_threads_leave ();
      return;
    }
    GtkWidget *dialog;
    gchar     *primary, *secondary;

    primary   = g_strdup (_("Check your GStreamer installation"));
    secondary = g_strdup (error->message);

    gdk_threads_enter ();

    dialog = gtk_message_dialog_new (NULL,
                                     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_OK,
                                     "%s", primary);
    gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog),
                                                "%s", secondary);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    g_error_free (error);
    g_free (primary);
    g_free (secondary);

    /* Clean up and exit */
    /* FIXME: handle errors in the infobar and remove this shit */
    gtk_widget_destroy (GTK_WIDGET (cheese_window));

    gdk_threads_leave ();

    return;
  }

  g_signal_connect (priv->camera, "photo-saved",
                    G_CALLBACK (cheese_window_photo_saved_cb), cheese_window);
  g_signal_connect (priv->camera, "video-saved",
                    G_CALLBACK (cheese_window_video_saved_cb), cheese_window);

  cheese_camera_set_effect (priv->camera,
                            cheese_effect_chooser_get_selection (CHEESE_EFFECT_CHOOSER (priv->effect_chooser)));

  cheese_camera_set_balance_property (priv->camera, "brightness", brightness);
  cheese_camera_set_balance_property (priv->camera, "contrast", contrast);
  cheese_camera_set_balance_property (priv->camera, "saturation", saturation);
  cheese_camera_set_balance_property (priv->camera, "hue", hue);

  cheese_camera_play (priv->camera);
  gdk_threads_enter ();
  gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), PAGE_WEBCAM);
  gtk_spinner_stop (GTK_SPINNER (priv->throbber));

  gtk_widget_set_sensitive (GTK_WIDGET (priv->take_picture), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (priv->take_picture_fullscreen), TRUE);
  gtk_action_group_set_sensitive (priv->actions_effects, TRUE);
  gdk_threads_leave ();
}

void
cheese_window_init (CheeseWindow *window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (window);

//  priv->startup_hal_dev_udi = hal_dev_udi;
  priv->gconf               = cheese_gconf_new ();
  priv->fileutil            = cheese_fileutil_new ();
  priv->flash               = cheese_flash_new (NULL);
  priv->isFullscreen        = FALSE;
  priv->is_bursting         = FALSE;
  priv->startup_wide        = FALSE;

  priv->fullscreen_timeout_source = NULL;

  cheese_window_create_window (window);
  g_object_set (G_OBJECT (priv->flash), "parent", GTK_WIDGET (window), NULL);

  gtk_action_group_set_sensitive (priv->actions_effects, FALSE);

  gtk_spinner_start (GTK_SPINNER (priv->throbber));

  gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), PAGE_SPINNER);

  priv->camera_mode = CAMERA_MODE_PHOTO;
  priv->recording   = FALSE;

  /* Run cam setup in its own thread */
  GError *error = NULL;
  if (!g_thread_create ((GThreadFunc) setup_camera, window, FALSE, &error))
  {
    g_error ("Failed to create setup thread: %s\n", error->message);
    g_error_free (error);
    return;
  }
}

static void
cheese_window_constructed (GObject *object)
{
  CheeseWindow *window = CHEESE_WINDOW (object);
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (window);

  gboolean startup_wide_saved;

  g_object_get (priv->gconf,
                "gconf_prop_wide_mode",
                &startup_wide_saved,
                NULL);

  priv->startup_wide = startup_wide_saved ? TRUE : priv->startup_wide;

  if (priv->startup_wide)
  {
    GtkAction *action = gtk_ui_manager_get_action (priv->ui_manager, "/MainMenu/Cheese/WideMode");
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), TRUE);
  }

  /* handy trick to set default size of the drawing area while not
   * limiting its minimum size, thanks Owen! */
  GtkRequisition req;
  gtk_widget_set_size_request (priv->notebook,
                               DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
  gtk_widget_size_request (GTK_WIDGET (window), &req);
  gtk_window_resize (GTK_WINDOW (window), req.width, req.height);
  gtk_widget_set_size_request (priv->notebook, -1, -1);

  gtk_widget_show_all (priv->main_vbox);
}

static void
cheese_window_dispose (GObject *object)
{
  CheeseWindow *window = CHEESE_WINDOW (object);
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (window);

  /* FIXME: fix the flash to be a gtk_window subclass */
  g_object_unref (priv->flash);
  G_OBJECT_CLASS (cheese_window_parent_class)->dispose (object);
}

static void
cheese_window_finalize (GObject *object)
{
  CheeseWindow *window = CHEESE_WINDOW (object);
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (window);

  g_object_unref (priv->camera);
  g_object_unref (priv->fileutil);
  g_object_unref (priv->actions_main);
  g_object_unref (priv->actions_countdown);
  g_object_unref (priv->actions_effects);
  g_object_unref (priv->actions_file);
  g_object_unref (priv->actions_photo);
  g_object_unref (priv->actions_toggle);
  g_object_unref (priv->actions_effects);
  g_object_unref (priv->actions_file);
  g_object_unref (priv->actions_video);
  g_object_unref (priv->actions_burst);
  g_object_unref (priv->actions_fullscreen);
  g_object_unref (priv->gconf);

  G_OBJECT_CLASS (cheese_window_parent_class)->finalize (object);
  gtk_main_quit ();
}

static void
cheese_window_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  CheeseWindow *window = CHEESE_WINDOW (object);
  CheeseWindowPrivate *priv =
    CHEESE_WINDOW_GET_PRIVATE (window);

  switch (prop_id)
  {
    case PROP_STARTUP_WIDE:
      g_value_set_boolean (value, priv->startup_wide);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
cheese_window_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  CheeseWindow *window = CHEESE_WINDOW (object);
  CheeseWindowPrivate *priv =
    CHEESE_WINDOW_GET_PRIVATE (window);

  switch (prop_id)
  {
    case PROP_STARTUP_WIDE:
      priv->startup_wide = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
cheese_window_class_init (CheeseWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = cheese_window_finalize;
  object_class->dispose = cheese_window_dispose;
  object_class->constructed = cheese_window_constructed;
  object_class->get_property = cheese_window_get_property;
  object_class->set_property = cheese_window_set_property;

 g_object_class_install_property (object_class, PROP_STARTUP_WIDE,
                                  g_param_spec_boolean ("startup-wide",
                                                        NULL, NULL, FALSE,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (object_class, sizeof(CheeseWindowPrivate));
}

CheeseThumbView *
cheese_window_get_thumbview (CheeseWindow *window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (window);
  return CHEESE_THUMB_VIEW (priv->thumb_view);
}

CheeseCamera *
cheese_window_get_camera (CheeseWindow *window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (window);
  return priv->camera;
}

CheeseGConf *
cheese_window_get_gconf (CheeseWindow *window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (window);
  return priv->gconf;
}

CheeseFileUtil *
cheese_window_get_fileutil (CheeseWindow *window)
{
  CheeseWindowPrivate *priv = CHEESE_WINDOW_GET_PRIVATE (window);
  return priv->fileutil;
}
