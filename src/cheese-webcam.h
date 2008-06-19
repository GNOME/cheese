/*
 * Copyright (C) 2007,2008 Jaap Haitsma <jaap@haitsma.org>
 * Copyright (C) 2007,2008 daniel g. siegel <dgsiegel@gmail.com>
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


#ifndef __CHEESE_WEBCAM_H__
#define __CHEESE_WEBCAM_H__

#include <glib-object.h>
#include <gtk/gtk.h>
#include <gst/interfaces/xoverlay.h>

G_BEGIN_DECLS

#define CHEESE_TYPE_WEBCAM (cheese_webcam_get_type ())
#define CHEESE_WEBCAM(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), CHEESE_TYPE_WEBCAM, CheeseWebcam))
#define CHEESE_WEBCAM_CLASS(k) (G_TYPE_CHECK_CLASS_CAST((k), CHEESE_TYPE_WEBCAM, CheeseWebcamClass))
#define CHEESE_IS_WEBCAM(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), CHEESE_TYPE_WEBCAM))
#define CHEESE_IS_WEBCAM_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), CHEESE_TYPE_WEBCAM))
#define CHEESE_WEBCAM_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CHEESE_TYPE_WEBCAM, CheeseWebcamClass))

typedef struct 
{
  int numerator;
  int denominator;
} CheeseFramerate;

typedef struct 
{
  char *mimetype;
  int width;
  int height;
  int num_framerates;
  CheeseFramerate *framerates; 
} CheeseVideoFormat;

typedef struct
{
  char *video_device; 
  char *gstreamer_src;
  char *product_name; 
  int num_video_formats;
  GArray *video_formats;
} CheeseWebcamDevice;

typedef enum
{
  CHEESE_WEBCAM_EFFECT_NO_EFFECT       = (0),
  CHEESE_WEBCAM_EFFECT_MAUVE           = (1 << 0),
  CHEESE_WEBCAM_EFFECT_NOIR_BLANC      = (1 << 1),
  CHEESE_WEBCAM_EFFECT_SATURATION      = (1 << 2),
  CHEESE_WEBCAM_EFFECT_HULK            = (1 << 3),
  CHEESE_WEBCAM_EFFECT_VERTICAL_FLIP   = (1 << 4),
  CHEESE_WEBCAM_EFFECT_HORIZONTAL_FLIP = (1 << 5),
  CHEESE_WEBCAM_EFFECT_SHAGADELIC      = (1 << 6),
  CHEESE_WEBCAM_EFFECT_VERTIGO         = (1 << 7),
  CHEESE_WEBCAM_EFFECT_EDGE            = (1 << 8),
  CHEESE_WEBCAM_EFFECT_DICE            = (1 << 9),
  CHEESE_WEBCAM_EFFECT_WARP            = (1 << 10),
}
CheeseWebcamEffect;

typedef struct
{
  GObject parent;
} CheeseWebcam;

typedef struct
{
  GObjectClass parent_class;
  void (* photo_saved) (CheeseWebcam *webcam);
  void (* video_saved) (CheeseWebcam *webcam);
} CheeseWebcamClass;


GType              cheese_webcam_get_type (void);
CheeseWebcam      *cheese_webcam_new (GtkWidget *video_window, 
                                      char *webcam_device_name, 
                                      int x_resolution,
                                      int y_resolution);
CheeseVideoFormat *cheese_webcam_get_current_video_format (CheeseWebcam *webcam);
void               cheese_webcam_setup (CheeseWebcam *webcam, GError **error);
void               cheese_webcam_play (CheeseWebcam *webcam);
void               cheese_webcam_stop (CheeseWebcam *webcam);
void               cheese_webcam_set_effect (CheeseWebcam *webcam, CheeseWebcamEffect effect);
void               cheese_webcam_start_video_recording (CheeseWebcam *webcam, char *filename);
void               cheese_webcam_stop_video_recording (CheeseWebcam *webcam);
void               cheese_webcam_take_photo (CheeseWebcam *webcam, char *filename);
gboolean           cheese_webcam_has_webcam (CheeseWebcam *webcam);
int                cheese_webcam_get_num_webcam_devices (CheeseWebcam *webcam);
int                cheese_webcam_get_selected_device (CheeseWebcam *webcam);
GArray            *cheese_webcam_get_webcam_devices (CheeseWebcam *webcam);
gboolean           cheese_webcam_switch_webcam_device (CheeseWebcam *webcam);
GArray            *cheese_webcam_get_video_formats (CheeseWebcam *webcam);
void               cheese_webcam_set_video_format (CheeseWebcam *webcam,
                                                   CheeseVideoFormat *format);
G_END_DECLS

#endif /* __CHEESE_WEBCAM_H__ */

