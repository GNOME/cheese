libcheese_la_CFLAGS = \
	-DDATADIR=\"$(datadir)\" \
	-DPREFIX=\""$(prefix)"\" \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
	-DPACKAGE_DATADIR=\""$(datadir)/cheese"\" \
	-DPACKAGE_LOCALEDIR=\""$(datadir)/locale"\" \
	-DAPPNAME_DATA_DIR=\"$(pkgdatadir)\" \
	-DGNOME_DESKTOP_USE_UNSTABLE_API=1 \
	$(CHEESE_CFLAGS)

lib_LTLIBRARIES = libcheese.la

libcheese_la_CFLAGS = $(CHEESE_CFLAGS)

libcheese_la_SOURCES = \
	cheese-camera.c \
	$(NULL)

include_HEADERS = cheese-camera.h

libcheese_la_LIBADD = \
	$(CHEESE_LIBS)
