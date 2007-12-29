/*
 * Copyright (C) 2007 daniel g. siegel <dgsiegel@gmail.com>
 * Copyright (C) 2007 Jaap Haitsma <jaap@haitsma.org>
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
#include <cheese-config.h>
#endif

#include <glib.h>
#include <gtk/gtk.h>
#include <libgnomeui/libgnomeui.h>
#include <libgnomevfs/gnome-vfs.h>
#include <glade/glade.h>

#include "cheese-fileutil.h"
#include "eog-thumbnail.h"

#include "cheese-thumb-view.h"


#define CHEESE_THUMB_VIEW_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), CHEESE_TYPE_THUMB_VIEW, CheeseThumbViewPrivate))

G_DEFINE_TYPE (CheeseThumbView, cheese_thumb_view, GTK_TYPE_ICON_VIEW);


typedef struct
{
  GtkListStore *store;
  GnomeVFSMonitorHandle *monitor_handle;
} CheeseThumbViewPrivate;

enum
{
  THUMBNAIL_PIXBUF_COLUMN,
  THUMBNAIL_URL_COLUMN
};

/* Drag 'n Drop */
enum 
{
  TARGET_PLAIN,
  TARGET_PLAIN_UTF8,
  TARGET_URILIST,
};

static GtkTargetEntry target_table[] = 
{
  { "text/uri-list", 0, TARGET_URILIST },
};

static void
cheese_thumb_view_append_item (CheeseThumbView *thumb_view, char *filename)
{
  CheeseThumbViewPrivate* priv = CHEESE_THUMB_VIEW_GET_PRIVATE (thumb_view);
  GtkTreeIter iter;
  GdkPixbuf *pixbuf = NULL;
  GnomeThumbnailFactory *factory;
  GnomeVFSFileInfo *file_info;
  char *uri;
  char *thumb_loc;
  GtkTreePath *path;

  file_info = gnome_vfs_file_info_new ();
  uri = g_filename_to_uri (filename, NULL, NULL);
  if (!uri || (gnome_vfs_get_file_info (uri, file_info,
                                        GNOME_VFS_FILE_INFO_DEFAULT |
                                        GNOME_VFS_FILE_INFO_GET_MIME_TYPE) != GNOME_VFS_OK))
  {
    g_warning ("Invalid filename\n");
    return;
  }

  factory = gnome_thumbnail_factory_new (GNOME_THUMBNAIL_SIZE_NORMAL);

  thumb_loc = gnome_thumbnail_factory_lookup (factory, uri, file_info->mtime);

  if (!thumb_loc)
  {
    pixbuf = gnome_thumbnail_factory_generate_thumbnail (factory, uri, file_info->mime_type);
    if (!pixbuf)
    {
      g_warning ("could not load %s (%s)\n", filename, file_info->mime_type);
      return;
    }
    gnome_thumbnail_factory_save_thumbnail (factory, pixbuf, uri, file_info->mtime);
  }
  else
  {
    pixbuf = gdk_pixbuf_new_from_file (thumb_loc, NULL);
    if (!pixbuf)
    {
      g_warning ("could not load %s (%s)\n", filename, file_info->mime_type);
      return;
    }
  }

  eog_thumbnail_add_frame (&pixbuf);

  gtk_list_store_append (priv->store, &iter);
  gtk_list_store_set (priv->store, &iter, THUMBNAIL_PIXBUF_COLUMN,
                      pixbuf, THUMBNAIL_URL_COLUMN, filename, -1);

  path = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->store), &iter);
  gtk_icon_view_scroll_to_path (GTK_ICON_VIEW (thumb_view), path,
                                TRUE, 1.0, 0.5);

  g_object_unref (pixbuf);
}

