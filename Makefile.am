ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

SUBDIRS = src po help docs

marshal_data = \
	libcheese/cheese-marshal.c \
	libcheese/cheese-marshal.h

libcheese/cheese-marshal.h: libcheese/cheese-marshal.list
	$(AM_V_GEN)$(GLIB_GENMARSHAL) --header --prefix=cheese_marshal $< > $@

libcheese/cheese-marshal.c: libcheese/cheese-marshal.list libcheese/cheese-marshal.h
	$(AM_V_GEN)$(GLIB_GENMARSHAL) --header --body --prefix=cheese_marshal $< > $@

enum_data = \
	libcheese/cheese-enums.c \
	libcheese/cheese-enums.h

cheese_enum_headers = \
	$(top_srcdir)/libcheese/cheese-widget.h

libcheese/cheese-enums.c: $(cheese_enum_headers) libcheese/cheese-enums.h
	$(AM_V_GEN)$(GLIB_MKENUMS) \
		--fhead "#include \"cheese-enums.h\"\n\n" \
		--fprod "/* enumerations from \"@filename@\" */\n" \
		--fprod "#include \"@filename@\"\n" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n" \
		--vhead "  static GType type = 0;\n\n" \
		--vhead "  if (!type)\n  {\n" \
		--vhead "    static const G@Type@Value _@enum_name@_values[] = {" \
		--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "      { 0, NULL, NULL }\n    };\n\n" \
		--vtail "    type = g_@type@_register_static (\"@EnumName@\", _@enum_name@_values);\n  }\n\n" \
		--vtail "  return type;\n}\n\n" \
		$(cheese_enum_headers) > $@

libcheese/cheese-enums.h: $(cheese_enum_headers)
	$(AM_V_GEN)$(GLIB_MKENUMS) \
		--fhead "#ifndef CHEESE_ENUMS_H_\n" \
		--fhead "#define CHEESE_ENUMS_H_\n\n" \
		--fhead "#include <glib-object.h>\n\n" \
		--fhead "G_BEGIN_DECLS\n\n" \
		--fprod "/* enumerations from @filename@ */\n" \
		--vhead "GType @enum_name@_get_type (void) G_GNUC_CONST;\n" \
		--vhead "#define CHEESE_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
		--ftail "G_END_DECLS\n\n" \
		--ftail "#endif /* CHEESE_ENUMS_H_ */" \
		$(cheese_enum_headers) > $@

AM_CPPFLAGS = \
	-DDATADIR=\"$(datadir)\" \
	-DPREFIX=\""$(prefix)"\" \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
	-DPACKAGE_DATADIR=\""$(pkgdatadir)"\" \
	-DPACKAGE_LOCALEDIR=\""$(datadir)"\" \
	-DGNOME_DESKTOP_USE_UNSTABLE_API=1 \
	$(CHEESE_CFLAGS) \
	$(CHEESE_GTK_FLAGS) \
	$(CHEESE_WFLAGS)

lib_LTLIBRARIES = libcheese.la libcheese-gtk.la

libcheese_gtk_la_CFLAGS = \
	$(CHEESE_CFLAGS) \
	$(CHEESE_GTK_CFLAGS) \
	$(CHEESE_WFLAGS)

libcheese_gtk_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_builddir) \
	-I$(top_builddir)/libcheese \
	-I$(top_srcdir)/libcheese \
	-I$(top_srcdir)/src

libcheese_gtk_la_LIBADD = \
	libcheese.la \
	$(CHEESE_LIBS) \
	$(CHEESE_GTK_LIBS)

libcheese_gtk_la_LDFLAGS = \
	-version-info $(LIBCHEESE_GTK_LT_VERSION) \
	-export-symbols $(top_srcdir)/libcheese/cheese-gtk.symbols \
	-no-undefined \
	$(AM_LDFLAGS)

libcheese_gtk_la_SOURCES = \
	libcheese/cheese-avatar-chooser.c \
	libcheese/cheese-aspect-frame.c \
	libcheese/cheese-flash.c \
	libcheese/cheese-widget.c \
	libcheese/um-crop-area.c

noinst_libcheese_gtk_headers = \
	libcheese/cheese-aspect-frame.h \
	libcheese/cheese-avatar-chooser.h \
	libcheese/cheese-flash.h \
	libcheese/cheese-widget.h \
	libcheese/cheese-widget-private.h \
	libcheese/um-crop-area.h

libcheese_la_CFLAGS = \
	$(CHEESE_CFLAGS) \
	$(CHEESE_GTK_CFLAGS) \
	$(CHEESE_WFLAGS)

libcheese_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_builddir) \
	-I$(top_builddir)/libcheese \
	-I$(top_srcdir)/libcheese

