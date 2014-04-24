ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

SUBDIRS = po help

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
	-DPACKAGE_LOCALEDIR=\""$(datadir)/locale"\" \
	-DGNOME_DESKTOP_USE_UNSTABLE_API=1 \
	-I$(top_builddir) \
	$(CHEESE_CFLAGS) \
	$(CHEESE_GTK_FLAGS) \
	$(CHEESE_WFLAGS) \
	$(WARN_CFLAGS)

AM_VALAFLAGS = \
	--target-glib=2.38 \
	--thread \
	--vapidir $(top_srcdir)/src/vapi \
	--gresources $(top_srcdir)/data/org.gnome.Cheese.gresource.xml \
	--pkg config \
	--pkg posix \
	--pkg gtk+-3.0 \
	--pkg gmodule-2.0 \
	--pkg clutter-1.0 \
	--pkg clutter-gtk-1.0 \
	--pkg gstreamer-1.0 \
	--pkg libcanberra-gtk \
	--pkg libcanberra \
	--pkg eogthumbnav \
	--pkg cheese-thumbview \
	--pkg cheese-common \
	--pkg gdk-x11-3.0 \
	--pkg gio-2.0

lib_LTLIBRARIES = libcheese.la libcheese-gtk.la

libcheese_gtk_la_CFLAGS = \
	$(CHEESE_CFLAGS) \
	$(CHEESE_GTK_CFLAGS) \
	$(CHEESE_WFLAGS)

libcheese_gtk_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-DG_LOG_DOMAIN=\"cheese-gtk\" \
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
	libcheese/cheese-avatar-widget.c \
	libcheese/totem-aspect-frame.c \
	libcheese/cheese-flash.c \
	libcheese/cheese-gtk.c \
	libcheese/cheese-widget.c \
	libcheese/um-crop-area.c

noinst_libcheese_gtk_headers = \
	libcheese/totem-aspect-frame.h \
	libcheese/cheese-avatar-chooser.h \
	libcheese/cheese-avatar-widget.h \
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
	-DG_LOG_DOMAIN=\"cheese\" \
	-I$(top_builddir)/libcheese \
	-I$(top_srcdir)/libcheese

libcheese_la_LIBADD = \
	$(CHEESE_LIBS)

libcheese_la_LDFLAGS = \
	-version-info $(LIBCHEESE_LT_VERSION) \
	$(AM_LDFLAGS)

libcheese_la_SOURCES = \
	libcheese/cheese-enums.c \
	libcheese/cheese-camera.c \
	libcheese/cheese-camera-device.c \
	libcheese/cheese-camera-device-monitor.c \
	libcheese/cheese-effect.c \
	libcheese/cheese-fileutil.c \
	libcheese/cheese.c

noinst_libcheese_headers = \
	libcheese/cheese-camera.h \
	libcheese/cheese-camera-device.h \
	libcheese/cheese-camera-device-monitor.h \
	libcheese/cheese-effect.h \
	libcheese/cheese-fileutil.h

# FIXME when we have a .pc file, and sonames
cheesedir = $(includedir)/cheese
cheese_HEADERS = \
	libcheese/cheese-avatar-widget.h \
	libcheese/cheese-avatar-chooser.h \
	libcheese/cheese-camera.h \
	libcheese/cheese-camera-device.h \
	libcheese/cheese-camera-device-monitor.h \
	libcheese/cheese-effect.h \
	libcheese/cheese-gtk.h \
	libcheese/cheese.h \
	libcheese/cheese-widget.h

if HAVE_INTROSPECTION
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_COMPILER_ARGS = --includedir=$(top_srcdir)
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(top_srcdir) --warn-all

INTROSPECTION_GIRS = Cheese-3.0.gir

Cheese-3.0.gir: libcheese.la
Cheese_3_0_gir_CFLAGS = \
	-I$(top_srcdir)/libcheese \
	-I$(top_builddir)/libcheese \
	$(CHEESE_CFLAGS)
Cheese_3_0_gir_FILES = \
	$(libcheese_la_SOURCES) \
	$(noinst_libcheese_headers) \
	libcheese/cheese-enums.h
