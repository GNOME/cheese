#!/usr/bin/env make

include toc2.make
GLADEFILE = cheese.glade
DESKTOPFILE = $(wildcard *.desktop)

package.dist_files += $(GLADEFILE) $(EFFECTS) $(DESKTOPFILE)

package.install.package_data += $(GLADEFILE)

package.install.desktop = $(DESKTOPFILE)
$(call toc2.call.define-install-set,desktop, $(prefix)/share/applications,-m 0644)

package.subdirs := effects icons
subdirs: subdir-effects subdir-icons
all: subdirs
