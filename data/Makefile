#!/usr/bin/env make

include toc2.make
GLADEFILE = cheese.glade
DESKTOPFILE = $(wildcard *.desktop)
UIFILE = $(wildcard *.xml)

package.dist_files += $(GLADEFILE) $(EFFECTS) $(DESKTOPFILE) $(UIFILE)

package.install.package_data += $(GLADEFILE)
package.install.package_data += $(UIFILE)

package.install.desktop = $(DESKTOPFILE)
$(call toc2.call.define-install-set,desktop, $(prefix)/share/applications,-m 0644)

package.subdirs := effects icons
subdirs: subdir-effects subdir-icons
all: subdirs
