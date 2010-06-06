VALAFLAGS = \
	--thread \
	--vapidir vapi \
	--pkg config \
	--pkg gtk+-2.0 \
	--pkg gmodule-2.0 \
	--pkg clutter-1.0 \
	--pkg clutter-gtk-0.10 \
	--pkg gconf-2.0 \
	--pkg gstreamer-0.10 \
	--pkg eogthumbnav \
	--pkg cheese-thumbview \
	--pkg cheese-common


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
	-include $(CONFIG_HEADER) \
	-I@top_srcdir@/libcheese 

AM_CFLAGS = \
	-export-dynamic

bin_PROGRAMS = cheese

cheese_SOURCES = \
	cheese-main.vala \
	cheese-window.vala \
	thumbview/cheese-gconf.c \
	thumbview/cheese-fileutil.c \
	thumbview/cheese-thumb-view.c \
	thumbview/eog-thumb-nav.c \
	thumbview/eog-thumbnail.c

cheese_LDADD = \
	$(top_builddir)/libcheese/libcheesecommon.la \
	$(CHEESE_LIBS)