libcheese_la_LIBADD = \
	$(CHEESE_LIBS)

libcheese_la_LDFLAGS = \
	-version-info $(LIBCHEESE_LT_VERSION) \
	$(AM_LDFLAGS)

libcheese_la_SOURCES = \
	libcheese/cheese-enums.c \
	libcheese/cheese-marshal.c \
	libcheese/cheese-camera.c \
	libcheese/cheese-camera-device.c \
	libcheese/cheese-camera-device-monitor.c \
	libcheese/cheese-effect.c \
	libcheese/cheese-fileutil.c

noinst_libcheese_headers = \
	libcheese/cheese-camera.h \
	libcheese/cheese-camera-device.h \
	libcheese/cheese-camera-device-monitor.h \
	libcheese/cheese-effect.h \
	libcheese/cheese-fileutil.h

# FIXME when we have a .pc file, and sonames
cheesedir = $(includedir)/cheese
cheese_HEADERS = \
	libcheese/cheese-avatar-chooser.h \
	libcheese/cheese-camera.h \
	libcheese/cheese-camera-device.h \
	libcheese/cheese-camera-device-monitor.h \
	libcheese/cheese-effect.h \
	libcheese/cheese-widget.h

if HAVE_INTROSPECTION
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_COMPILER_ARGS = --includedir=$(top_srcdir)
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(top_srcdir) --warn-all

INTROSPECTION_GIRS = Cheese-3.0.gir

Cheese-3.0.gir: libcheese.la
Cheese_3_0_gir_CFLAGS = \
	-I$(srcdir) \
	$(INCLUDES) \
	$(CHEESE_CFLAGS)
Cheese_3_0_gir_FILES = \
	$(libcheese_la_SOURCES) \
	libcheese/cheese-marshal.h \
	libcheese/cheese-enums.h
Cheese_3_0_gir_INCLUDES = \
	GObject-2.0 \
	GLib-2.0 \
	Gst-0.10 \
	GstBase-0.10 \
	cairo-1.0 \
	Clutter-1.0 \
	GdkPixbuf-2.0
Cheese_3_0_gir_LIBS = libcheese.la
Cheese_3_0_gir_SCANNERFLAGS = --verbose --warn-all --pkg-export cheese

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = Cheese-3.0.typelib
endif

EXAMPLES = \
	tests/cheese-test-camera \
	tests/cheese-test-chooser \
	tests/cheese-test-flash \
	tests/cheese-test-monitor \
	tests/cheese-test-widget

if HAVE_XTEST
noinst_PROGRAMS = \
	$(EXAMPLES) \
	tests/test-webcam-button
else
noinst_PROGRAMS = $(EXAMPLES)
endif

EXAMPLES_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	$(CHEESE_GTK_CFLAGS) \
	-I$(top_srcdir)/libcheese \
	-I$(top_builddir)

tests_cheese_test_camera_SOURCES = tests/cheese-test-camera.c
tests_cheese_test_chooser_SOURCES = tests/cheese-test-chooser.c
tests_cheese_test_flash_SOURCES = tests/cheese-test-flash.c
tests_cheese_test_monitor_SOURCES = tests/cheese-test-monitor.c
tests_cheese_test_widget_SOURCES = tests/cheese-test-widget.c
tests_test_webcam_button_SOURCES = tests/test-webcam-button.c

tests_cheese_test_camera_CPPFLAGS = $(EXAMPLES_CPPFLAGS)
tests_cheese_test_chooser_CPPFLAGS = $(EXAMPLES_CPPFLAGS)
tests_cheese_test_flash_CPPFLAGS = $(EXAMPLES_CPPFLAGS)
tests_cheese_test_monitor_CPPFLAGS = $(EXAMPLES_CPPFLAGS)
tests_cheese_test_widget_CPPFLAGS = $(EXAMPLES_CPPFLAGS)
tests_test_webcam_button_CPPFLAGS = $(EXAMPLES_CPPFLAGS)

tests_cheese_test_camera_LDADD = \
	$(CHEESE_LIBS) \
	$(CHEESE_GTK_LIBS) \
	libcheese.la
tests_cheese_test_chooser_LDADD = \
	$(CHEESE_LIBS) \
	$(CHEESE_GTK_LIBS) \
	libcheese.la \
	libcheese-gtk.la
tests_cheese_test_flash_LDADD = \
	$(CHEESE_LIBS) \
	$(CHEESE_GTK_LIBS) \
	libcheese.la \
	libcheese-gtk.la
tests_cheese_test_monitor_LDADD = \
	$(CHEESE_LIBS) \
	$(CHEESE_GTK_LIBS) \
	libcheese.la \
	libcheese-gtk.la
tests_cheese_test_widget_LDADD = \
	$(CHEESE_LIBS) \
	$(CHEESE_GTK_LIBS) \
	libcheese.la \
	libcheese-gtk.la
tests_test_webcam_button_LDADD = $(XTEST_LIBS)

@INTLTOOL_DESKTOP_RULE@
desktopdir = $(datadir)/applications
desktop_in_files = data/cheese.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_XML_NOMERGE_RULE@
gsettings_in_file = data/org.gnome.Cheese.gschema.xml.in
gsettings_SCHEMAS = $(gsettings_in_file:.xml.in=.xml)
@GSETTINGS_RULES@

dist_pkgdata_DATA = \
	data/cheese.css \
	data/cheese-about.ui \
	data/cheese-actions.ui \
	data/cheese-main-window.ui \
	data/cheese-prefs.ui \
	data/cheese-viewport.json

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = \
	data/cheese.pc \
	data/cheese-gtk.pc

iconthemedir = $(datadir)/icons/hicolor

appsicon16dir = $(iconthemedir)/16x16/apps
appsicon22dir = $(iconthemedir)/22x22/apps
appsicon24dir = $(iconthemedir)/24x24/apps
appsicon32dir = $(iconthemedir)/32x32/apps
appsicon48dir = $(iconthemedir)/48x48/apps
appsicon256dir = $(iconthemedir)/256x256/apps

actionsicon16dir = $(iconthemedir)/16x16/actions
actionsicon22dir = $(iconthemedir)/22x22/actions
actionsicon24dir = $(iconthemedir)/24x24/actions
actionsicon32dir = $(iconthemedir)/32x32/actions
actionsicon48dir = $(iconthemedir)/48x48/actions
actionsiconscalabledir = $(iconthemedir)/scalable/actions

dist_appsicon16_DATA = data/icons/16x16/cheese.png
dist_appsicon22_DATA = data/icons/22x22/cheese.png
dist_appsicon24_DATA = data/icons/24x24/cheese.png
dist_appsicon32_DATA = data/icons/32x32/cheese.png
dist_appsicon48_DATA = data/icons/48x48/cheese.png
dist_appsicon256_DATA = data/icons/256x256/cheese.png

dist_actionsicon16_DATA = \
	data/icons/16x16/actions/browse-webcam-effects.png \
	data/icons/16x16/actions/cheese-take-burst.png \
	data/icons/16x16/actions/cheese-take-photo.png
dist_actionsicon22_DATA = \
	data/icons/22x22/actions/browse-webcam-effects.png \
	data/icons/22x22/actions/cheese-take-burst.png \
	data/icons/22x22/actions/cheese-take-photo.png
