VALAFLAGS = \
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

bin_PROGRAMS = cheese

cheese_SOURCES = \
	cheese.vala

cheese_LDADD = \
	$(CHEESE_LIBS)
