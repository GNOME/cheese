#include <gtk/gtk.h>

#include <glib.h>

#include "cheese_config.h"

//FIXME: provide option to choose the folder
#define SAVE_FOLDER_DEFAULT  	 "images/"
//FIXME: provide option to choose the naming of the photos
#define PHOTO_NAME_DEFAULT	 "Picture"
//FIXME: provide option to choose the format
#define PHOTO_NAME_SUFFIX_DEFAULT ".jpg"
#define PHOTO_WIDTH 640
#define PHOTO_HEIGHT 480
#define THUMB_WIDTH (PHOTO_WIDTH / 5)
#define THUMB_HEIGHT (PHOTO_HEIGHT / 5)

void create_photo(unsigned char *data);

gboolean set_screen_x_window_id();
void on_cheese_window_close_cb(GtkWidget *widget, gpointer data);
