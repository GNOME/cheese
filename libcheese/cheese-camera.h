/*
 * Copyright © 2007,2008 Jaap Haitsma <jaap@haitsma.org>
 * Copyright © 2007-2009 daniel g. siegel <dgsiegel@gnome.org>
 * Copyright © 2008 Ryan zeigler <zeiglerr@gmail.com>
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


#ifndef __CHEESE_CAMERA_H__
#define __CHEESE_CAMERA_H__

#include <glib-object.h>
#include <gtk/gtk.h>
#include <gst/interfaces/xoverlay.h>
#include <cheese-camera-device.h>

G_BEGIN_DECLS

#define CHEESE_TYPE_CAMERA (cheese_camera_get_type ())
#define CHEESE_CAMERA(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CHEESE_TYPE_CAMERA, CheeseCamera))
#define CHEESE_CAMERA_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), CHEESE_TYPE_CAMERA, CheeseCameraClass))
#define CHEESE_IS_CAMERA(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CHEESE_TYPE_CAMERA))
#define CHEESE_IS_CAMERA_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CHEESE_TYPE_CAMERA))
#define CHEESE_CAMERA_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CHEESE_TYPE_CAMERA, CheeseCameraClass))

typedef enum
{
  CHEESE_CAMERA_EFFECT_NO_EFFECT       = (0),
  CHEESE_CAMERA_EFFECT_MAUVE           = (1 << 0),
  CHEESE_CAMERA_EFFECT_NOIR_BLANC      = (1 << 1),
  CHEESE_CAMERA_EFFECT_SATURATION      = (1 << 2),
  CHEESE_CAMERA_EFFECT_HULK            = (1 << 3),
  CHEESE_CAMERA_EFFECT_VERTICAL_FLIP   = (1 << 4),
  CHEESE_CAMERA_EFFECT_HORIZONTAL_FLIP = (1 << 5),
  CHEESE_CAMERA_EFFECT_SHAGADELIC      = (1 << 6),
  CHEESE_CAMERA_EFFECT_VERTIGO         = (1 << 7),
  CHEESE_CAMERA_EFFECT_EDGE            = (1 << 8),
  CHEESE_CAMERA_EFFECT_DICE            = (1 << 9),
  CHEESE_CAMERA_EFFECT_WARP            = (1 << 10),
}
CheeseCameraEffect;

typedef struct
{
  GObject parent;
} CheeseCamera;

typedef struct
{
  GObjectClass parent_class;
  void (*photo_saved)(CheeseCamera *camera);
  void (*photo_taken)(CheeseCamera *camera, GdkPixbuf *pixbuf);
  void (*video_saved)(CheeseCamera *camera);
} CheeseCameraClass;

enum CheeseCameraError
{
  CHEESE_CAMERA_ERROR_UNKNOWN,
  CHEESE_CAMERA_ERROR_ELEMENT_NOT_FOUND,
  CHEESE_CAMERA_ERROR_NO_DEVICE
};

GType         cheese_camera_get_type (void);
CheeseCamera *cheese_camera_new (GtkWidget *video_window,
                                 char      *camera_device_name,
                                 int        x_resolution,
                                 int        y_resolution);

const CheeseVideoFormat *cheese_camera_get_current_video_format (CheeseCamera *camera);
void                     cheese_camera_setup (CheeseCamera *camera, char *udi, GError **error);
void                     cheese_camera_play (CheeseCamera *camera);
void                     cheese_camera_stop (CheeseCamera *camera);
void                     cheese_camera_set_effect (CheeseCamera *camera, CheeseCameraEffect effect);
void                     cheese_camera_start_video_recording (CheeseCamera *camera, char *filename);
void                     cheese_camera_stop_video_recording (CheeseCamera *camera);
gboolean                 cheese_camera_take_photo (CheeseCamera *camera, char *filename);
gboolean                 cheese_camera_take_photo_pixbuf (CheeseCamera *camera);
gboolean                 cheese_camera_has_camera (CheeseCamera *camera);
int                      cheese_camera_get_num_camera_devices (CheeseCamera *camera);
CheeseCameraDevice *     cheese_camera_get_selected_device (CheeseCamera *camera);
GPtrArray *              cheese_camera_get_camera_devices (CheeseCamera *camera);
void                     cheese_camera_set_device_by_dev_file (CheeseCamera *camera, char *file);
void                     cheese_camera_set_device_by_dev_udi (CheeseCamera *camera, char *udi);
gboolean                 cheese_camera_switch_camera_device (CheeseCamera *camera);
GList *                  cheese_camera_get_video_formats (CheeseCamera *camera);
void                     cheese_camera_set_video_format (CheeseCamera      *camera,
                                                         CheeseVideoFormat *format);
gboolean cheese_camera_get_balance_property_range (CheeseCamera *camera,
                                                   gchar *property,
                                                   gdouble *min, gdouble *max, gdouble *def);
void cheese_camera_set_balance_property (CheeseCamera *camera, gchar *property, gdouble value);
G_END_DECLS

#endif /* __CHEESE_CAMERA_H__ */
