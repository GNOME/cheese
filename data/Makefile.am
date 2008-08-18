SUBDIRS = icons effects pixmaps sounds

@INTLTOOL_DESKTOP_RULE@
desktopdir = $(datadir)/applications
desktop_in_files = cheese.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_SCHEMAS_RULE@
schemadir = $(sysconfdir)/gconf/schemas
schema_in_files = cheese.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)

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
	cheese.ui	\
	cheese-ui.xml \
	cheese-prefs.ui

EXTRA_DIST = \
	$(desktop_in_files) \
	$(schema_in_files) \
	$(service_in_files) \
	$(pkgdata_DATA) \
	$(gtkrc_DATA) \
	$(bugreport_SCRIPTS)

bugreportdir      = $(libexecdir)/cheese
bugreport_SCRIPTS = cheese-bugreport.sh

if GCONF_SCHEMAS_INSTALL
install-data-local:
	-GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
	$(GCONFTOOL) --makefile-install-rule $(schema_DATA)
endif

if GCONF_SCHEMAS_INSTALL
uninstall-local:
	-GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
	$(GCONFTOOL) --makefile-uninstall-rule $(schema_DATA)
endif

clean-local :
	rm -f *~

DISTCLEANFILES = \
	cheese.desktop \
	cheese.schemas \
	org.gnome.Cheese.service