dist_actionsicon24_DATA = \
	data/icons/24x24/actions/browse-webcam-effects.png \
	data/icons/24x24/actions/cheese-take-burst.png \
	data/icons/24x24/actions/cheese-take-photo.png
dist_actionsicon32_DATA = \
	data/icons/32x32/actions/browse-webcam-effects.png \
	data/icons/32x32/actions/cheese-take-burst.png \
	data/icons/32x32/actions/cheese-take-photo.png
dist_actionsicon48_DATA = \
	data/icons/48x48/actions/browse-webcam-effects.png \
	data/icons/48x48/actions/cheese-take-burst.png \
	data/icons/48x48/actions/cheese-take-photo.png
dist_actionsiconscalable_DATA = \
	data/icons/scalable/actions/browse-webcam-effects.svg \
	data/icons/scalable/actions/cheese-take-burst.svg \
	data/icons/scalable/actions/cheese-take-photo.svg

update_icon_cache = gtk-update-icon-cache --ignore-theme-index --force

install-data-hook: install-update-icon-cache
uninstall-hook: uninstall-update-icon-cache

install-update-icon-cache:
	$(AM_V_at)$(POST_INSTALL)
	test -n "$(DESTDIR)" || $(update_icon_cache) "$(iconthemedir)"

uninstall-update-icon-cache:
	$(AM_V_at)$(POST_UNINSTALL)
	test -n "$(DESTDIR)" || $(update_icon_cache) "$(iconthemedir)"

pixmapsdir = $(pkgdatadir)/pixmaps
dist_pixmaps_DATA = \
	data/pixmaps/thumbnail-frame.png \
	data/pixmaps/camera-icon.svg \
	data/pixmaps/cheese-1.svg \
	data/pixmaps/cheese-2.svg \
	data/pixmaps/cheese-3.svg

dist_noinst_DATA = \
	build-aux/gnome-doc-utils.make \
	ChangeLog.pre-git \
	$(desktop_in_files) \
	$(gsettings_in_file) \
	$(service_in_files) \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	$(noinst_libcheese_headers) \
	$(noinst_libcheese_gtk_headers) \
	libcheese/cheese-gtk.symbols \
	libcheese/cheese-marshal.list \
	m4/introspection.m4 \

dist_noinst_SCRIPTS = \
	autogen.sh

CLEANFILES = \
	$(desktop_DATA) \
	$(gsettings_SCHEMAS) \
	$(pkgconfig_DATA) \
	$(marshal_data) \
	$(enum_data) \
	$(gir_DATA) \
	$(typelib_DATA)

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update \
	po/.intltool-merge-cache

MAINTAINERCLEANFILES = \
	build-aux/compile \
	build-aux/config.guess \
	build-aux/config.sub \
	build-aux/depcomp \
	build-aux/gnome-doc-utils.make \
	build-aux/install-sh \
	build-aux/ltmain.sh \
	build-aux/missing \
	aclocal.m4 \
	config.h.n \
	$(gsettings_SCHEMAS:.xml=.valid) \
	mkinstalldirs \
	omf.make \
	xmldocs.make

DISTCHECK_CONFIGURE_FLAGS = --disable-schemas-install --disable-scrollkeeper --enable-gtk-doc --enable-introspection

CHANGELOG_START = 2.26.0

dist-hook:
	$(AM_V_at)if test -d "$(srcdir)/.git"; \
	then \
		echo Creating ChangeLog && \
		( cd "$(top_srcdir)" && \
		  echo '# Generated by Makefile. Do not edit.'; echo; \
		  $(top_srcdir)/missing --run git log $(CHANGELOG_START).. \
			--stat -M -C --name-status --no-color ) | \
			fmt --split-only > ChangeLog.tmp \
		&& mv -f ChangeLog.tmp $(top_distdir)/ChangeLog \
		|| ( rm -f ChangeLog.tmp ; \
		  echo Failed to generate ChangeLog >&2 ); \
	else \
		echo A git clone is required to generate a ChangeLog >&2; \
	fi
