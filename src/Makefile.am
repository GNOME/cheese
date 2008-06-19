INCLUDES = 						\
	-DBINDIR=\"$(bindir)\"			 	\
	-DDATADIR=\"$(datadir)\"			\
	-DPREFIX=\""$(prefix)"\" 			\
	-DSYSCONFDIR=\""$(sysconfdir)"\" 		\
	-DLIBDIR=\""$(libdir)"\" 			\
	-DPACKAGE_DATADIR=\""$(datadir)/cheese"\"	\
	-DPACKAGE_LOCALEDIR=\""$(datadir)/locale"\"	\
	-DAPPNAME_DATA_DIR=\"$(pkgdatadir)\"		\
	$(CHEESE_CFLAGS)

bin_PROGRAMS = cheese

cheese_SOURCES = \
	cheese.c \
	cheese-countdown.c \
	cheese-countdown.h \
	cheese-effect-chooser.c \
	cheese-effect-chooser.h \
	cheese-fileutil.c \
	cheese-fileutil.h \
	cheese-gconf.c \
	cheese-gconf.h \
	cheese-thumb-view.c \
	cheese-thumb-view.h \
	cheese-webcam.c \
	cheese-webcam.h \
	cheese-window.c \
	cheese-window.h \
	eog-thumbnail.c \
	eog-thumbnail.h \
	ephy-spinner.c \
	ephy-spinner.h \
	gst-audio-play.c \
	gst-audio-play.h \
	gedit-message-area.c \
	gedit-message-area.h \
	cheese-no-camera.c \
	cheese-no-camera.h \
	cheese-prefs-widget.h \
	cheese-prefs-widget.c \
	cheese-prefs-dialog-widgets.h \
	cheese-prefs-dialog-widgets.c \
	cheese-prefs-resolution-combo.h \
	cheese-prefs-resolution-combo.c \
	cheese-prefs-dialog.c \
	cheese-prefs-dialog.h \
	cheese-flash.h \
	cheese-flash.c \
	cheese-prefs-webcam-combo.c \
	cheese-prefs-webcam-combo.h
	
cheese_LDADD = \
	$(CHEESE_LIBS)

if WITH_HILDON
  INCLUDES += $(HILDON_CFLAGS) -DHILDON
  cheese_LDADD += $(HILDON_LIBS)
endif