Cheese_3_0_gir_INCLUDES = \
	GObject-2.0 \
	GLib-2.0 \
	Gst-1.0 \
	GstBase-1.0 \
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

noinst_resource_files = \
	data/cheese.css \
	data/cheese-viewport.json \
	data/cheese-appmenu.ui \
	data/cheese-main-window.ui \
	data/cheese-prefs.ui \
	data/headerbar.ui \
	data/pixmaps/cheese-1.svg \
	data/pixmaps/cheese-2.svg \
	data/pixmaps/cheese-3.svg \
	data/pixmaps/thumbnail-frame.png

src/cheese-resource.c src/cheese-resource.h: data/org.gnome.Cheese.gresource.xml Makefile $(noinst_resource_files)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/data --generate --c-name cheese $<

bin_PROGRAMS = cheese

nodist_cheese_SOURCES = \
	src/cheese-resource.c

# cheese-main.vala must go at the end of the Vala sources.
cheese_SOURCES = \
	src/cheese-application.vala \
	src/cheese-countdown.vala \
	src/cheese-effects-manager.vala \
	src/cheese-preferences.vala \
	src/cheese-window.vala \
	src/cheese-main.vala \
	src/thumbview/cheese-thumb-view.c \
	src/thumbview/cheese-thumbnail.c \
	src/thumbview/eog-thumb-nav.c

nodist_cheese_HEADERS = \
	src/cheese-resource.h

noinst_HEADERS = \
	src/thumbview/cheese-thumb-view.h \
	src/thumbview/cheese-thumbnail.h \
	src/thumbview/eog-thumb-nav.h

cheese_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	$(CHEESE_CFLAGS) \
	$(CHEESE_GTK_CFLAGS) \
	-include $(top_builddir)/$(CONFIG_HEADER) \
	-I$(top_srcdir)/libcheese \
	-I$(top_srcdir)/src

cheese_CFLAGS = -export-dynamic -w

cheese_LDADD = \
	libcheese.la \
	libcheese-gtk.la \
	$(CHEESE_LIBS) \
	$(CHEESE_GTK_LIBS)

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
	-I$(top_srcdir)/libcheese

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
desktop_in_files = data/org.gnome.Cheese.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

@APPDATA_XML_RULES@
@INTLTOOL_XML_RULE@
appdata_in_files = data/org.gnome.Cheese.appdata.xml.in
appdata_XML = $(appdata_in_files:.appdata.xml.in=.appdata.xml)

servicedir = $(datadir)/dbus-1/services
service_DATA = data/org.gnome.Cheese.service

@GSETTINGS_RULES@
gsettings_SCHEMAS = data/org.gnome.Cheese.gschema.xml

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

dist_appsicon16_DATA = data/icons/16x16/cheese.png
dist_appsicon22_DATA = data/icons/22x22/cheese.png
dist_appsicon24_DATA = data/icons/24x24/cheese.png
dist_appsicon32_DATA = data/icons/32x32/cheese.png
dist_appsicon48_DATA = data/icons/48x48/cheese.png
dist_appsicon256_DATA = data/icons/256x256/cheese.png

update_icon_cache = gtk-update-icon-cache --ignore-theme-index --force

install-data-hook: install-update-icon-cache
uninstall-hook: uninstall-update-icon-cache
	touch libcheese_la_vala.stamp

install-update-icon-cache:
	$(AM_V_at)$(POST_INSTALL)
	test -n "$(DESTDIR)" || $(update_icon_cache) "$(iconthemedir)"

uninstall-update-icon-cache:
	$(AM_V_at)$(POST_UNINSTALL)
	test -n "$(DESTDIR)" || $(update_icon_cache) "$(iconthemedir)"

# Tests.
check_PROGRAMS = \
	tests/test-libcheese \
	tests/test-libcheese-gtk

tests_test_libcheese_gtk_SOURCES = \
	libcheese/um-crop-area.c \
	tests/test-libcheese-gtk.c
tests_test_libcheese_gtk_CPPFLAGS = $(EXAMPLES_CPPFLAGS)
tests_test_libcheese_gtk_LDADD = \
	$(CHEESE_LIBS) \
	$(CHEESE_GTK_LIBS) \
	libcheese-gtk.la \
	libcheese.la

