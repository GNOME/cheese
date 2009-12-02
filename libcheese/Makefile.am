INCLUDES = \
	-DDATADIR=\"$(datadir)\" \
	-DPREFIX=\""$(prefix)"\" \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
	-DPACKAGE_DATADIR=\""$(datadir)/cheese"\" \
	-DPACKAGE_LOCALEDIR=\""$(datadir)/locale"\" \
	-DAPPNAME_DATA_DIR=\"$(pkgdatadir)\" \
	-DGNOME_DESKTOP_USE_UNSTABLE_API=1 \
	$(CHEESE_CFLAGS)

lib_LTLIBRARIES = libcheese-gtk.la
noinst_LTLIBRARIES = libcheesecommon.la

libcheese_gtk_la_CFLAGS = $(CHEESE_CFLAGS)

libcheesecommon_la_SOURCES = \
	cheese-gconf.c \
	cheese-gconf.h \
	cheese-fileutil.c \
	cheese-fileutil.h \
	cheese-camera.c \
	cheese-camera.h \
	cheese-camera-device.c \
	cheese-camera-device.h \
	cheese-camera-device-monitor.c \
	cheese-camera-device-monitor.h \
	cheese-flash.h \
	cheese-flash.c \
	cheese-countdown.c \
	cheese-countdown.h

libcheese_gtk_la_SOURCES = \
	cheese-widget.c \
	cheese-widget.h \
	$(NULL)

# FIXME when we have a .pc file, and sonames
# include_HEADERS = cheese-widget.h

libcheese_gtk_la_LIBADD = \
	libcheesecommon.la \
	$(CHEESE_LIBS)

noinst_PROGRAMS = cheese-test-widget cheese-test-monitor
cheese_test_widget_SOURCES = cheese-test-widget.c
cheese_test_widget_LDADD = $(CHEESE_LIBS) libcheese-gtk.la libcheesecommon.la
cheese_test_monitor_SOURCES = cheese-test-monitor.c
cheese_test_monitor_LDADD = $(CHEESE_LIBS) libcheese-gtk.la libcheesecommon.la
