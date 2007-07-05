#ifndef __THUMBNAILS_H__
#define __THUMBNAILS_H__

void create_thumbnails_store();
void fill_thumbs();
void append_photo(gchar *);
void remove_photo(gchar *);

struct _thumbnails
{
  GtkListStore *store;
  GtkTreeIter iter;
  GtkWidget *iconview;
  //FIXME: add some structure to save thumbnails
};

extern struct _thumbnails thumbnails;

#endif /* __THUMBNAILS_H__ */
