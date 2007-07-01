#!/usr/bin/env make
include toc2.make

LANGS := cs de es gl it pt
include $(toc2.home)/make/pomo.make
$(foreach LANG,$(LANGS),\
	$(call toc2.call.setup-pomo-rules,$(LANG)))

package.dist_files += $(POFILES) $(package.name).pot
toc2.bins.msgfmt.flags := --statistics --verbose
POFILES = cs.po de.po es.po gl.po it.po pt.po
MOFILES = $(POFILES:%.po=%.mo)


all: mo

mo: $(MOFILES)

SOURCES = $(wildcard $(toc2.top_srcdir)/src/*.c)
updatepo:
	xgettext -L C -k_ -o cheese.pot $(SOURCES)
	for po in $(POFILES); do \
		msgmerge -U $$po cheese.pot; \
	done

