SUBDIRS = src po data help

EXTRA_DIST =			\
	$(cheesedoc_DATA)	\
	gnome-doc-utils.make	\
	intltool-extract.in	\
	intltool-merge.in	\
	intltool-update.in

DISTCLEANFILES =		\
	gnome-doc-utils.make	\
	intltool-extract	\
	intltool-merge		\
	intltool-update

DISTCHECK_CONFIGURE_FLAGS = --disable-schemas-install --disable-scrollkeeper

