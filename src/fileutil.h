#ifndef __FILE_UTIL_H__
#define __FILE_UTIL_H__

gchar *get_cheese_path(void);
gchar *get_cheese_filename(int);
void photos_monitor_cb(GnomeVFSMonitorHandle *, const gchar *, const gchar *, GnomeVFSMonitorEventType);

#endif /* __FILE_UTIL_H__ */
