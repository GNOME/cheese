INCLUDES = \
	-DBINDIR=\"$(bindir)\" \
	-DDATADIR=\"$(datadir)\" \
	-DPREFIX=\""$(prefix)"\" \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
	-DPACKAGE_DATADIR=\""$(datadir)/cheese"\" \
	-DPACKAGE_LOCALEDIR=\""$(datadir)/locale"\" \
	-DAPPNAME_DATA_DIR=\"$(pkgdatadir)\" \
	-DGNOME_DESKTOP_USE_UNSTABLE_API=1 \
	$(CHEESE_CFLAGS) \
	-I@top_srcdir@/libcheese \
	$(NULL)

bin_PROGRAMS = cheese

cheese_SOURCES = \
	cheese.c \
	cheese-window.c \
	cheese-window.h \
	cheese-ui.h \
	cheese-commands.h \
	cheese-commands.c \
	cheese-commands-help.c \
	cheese-commands-file.c \
	cheese-dbus.c \
	cheese-dbus.h \
	cheese-effect-chooser.c \
	cheese-effect-chooser.h \
	cheese-thumb-view.c \
	cheese-thumb-view.h \
	eog-thumbnail.c \
	eog-thumbnail.h \
	eog-thumb-nav.c \
	eog-thumb-nav.h \
	cheese-no-camera.c \
	cheese-no-camera.h \
	cheese-prefs-widget.h \
	cheese-prefs-widget.c \
	cheese-prefs-dialog-widgets.h \
	cheese-prefs-dialog-widgets.c \
	cheese-prefs-resolution-combo.h \
	cheese-prefs-resolution-combo.c \
	cheese-prefs-balance-scale.c \
	cheese-prefs-balance-scale.h \
	cheese-prefs-dialog.c \
	cheese-prefs-dialog.h \
	cheese-prefs-camera-combo.c \
	cheese-prefs-camera-combo.h \
	cheese-prefs-burst-spinbox.h \
	cheese-prefs-burst-spinbox.c

cheese_LDADD = \
	$(top_builddir)/libcheese/libcheesecommon.la \
	$(top_builddir)/libcheese/libcheese-gtk.la \
	$(CHEESE_LIBS)

#dbus
BUILT_SOURCES = cheese-dbus-infos.h
# We don't want to install this header
noinst_HEADERS = $(BUILT_SOURCES)

# Correctly clean the generated headers, but keep the xml description
CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = cheese-dbus-infos.xml

#Rule to generate the binding headers
cheese-dbus-infos.h:  cheese-dbus-infos.xml
	dbus-binding-tool --prefix=cheese_dbus --mode=glib-server $<> $@

