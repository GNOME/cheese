
#include <glade/glade.h>

#define GLADE_FILE  "cheese.glade"

struct _widgets
{
  GtkWidget *take_picture;
  GtkWidget *screen;
  GtkWidget *button_video;
  GtkWidget *button_photo;
  GtkWidget *button_effects_left;
  GtkWidget *button_effects_right;
  GtkWidget *label_effects;
  GtkWidget *menubar;
  GtkWidget *file_menu;
  GtkWidget *help_menu;
};

struct _cheese_window
{
  GladeXML *gxml;
  GtkWidget *window;
  struct _widgets widgets;
};

extern struct _cheese_window cheese_window;

void create_window();
void set_effects_label(gchar *effect);