tests_test_libcheese_SOURCES = \
	tests/test-libcheese.c
tests_test_libcheese_CPPFLAGS = $(EXAMPLES_CPPFLAGS)
tests_test_libcheese_LDADD = \
	$(CHEESE_LIBS) \
	libcheese.la

if CHEESE_ENABLE_TESTS
# Not automake built-in TESTS_ENVIRONMENT!
TEST_ENVIRONMENT = MALLOC_CHECK_=2 MALLOC_PERTURB_=$$(($${RANDOM:-256} % 256))

# test: run all tests.
test: $(check_PROGRAMS)
	$(AM_V_at)$(TEST_ENVIRONMENT) $(GTESTER) --verbose $(check_PROGRAMS)

# test-report: run tests and generate report.
# perf-report: run tests with -m perf and generate report.
# full-report: like test-report: with -m perf and -m slow.
test-report perf-report full-report: $(check_PROGRAMS)
	$(AM_V_at)test -z "$(check_PROGRAMS)" || { \
	  case $@ in \
	  test-report) test_options="-k";; \
	  perf-report) test_options="-k -m=perf";; \
	  full-report) test_options="-k -m=perf -m=slow";; \
	  esac ; \
	  $(TEST_ENVIRONMENT) $(GTESTER) --verbose $$test_options -o test-log.xml $(check_PROGRAMS) ; \
	  if test -d "$(top_srcdir)/.git" ; then \
	    REVISION=`git describe` ; \
	  else \
	    REVISION=$(PACKAGE_VERSION) ; \
	  fi ; \
	  echo '<?xml version="1.0"?>' > $@.xml ; \
	  echo '<report-collection>' >> $@.xml ; \
	  echo '<info>' >> $@.xml ; \
	  echo '  <package>$(PACKAGE_NAME)</package>' >> $@.xml ; \
	  echo '  <version>$(PACKAGE_VERSION)</version>' >> $@.xml ; \
	  echo "  <revision>$$REVISION</revision>" >> $@.xml ; \
	  echo '</info>' >> $@.xml ; \
	  sed '1,1s/^<?xml\b[^>?]*?>//' < test-log.xml >> $@.xml ; \
	  rm test-log.xml ; \
	  echo >> $@.xml ; \
	  echo '</report-collection>' >> $@.xml ; \
	  $(GTESTER_REPORT) --version 2>/dev/null 1>&2 ; test "$$?" != 0 || $(GTESTER_REPORT) $@.xml >$@.html ; \
	}
else # !CHEESE_ENABLE_TESTS
test:
	echo "Test run disabled due to the lack of GLib testing utilities"
endif

# Code coverage reporting.
if CHEESE_ENABLE_LCOV
# Depend on full-report so that all the tests are run.
lcov: full-report
	$(AM_V_at)$(LCOV) --base-directory $(top_builddir) \
		--directory $(top_builddir)/libcheese --capture \
		--output-file cheese-lcov.info --test-name CHEESE \
		--no-checksum --compat-libtool
	$(AM_V_at)LANG=C $(LCOV_GENHTML) --prefix $(top_builddir) \
		--output-directory cheese-lcov --title "Cheese code coverage" \
		--legend --show-details cheese-lcov.info

lcov-clean:
	$(AM_V_at)$(LCOV) --directory $(top_builddir) -z
	$(AM_V_at)rm -rf cheese-lcov.info cheese-lcov
	$(AM_V_at)find . -name '*.gcda' -exec rm -f {} +
else # !CHEESE_ENABLE_LCOV
lcov:
	$(AM_V_at)echo "Code coverage reporting not available"

lcov-clean:
	$(AM_V_at)rm -rf cheese-lcov.info cheese-lcov
	$(AM_V_at)find . -name '*.gcda' -exec rm -f {} +
endif

# gtk-doc
gtkdoc_builddir = $(top_builddir)/docs/reference
gtkdoc_distdir = $(top_distdir)/docs/reference
gtkdoc_srcdir = $(top_srcdir)/docs/reference

all-local: $(bin_PROGRAMS)
	cd $(gtkdoc_builddir) && $(MAKE) $(AM_MAKEFLAGS) all
