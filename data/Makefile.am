SUBDIRS = icons pixmaps

@INTLTOOL_DESKTOP_RULE@
desktopdir = $(datadir)/applications
desktop_in_files = cheese.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

gsettings_SCHEMAS = org.gnome.Cheese.gschema.xml
@GSETTINGS_RULES@

gtkrcdir = $(pkgdatadir)
gtkrc_DATA = gtkrc

# Dbus service file
servicedir = $(datadir)/dbus-1/services
service_in_files = org.gnome.Cheese.service.in
service_DATA = $(service_in_files:.service.in=.service)

# Rule to make the service file with bindir expanded
$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@bindir\@|$(bindir)|" $< > $@

pkgdata_DATA = \
	cheese-main-window.ui	\
	cheese-about.ui \
	cheese-actions.ui \
	cheese-prefs.ui \
	cheese-viewport.json

EXTRA_DIST = \
	$(desktop_in_files) \
	$(gsettings_SCHEMAS) \
	$(service_in_files) \
	$(pkgdata_DATA) \
	$(gtkrc_DATA)

clean-local :
	rm -f *~

DISTCLEANFILES = \
	cheese.desktop \
	org.gnome.Cheese.service
