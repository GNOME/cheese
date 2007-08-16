#!/usr/bin/env make

include toc2.make
include $(toc2.dirs.makefiles)/toc2-c.make

SOURCES := cheese.c \
						cheese-cairo-custom.c \
						cheese-effects-widget.c \
						cheese-fileutil.c \
						cheese-pipeline-photo.c \
						cheese-pipeline-video.c \
						cheese-pipeline.c \
						cheese-thumbnails.c \
						cheese-window.c \
						eog-thumb-shadow.c

print-var-%:
	@echo $*=$($*)
	echo value $*=$(value $*)

GTK_CFLAGS := $(GTK_2_0_CFLAGS)
GTK_LIBS := $(GTK_2_0_LIBS)

GLADE_CFLAGS := $(LIBGLADE_2_0_CFLAGS)
GLADE_LIBS := $(LIBGLADE_2_0_LIBS)

DBUS_CFLAGS := $(DBUS_1_CFLAGS)
DBUS_LIBS := $(DBUS_1_CFLAGS)

GSTREAMER_CFLAGS := $(GSTREAMER_0_10_CFLAGS) $(GSTREAMER_PLUGINS_BASE_0_10_CFLAGS)
GSTREAMER_LIBS := $(GSTREAMER_0_10_LIBS) $(GSTREAMER_PLUGINS_BASE_0_10_LIBS) -lgstinterfaces-0.10 

GNOME_VFS_CFLAGS := $(GNOME_VFS_2_0_CFLAGS)
GNOME_VFS_LIBS := $(GNOME_VFS_2_0_LIBS)

GDK_CFLAGS := $(GDK_2_0_CFLAGS)
GDK_LIBS := $(GDK_2_0_LIBS)

LIBGNOMEUI_CFLAGS := $(LIBGNOMEUI_2_0_CFLAGS)
LIBGNOMEUI_LIBS := $(LIBGNOMEUI_2_0_LIBS)

CFLAGS := $(call toc2.call.remove-dupes,-g -O2 -Wall -Werror $(DBUS_CFLAGS) $(GTK_CFLAGS) $(GLADE_CFLAGS) $(GSTREAMER_CFLAGS) $(GNOME_VFS_CFLAGS) $(GDK_CFLAGS) $(LIBGNOMEUI_CFLAGS))
LDFLAGS := $(call toc2.call.remove-dupes,-lpthread $(DBUS_LIBS) $(GTK_LIBS) $(GLADE_LIBS) $(GSTREAMER_LIBS) $(GNOME_VFS_LIBS) $(GDK_LIBS) $(LIBGNOMEUI_LIBS))

OBJS = $(SOURCES:%.c=%.o)

package.dist_files += $(SOURCES) $(SOURCES:%.c=%.h) cheese-config.h.at
#package.clean_files += cheese $(OBJS)
package.distclean_files += cheese-config.h
package.install.bins := cheese

########################################################################
# Build binaries:
cheese.BIN.OBJECTS = $(OBJS)
$(call toc2.call.rules.c-bin,cheese)
########################################################################

all: cheese.BIN