# run make test as part of make check.
check-local: test
	cd $(gtkdoc_builddir) && $(MAKE) $(AM_MAKEFlAGS) check
clean-local: lcov-clean
	cd $(gtkdoc_builddir) && if test -e Makefile ; then \
		$(MAKE) $(AM_MAKEFLAGS) clean ; \
	fi
	find . -name '*.gcno' -exec rm -f {} +
distclean-local:
	cd $(gtkdoc_builddir) && $(MAKE) $(AM_MAKEFLAGS) distclean
docs: $(lib_LTLIBRARIES)
	cd $(gtkdoc_builddir) && $(MAKE) $(AM_MAKEFLAGS) docs
install-data-local:
	cd $(gtkdoc_builddir) && $(MAKE) $(AM_MAKEFLAGS) install-data
uninstall-local:
	cd $(gtkdoc_builddir) && $(MAKE) $(AM_MAKEFLAGS) uninstall

dist-hook: git-changelog-hook
	$(MKDIR_P) $(gtkdoc_distdir)
	cp $(gtkdoc_srcdir)/Makefile.am $(gtkdoc_srcdir)/Makefile.in $(gtkdoc_distdir)
	cd $(gtkdoc_builddir) && $(MAKE) $(AM_MAKEFLAGS) \
	  distdir="../../$(gtkdoc_distdir)" \
	  top_distdir="../../$(top_distdir)" \
	  dist-hook

dist_noinst_DATA = \
	ChangeLog.pre-git \
	$(desktop_in_files) \
	$(gsettings_SCHEMAS) \
	$(noinst_resource_files) \
	$(appdata_in_files) \
	data/org.gnome.Cheese.gresource.xml \
	build-aux/test-driver \
	$(gtkdoc_srcdir)/cheese-docs.xml \
	$(gtkdoc_srcdir)/cheese-sections.txt \
	$(gtkdoc_srcdir)/cheese.xml \
	$(gtkdoc_srcdir)/version.xml.in \
	gtk-doc.make \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	$(noinst_libcheese_headers) \
	$(noinst_libcheese_gtk_headers) \
	libcheese/cheese-gtk.symbols \
	m4/introspection.m4 \
	src/vapi/cheese-common.deps \
	src/vapi/cheese-common.vapi \
	src/vapi/cheese-thumbview.vapi \
	src/vapi/config.vapi \
	src/vapi/eogthumbnav.vapi

dist_noinst_SCRIPTS = \
	autogen.sh

CLEANFILES = \
	$(appdata_XML) \
	$(desktop_DATA) \
	$(pkgconfig_DATA) \
	$(enum_data) \
	$(gir_DATA) \
	$(typelib_DATA) \
	src/cheese-resource.c \
	src/cheese-resource.h

DISTCLEANFILES = \
	libcheese_la_vala.stamp \
	intltool-extract \
	intltool-merge \
	intltool-update \
	po/.intltool-merge-cache

MAINTAINERCLEANFILES = \
	build-aux/compile \
	build-aux/config.guess \
	build-aux/config.sub \
	build-aux/depcomp \
	build-aux/install-sh \
	build-aux/ltmain.sh \
	build-aux/missing \
	build-aux/test-driver \
	aclocal.m4 \
	config.h.n \
	$(gsettings_SCHEMAS:.xml=.valid) \
	mkinstalldirs \
	omf.make \
	xmldocs.make

DISTCHECK_CONFIGURE_FLAGS = \
	--enable-appdata-check \
	--enable-gtk-doc \
	--enable-introspection \
	--enable-man

CHANGELOG_START = 2.26.0

git-changelog-hook:
	$(AM_V_at)if $(top_srcdir)/build-aux/missing --run git \
		--git-dir=$(top_srcdir)/.git --work-tree=$(top_srcdir) log \
		--stat -M -C --name-status --no-color $(CHANGELOG_START).. \
		| fmt --split-only >.ChangeLog.tmp; \
	then mv -f .ChangeLog.tmp "$(top_distdir)/ChangeLog"; \
	else rm -f .ChangeLog.tmp; exit 1; fi

.PHONY: docs git-changelog-hook test test-report perf-report full-report
.PHONY: lcov lcov-clean