static void
cheese_thumb_view_remove_item (CheeseThumbView *thumb_view, char *filename)
{
  CheeseThumbViewPrivate* priv = CHEESE_THUMB_VIEW_GET_PRIVATE (thumb_view);
  char *path;
  GtkTreeIter iter;

  gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->store), &iter);
  /* check if the selected item is the first, else go through the store */
  gtk_tree_model_get (GTK_TREE_MODEL (priv->store), &iter, THUMBNAIL_URL_COLUMN, &path, -1);
  if (g_ascii_strcasecmp (path, filename)) 
  {
    while (gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->store), &iter))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (priv->store), &iter, THUMBNAIL_URL_COLUMN, &path, -1);
      if (!g_ascii_strcasecmp (path, filename))
        break;
    }
  }

  gtk_list_store_remove (priv->store, &iter);
}

static void
cheese_thumb_view_monitor_cb (GnomeVFSMonitorHandle *monitor_handle,
                              const char *monitor_uri,
                              const char *info_uri,
                              GnomeVFSMonitorEventType event_type,
                              CheeseThumbView *thumb_view)
{
  char *filename = gnome_vfs_get_local_path_from_uri (info_uri);
  gboolean is_dir;

  is_dir = g_file_test (filename, G_FILE_TEST_IS_DIR);

  if (!is_dir)
  {
    switch (event_type)
    {
      case GNOME_VFS_MONITOR_EVENT_DELETED:
        cheese_thumb_view_remove_item (thumb_view, filename);
        break;
      case GNOME_VFS_MONITOR_EVENT_CREATED:
        cheese_thumb_view_append_item (thumb_view, filename);
        break;
      default:
        break;
    }
  }
  g_free (filename);
}


static void
cheese_thumb_view_on_drag_data_get_cb (GtkIconView      *thumb_view,
                                       GdkDragContext   *drag_context,
                                       GtkSelectionData *data,
                                       guint             info,
                                       guint             time,
                                       gpointer          user_data)
{
  GList *list;
  GtkTreePath *tree_path = NULL;
  GtkTreeIter iter;
  GtkTreeModel *model;
  char *str;
  char *uris = NULL;
  char *tmp_str;
  gint i;

  list = gtk_icon_view_get_selected_items (thumb_view);
  model = gtk_icon_view_get_model (thumb_view);

  for (i = 0; i < g_list_length (list); i++)
  {
    tree_path = g_list_nth_data (list, i);
    gtk_tree_model_get_iter (model, &iter, tree_path);
    gtk_tree_model_get (model, &iter, 1, &str, -1);

    /* build the "text/uri-list" string */
    if (uris) 
    {
      tmp_str = g_strconcat (uris, str, "\r\n", NULL);
      g_free (uris);
    } 
    else 
    { 
      tmp_str = g_strconcat (str, "\r\n", NULL);
    }
    uris = tmp_str;

    g_free (str);
  }
  gtk_selection_data_set (data, data->target, 8,
                          (guchar*) uris, strlen (uris));
  g_free (uris);
  g_list_free (list);
}


static char *
cheese_thumb_view_get_url_from_path (CheeseThumbView *thumb_view, GtkTreePath *path)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  char *file;

  model = gtk_icon_view_get_model (GTK_ICON_VIEW (thumb_view));
  gtk_tree_model_get_iter (model, &iter, path);

  gtk_tree_model_get (model, &iter, THUMBNAIL_URL_COLUMN, &file, -1);
			    
  return file;
}

char *
cheese_thumb_view_get_selected_image (CheeseThumbView *thumb_view)
{
  GList *list;
  char *filename = NULL;

  list = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (thumb_view));
  if (list)
  {
    filename = cheese_thumb_view_get_url_from_path (thumb_view, (GtkTreePath *) list->data);
    g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
    g_list_free (list);
  }

  return filename;
}

GList *
cheese_thumb_view_get_selected_images_list (CheeseThumbView *thumb_view)
{
  GList *l, *item;
  GList *list = NULL;

  GtkTreePath *path;

  l = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (thumb_view));

  for (item = l; item != NULL; item = item->next) 
  {
    path = (GtkTreePath *) item->data;
    list = g_list_prepend (list, cheese_thumb_view_get_url_from_path (thumb_view, path));
    gtk_tree_path_free (path);
  }

  g_list_free (l);
  list = g_list_reverse (list);

  return list;
}

