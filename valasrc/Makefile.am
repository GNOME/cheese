VALA_CFLAGS = \
	--thread \
	--vapidir vapi \
	--pkg config \
	--pkg gtk+-2.0 

AM_CPPFLAGS = \
	-DBINDIR=\"$(bindir)\"			 	\
	-DDATADIR=\"$(datadir)\"			\
	-DPREFIX=\""$(prefix)"\" 			\
	-DSYSCONFDIR=\""$(sysconfdir)"\" 		\
	-DLIBDIR=\""$(libdir)"\" 			\
	-DPACKAGE_DATADIR=\""$(datadir)/cheese"\"	\
	-DPACKAGE_LOCALEDIR=\""$(datadir)/locale"\"	\
	$(CHEESE_CFLAGS) \
	-include $(CONFIG_HEADER)

BUILT_SOURCES = cheese.vala.stamp

bin_PROGRAMS = cheese

cheese_VALASOURCES = \
	cheese.vala

cheese_SOURCES = \
	$(cheese_VALASOURCES:.vala=.c) \
	$(cheese_VALASOURCES:.vala=.h) 

cheese.vala.stamp: $(cheese_VALASOURCES)
	$(VALAC) -C $(VALA_CFLAGS) $^
	touch $@

cheese_LDADD = \
	$(CHEESE_LIBS)
