#!/usr/bin/env make
include toc2.make

LANGS := cs de es fr gl it nl pl pt
include $(toc2.home)/make/pomo.make
$(foreach LANG,$(LANGS),\
	$(call toc2.call.setup-pomo-rules,$(LANG)))

package.dist_files += $(POFILES) $(package.name).pot
toc2.bins.msgfmt.flags := --statistics --verbose
POFILES = $(LANGS:%=%.po)
MOFILES = $(POFILES:%.po=%.mo)


all: mo

mo: $(MOFILES)

SOURCES = $(wildcard $(toc2.top_srcdir)/src/*.c)
updatepo:
	@#xgettext -L C -k_ -o $(package.name).pot $(SOURCES)
	xgettext -k_ -klabel -ktitle -o $(package.name).pot \
					$(SOURCES) \
					$(wildcard $(toc2.top_srcdir)/data/*.glade)
	for po in $(POFILES); do \
		msgmerge -U $$po $(package.name).pot; \
	done

