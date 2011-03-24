AM_CPPFLAGS = \
	-DDATADIR=\"$(datadir)\" \
	-DPREFIX=\""$(prefix)"\" \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
	-DPACKAGE_DATADIR=\""$(datadir)/cheese"\" \
	-DPACKAGE_LOCALEDIR=\""$(datadir)/locale"\" \
	-DAPPNAME_DATA_DIR=\"$(pkgdatadir)\" \
	-DGNOME_DESKTOP_USE_UNSTABLE_API=1 \
	$(CHEESE_CFLAGS) \
	$(CHEESE_GTK_CFLAGS)

lib_LTLIBRARIES = libcheese.la libcheese-gtk.la

EXTRA_DIST = cheese-gtk.symbols
CLEANFILES =
DISTCLEANFILES =
BUILT_SOURCES =

glib_marshal_list = cheese-marshal.list
glib_marshal_prefix = _cheese_marshal
include $(top_srcdir)/Makefile.am.marshal

glib_enum_h = cheese-enum-types.h
glib_enum_c = cheese-enum-types.c
glib_enum_headers = $(top_srcdir)/libcheese/cheese-widget.h
include $(top_srcdir)/Makefile.am.enums

libcheese_gtk_la_CFLAGS = \
	$(CHEESE_CFLAGS) \
	$(CHEESE_GTK_CFLAGS)

libcheese_la_CFLAGS = \
	$(CHEESE_CFLAGS)

libcheese_la_SOURCES = \
	cheese-fileutil.c \
	cheese-fileutil.h \
	cheese-camera.c \
	cheese-camera.h \
	cheese-camera-device.c \
	cheese-camera-device.h \
	cheese-camera-device-monitor.c \
	cheese-camera-device-monitor.h \
	cheese-effect.c \
	cheese-effect.h \
	$(BUILT_SOURCES)

libcheese_gtk_la_SOURCES = \
	cheese-avatar-chooser.c \
	cheese-avatar-chooser.h \
	cheese-aspect-frame.c \
	cheese-aspect-frame.h \
	um-crop-area.c \
	um-crop-area.h \
	cheese-widget.c \
	cheese-widget.h \
	cheese-widget-private.h \
	cheese-flash.h \
	cheese-flash.c \
	$(NULL)

# FIXME when we have a .pc file, and sonames
cheesedir = $(includedir)/cheese
cheese_HEADERS = cheese-widget.h cheese-avatar-chooser.h cheese-camera.h cheese-effect.h cheese-camera-device.h cheese-camera-device-monitor.h

libcheese_la_LIBADD = \
	$(CHEESE_LIBS)
libcheese_la_LDFLAGS = \
	-version-info $(LIBCHEESE_LT_VERSION) \
	$(AM_LDFLAGS)

libcheese_gtk_la_LIBADD = \
	libcheese.la \
	$(CHEESE_LIBS) \
	$(CHEESE_GTK_LIBS)
libcheese_gtk_la_LDFLAGS = \
	-version-info $(LIBCHEESE_GTK_LT_VERSION)		\
	-export-symbols $(srcdir)/cheese-gtk.symbols	\
	-no-undefined					\
	$(AM_LDFLAGS)

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir) \
	--warn-all
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)
introspection_sources = $(libcheese_la_SOURCES) \
	cheese-marshal.h \
	cheese-enum-types.h

if HAVE_INTROSPECTION
INTROSPECTION_GIRS = Cheese-3.0.gir

Cheese-3.0.gir: libcheese.la
Cheese_3_0_gir_INCLUDES = GObject-2.0 \
	GLib-2.0 \
	Gst-0.10 \
	GstBase-0.10 \
	cairo-1.0 \
	Clutter-1.0 \
	GdkPixbuf-2.0
Cheese_3_0_gir_CFLAGS = -I$(srcdir) $(INCLUDES) \
	$(CHEESE_CFLAGS)
Cheese_3_0_gir_LIBS = libcheese.la
Cheese_3_0_gir_FILES = $(introspection_sources)
Cheese_3_0_gir_SCANNERFLAGS = --verbose --warn-all --pkg-export cheese-3.0

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = Cheese-3.0.typelib

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif
