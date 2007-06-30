#!/usr/bin/env make

include toc2.make

# mo files should go to:
#  $(prefix)/share/locale/$LANG/LC_MESSAGES/$pkgname.mo

LANGS := cs de es gl it pt

############################################################
# $(eval $(call install-mo-lang,LANG))
# $1 = lang
# 
# Maintenance reminder: we cannot use toc2's default
# INSTALL function because the source file and target file
# have different names. We CAN use the toc2 UNINSTALL code
# and we do so because it can remove empty directories
# for us.
define install-mo-lang
install: install-mo-$(1)
install-mo-$(1)-dest := $(prefix)/share/locale/$(1)/LC_MESSAGES
install-mo-$(1):
	@echo "Installing $(1).mo ..."
	@test -d $(DESTDIR)$$(install-mo-$(1)-dest) && exit 0; mkdir -p $(DESTDIR)$$(install-mo-$(1)-dest)
	@cp $(1).mo $(DESTDIR)$$(install-mo-$(1)-dest)/$(package.name).mo
uninstall: uninstall-mo-$(1)
uninstall-mo-$(1):
	@$$(call toc2.call.uninstall,$(package.name).mo,$$(install-mo-$(1)-dest))
endef

$(foreach LANG,$(LANGS),$(eval $(call install-mo-lang,$(LANG))))

POFILES = $(LANGS:%=%.po)
MOFILES = $(POFILES:%.po=%.mo)

package.dist_files += $(POFILES)
package.clean_files +=  $(MOFILES)

all: mo

mo: $(MOFILES)

%.mo: %.po
	@echo Building $@...
	@msgfmt --statistics -o $@ $<

SOURCES = $(wildcard $(toc2.top_srcdir)/src/*.c)
updatepo:
	xgettext -L C -k_ -o cheese.pot $(SOURCES)
	for po in $(POFILES); do \
		msgmerge -U $$po cheese.pot; \
	done
