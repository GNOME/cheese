#!/do/not/make
# A toc snippet to provide generic file filtering support. (Originally
# written to filter a namespace name in some C++ code.)
# Depends on the script makerules.file-filters, plus some toc-related
# conventions.
#
# stephan@s11n.net, 1 Dec 2003
#
# Usage:
#
# Define:
#  file-filters = filterX filterY
#      These are logical names for filter sets, and should be unique with a Makefile,
#      especially, there should be no targets with the names in $(file-filters).
#
#  either:
#      file-filters.bin = /path/to/filter/app + args
#  or:
#      filterX.filter.bin = /path/to/filter/app + args (defaults to 'sed')
#
#  filterX.filter.rules = -e 's|rules for your filter|...|' (FILTER_BIN must accept this
#          as a command-line argument)
#
#  filterX.filter.sources = list of input files, all of which must have some common prefix.
#
#  filterX.filter.renameexpr = sed code to translate x.filter.sources to target filenames. Gets
#          applied to each name in x.filter.sources. If this uses the $ pattern you must
#          use $$ instead, e.g. s/$$/.out/. BE CAREFUL!
#
#  filterX_FILTER_DEPS = optional list of extra deps. Automatically added are 
#          all input/control files used in creating the rules, including Makefile.
#
#
#  Generated files:
#  - are named .toc.file-filters.*
#  - are added to $(CLEAN_FILES)
#  - are not changed unless out-of-date, so they are dependencies-safe.
#  - get their own target names. This may cause collisions with other targets,
#      but presumably only one target is responsible for creating any given
#      file.
#
#
#########################################################################################
# BUG WARNING:  BUG WARNING:  BUG WARNING:  BUG WARNING:  BUG WARNING:  BUG WARNING:
#
# It works by creating intermediary makefiles (.toc.file-filters.*), so:
#
# When you remove/rename entries from file-filters (as your project changes) you
# will need to 'rm .toc.file-filters.*' in order to be able to run make again, as some
# pre-generated rules may become invalidated and generate now-bogus errors which will
# kill make before it can run any targets (e.g., clean).
#
#########################################################################################
# Sample usage:
#  file-filters = namespace filter2
#  # optional: namespace.filter.bin = $(toc2.bins.perl) -p
#  namespace.filter.rules = -e 's|PACKAGE_NAMESPACE|$(PACKAGE_NAMESPACE)|g'
#  namespace.filter.sources = $(wildcard src/*.cpp src/*.h)
#  namespace.filter.renameexpr = s,src/,,
#  ... similar for filter2.filter.xxx
#  include $(toc2.dirs.makefiles)/file-filters.make
#
#  That will filter src/*.{cpp,h} to ./*.{cpp,h}, replacing PACKAGE_NAMESPACE
#  with $(PACKAGE_NAMESPACE)
#
# To process it, either:
#    all: file-filters mytarget othertarget
#  or:
#    all: filter-namespace mytarget filter-filter2 othertarget
#  or:
#    all: mytarget othertarget
#
#########################################################################################

file-filters.makefile := $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))
$(file-filters.makefile):

ifeq (,$(file-filters.list))
$(error $(file-filters.makefile): you must define file-filters.list, plus some other vars, before including this file! Read the docs in this file)
endif

file-filters.bin ?= $(toc2.bins.sed)

file-filters.RULES_GENERATOR = $(dir $(file-filters.makefile))/makerules.file-filters
$(file-filters.RULES_GENERATOR):

file-filters.incfile := .toc2.file-filters.make
# toc2.clean_files += $(file-filters.incfile)
$(file-filters.incfile): $(toc2.files.makefile) $(file-filters.RULES_GENERATOR) $(file-filters.makefile)
ifeq (1,$(toc2.flags.making_clean))
	@echo "$(MAKECMDGOALS): skipping file-filters rules generation."
else
	@echo "Generating file-filters rules."; \
	$(call toc2.call.generate-rules,file-filters,$(file-filters.list)) > $@
endif
-include $(file-filters.incfile)

file-filters:
