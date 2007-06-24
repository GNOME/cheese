
#include <glade/glade.h>

#define GLADE_FILE "cheese.glade"
//#define GLADE_FILE "/usr/share/cheese/cheese.glade"

struct _widgets
{
  GtkWidget *take_picture;
  GtkWidget *screen;
  GtkWidget *notebook;
  GtkWidget *table;
  GtkWidget *button_video;
  GtkWidget *button_photo;
  GtkWidget *button_effects;
  GtkWidget *label_effects;
  GtkWidget *label_photo;
  GtkWidget *label_video;
  GtkWidget *label_take_photo;
  GtkWidget *menubar;
  GtkWidget *file_menu;
  GtkWidget *help_menu;
  GtkWidget *effects_widget;
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