static void
cheese_thumb_view_fill (CheeseThumbView *thumb_view)
{
  CheeseThumbViewPrivate* priv = CHEESE_THUMB_VIEW_GET_PRIVATE (thumb_view);
  GDir *dir;
  char *path;
  const char *name;
  char *filename;

  gtk_list_store_clear (priv->store);

  path = cheese_fileutil_get_media_path ();
  dir = g_dir_open (path, 0, NULL);
  if (!dir)
    return;

  while ((name = g_dir_read_name (dir)))
  {
    if (!(g_str_has_suffix (name, PHOTO_NAME_SUFFIX) || g_str_has_suffix (name, VIDEO_NAME_SUFFIX)))
      continue;

    filename = g_build_filename (path, name, NULL);
    cheese_thumb_view_append_item (thumb_view, filename);
    g_free (filename);
  }

  g_free (path);
  g_dir_close (dir);
}

static void
cheese_thumb_view_finalize (GObject *object)
{
  CheeseThumbView *thumb_view;

  thumb_view = CHEESE_THUMB_VIEW (object);
  CheeseThumbViewPrivate *priv = CHEESE_THUMB_VIEW_GET_PRIVATE (thumb_view);  

  g_object_unref (priv->store);
  gnome_vfs_monitor_cancel (priv->monitor_handle);

  G_OBJECT_CLASS (cheese_thumb_view_parent_class)->finalize (object);
}

static void
cheese_thumb_view_class_init (CheeseThumbViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = cheese_thumb_view_finalize;

  g_type_class_add_private (klass, sizeof (CheeseThumbViewPrivate));
}


static void
cheese_thumb_view_init (CheeseThumbView *thumb_view)
{
  CheeseThumbViewPrivate* priv = CHEESE_THUMB_VIEW_GET_PRIVATE (thumb_view);
  char *path = NULL;
  GnomeVFSURI *uri;  
  const int THUMB_VIEW_HEIGHT = 120;

  eog_thumbnail_init ();

  priv->store = gtk_list_store_new (2, GDK_TYPE_PIXBUF, G_TYPE_STRING);

  gtk_icon_view_set_model (GTK_ICON_VIEW (thumb_view), GTK_TREE_MODEL (priv->store));

  gtk_widget_set_size_request (GTK_WIDGET (thumb_view), -1, THUMB_VIEW_HEIGHT);

  path = cheese_fileutil_get_media_path ();
  uri = gnome_vfs_uri_new (path);

  if (!gnome_vfs_uri_exists (uri))
  {
    gnome_vfs_make_directory_for_uri (uri, 0775);
    g_mkdir_with_parents (path, 0775);
  }

  gnome_vfs_monitor_add (&(priv->monitor_handle), path, GNOME_VFS_MONITOR_DIRECTORY,
                         (GnomeVFSMonitorCallback) cheese_thumb_view_monitor_cb, thumb_view);
  g_free (path);
  gnome_vfs_uri_unref (uri);

  gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (thumb_view), 0);
  gtk_icon_view_set_columns (GTK_ICON_VIEW (thumb_view), G_MAXINT);

  gtk_icon_view_enable_model_drag_source (GTK_ICON_VIEW (thumb_view), GDK_BUTTON1_MASK,
                                          target_table, G_N_ELEMENTS (target_table),
                                          GDK_ACTION_COPY);
  g_signal_connect (G_OBJECT (thumb_view), "drag-data-get",
                    G_CALLBACK (cheese_thumb_view_on_drag_data_get_cb), NULL);

  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(priv->store),
                                       THUMBNAIL_URL_COLUMN, GTK_SORT_ASCENDING);

  cheese_thumb_view_fill (thumb_view);
}

GtkWidget * 
cheese_thumb_view_new ()
{
  CheeseThumbView *thumb_view;

  thumb_view = g_object_new (CHEESE_TYPE_THUMB_VIEW, NULL);  
  return GTK_WIDGET (thumb_view);
}
