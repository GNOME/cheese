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
	cheese-avatar-chooser.c \
	cheese-avatar-chooser.h \
	um-crop-area.c \
	um-crop-area.h \
	cheese-widget.c \
	cheese-widget.h \
	$(NULL)

# FIXME when we have a .pc file, and sonames
cheesedir = $(includedir)/cheese
cheese_HEADERS = cheese-widget.h cheese-avatar-chooser.h

libcheese_gtk_la_LIBADD = \
	libcheesecommon.la \
	$(CHEESE_LIBS)
libcheese_gtk_la_LDFLAGS = \
	-version-info $(CHEESE_LT_VERSION)		\
	-export-symbols $(srcdir)/cheese-gtk.symbols	\
	-no-undefined					\
	$(AM_LDFLAGS)

