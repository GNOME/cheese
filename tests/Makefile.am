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

noinst_PROGRAMS = cheese-test-widget cheese-test-monitor

cheese_test_widget_SOURCES = cheese-test-widget.c
cheese_test_monitor_SOURCES = cheese-test-monitor.c

cheese_test_widget_LDADD = $(CHEESE_LIBS) \
	$(top_builddir)/libcheese/libcheesecommon.la \
	$(top_builddir)/libcheese/libcheese-gtk.la \
	$(NULL)
cheese_test_monitor_LDADD = $(CHEESE_LIBS) \
	$(top_builddir)/libcheese/libcheesecommon.la \
	$(top_builddir)/libcheese/libcheese-gtk.la \
	$(NULL)
