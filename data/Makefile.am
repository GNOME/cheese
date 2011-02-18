SUBDIRS = icons pixmaps

@INTLTOOL_DESKTOP_RULE@
desktopdir = $(datadir)/applications
desktop_in_files = cheese.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

gsettings_SCHEMAS = org.gnome.Cheese.gschema.xml
@GSETTINGS_RULES@

cssdir = $(pkgdatadir)
css_DATA = cheese.css

pkgdata_DATA = \
	cheese-main-window.ui	\
	cheese-about.ui \
	cheese-actions.ui \
	cheese-prefs.ui \
	cheese-viewport.json

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = \
	cheese.pc \
	cheese-gtk.pc

EXTRA_DIST = \
	$(desktop_in_files) \
	$(gsettings_SCHEMAS) \
	$(service_in_files) \
	$(pkgdata_DATA) \
        $(css_DATA)

clean-local :
	rm -f *~

DISTCLEANFILES = \
	cheese.desktop \
	org.gnome.Cheese.service

CLEANFILES = \
	cheese.pc \
	cheese-gtk.pc

