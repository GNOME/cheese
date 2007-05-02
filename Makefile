CC = gcc

GTK_CFLAGS = $(shell pkg-config --cflags gtk+-2.0)
GTK_LIBS = $(shell pkg-config --libs gtk+-2.0)

GLADE_CFLAGS = $(shell pkg-config --cflags libglade-2.0)
GLADE_LIBS = $(shell pkg-config --libs libglade-2.0)

DBUS_CFLAGS = $(shell pkg-config dbus-1 --cflags)
DBUS_LIBS = $(shell pkg-config dbus-1 --libs)

GSTREAMER_CFLAGS = $(shell pkg-config --cflags gstreamer-0.10 gstreamer-plugins-base-0.10)
GSTREAMER_LIBS = $(shell pkg-config --libs gstreamer-0.10 gstreamer-plugins-base-0.10) -lgstinterfaces-0.10 

GNOME_VFS_CFLAGS = $(shell pkg-config --cflags gnome-vfs-2.0)
GNOME_VFS_LIBS = $(shell pkg-config --libs gnome-vfs-2.0)

GDK_CFLAGS = $(shell pkg-config --cflags gdk-2.0)
GDK_LIBS = $(shell pkg-config --libs gdk-2.0)

CFLAGS = -g -O2 -Wall $(DBUS_CFLAGS) $(GTK_CFLAGS) $(GLADE_CFLAGS) $(GSTREAMER_CFLAGS) $(GNOME_VFS_CFLAGS) $(GDK_CFLAGS)
LDFLAGS = -lpthread $(DBUS_LIBS) $(GTK_LIBS) $(GLADE_LIBS) $(GSTREAMER_LIBS) $(GNOME_VFS_LIBS) $(GDK_LIBS)

SOURCES = cheese.c
#POFILES = po/fr.po po/de.po po/es.po po/it.po

OBJS = $(SOURCES:%.c=%.o)
#MOFILES = $(POFILES:%.po=%.mo)

all: cheese
#all: cheese mo

#mo: $(MOFILES)

cheese: $(OBJS) $(LIBS)

cheese.o: cheese.c cheese.h

#%.mo: %.po
#	@echo Building $@...
#	@msgfmt --statistics -o $@ $<
#
#updatepo:
#	xgettext -L C -k_ -o po/cheese.pot $(SOURCES)
#	for po in $(POFILES); do \
#		msgmerge -U $$po po/cheese.pot; \
#	done
#
clean:
	rm -f cheese $(OBJS) $(MOFILES)
	rm -f *~ po/*~
