VALAFLAGS = \
	--thread \
	--vapidir vapi \
	--pkg config \
	--pkg gtk+-3.0 \
	--pkg gmodule-2.0 \
	--pkg gee-1.0 \
	--pkg clutter-1.0 \
        --pkg clutter-gtk-1.0 \
	--pkg gstreamer-0.10 \
        --pkg libcanberra-gtk \
        --pkg libcanberra \
	--pkg eogthumbnav \
	--pkg cheese-thumbview \
	--pkg cheese-common \
	--pkg clutter-tablelayout

AM_CPPFLAGS = \
	-DBINDIR=\"$(bindir)\"			 	\
	-DDATADIR=\"$(datadir)\"			\
	-DPREFIX=\""$(prefix)"\" 			\
	-DSYSCONFDIR=\""$(sysconfdir)"\" 		\
	-DLIBDIR=\""$(libdir)"\" 			\
	-DPACKAGE_DATADIR=\""$(datadir)/cheese"\"	\
	-DPACKAGE_LOCALEDIR=\""$(datadir)/locale"\"	\
	-DGNOME_DESKTOP_USE_UNSTABLE_API=1 \
	$(CHEESE_CFLAGS) \
	$(CHEESE_GTK_CFLAGS) \
	-include $(CONFIG_HEADER) \
	-I@top_srcdir@/libcheese 

AM_CFLAGS = \
	-export-dynamic 

bin_PROGRAMS = cheese

cheese_SOURCES = \
	cheese-main.vala \
	cheese-window.vala \
	cheese-countdown.vala \
	cheese-effects-manager.vala \
	cheese-preferences.vala \
	thumbview/cheese-thumb-view.c \
	thumbview/cheese-thumbnail.c \
	thumbview/eog-thumb-nav.c

noinst_HEADERS = \
	thumbview/cheese-thumb-view.h \
	thumbview/cheese-thumbnail.h \
	thumbview/eog-thumb-nav.h

cheese_LDADD = \
	$(top_builddir)/libcheese/libcheese.la \
	$(top_builddir)/libcheese/libcheese-gtk.la \
	$(CHEESE_LIBS) \
	$(CHEESE_GTK_LIBS)
