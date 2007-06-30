#!/do/not/make
# ^^^ help out emacs, so it knows what edit mode i want.
#
# toc makefile snippet to link binaries
########################################################################
#
# Usage:
# define these vars:
#   c-bins.list = bin1 [bin2 ... binN]
#   c-bins.LDADD (optional - list of libs to link to all c-bins.list)
#   c-bins.OBJECTS (optional - list of .o files to link to all c-bins.list)
#
# For each FOO in c-bins, define:
#   FOO.bin.OBJECTS = list of .o files for FOO
#   FOO.bin.LDADD = optional arguments to pass to linker, e.g. -lstdc++
#
# Reminder: when linking binaries which will use dlopen() at some point, you
# should add -rdynamic to the xxx.bin.LDADD flags. Without this, symbols won't be
# visible by dlsym(), and some types of global/ns-scope objects won't get
# initialized.
#
# Then include this file:
#
#   include $(toc2.dirs.makefiles)/toc2-c-bins.make
#
# and add 'c-bins' somewhere in your dependencies, e.g.:
#
#   all: c-bins
#
# This file depends on functions defined in toc2-c.make.
########################################################################

c-bins.makefile = $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))

ifeq (,$(c-bins.list))
$(error c-bins.list must be defined before including this file (read this file for the full docs))
endif

ifeq (1,$(toc2.flags.smells_like_windows))
  package.clean_files += $(wildcard *.exe.stackdump)
endif

c-bins.DEPSFILE = .toc2.c-bins.make

c-bins.RULES_GENERATOR = $(toc2.dirs.makefiles)/makerules.c-bins

c-bins.COMMON_DEPS += $(toc.2.files.makefile) $(c-bins.makefile) $(c-bins.objects)
ifeq (1,$(toc2.flags.making_clean))
$(c-bins.DEPSFILE): ; @true
else
$(c-bins.DEPSFILE): $(toc.2.files.makefile) $(c-bins.RULES_GENERATOR) $(c-bins.makefile)
	@echo "Generating c-bins rules: $(c-bins.list)"; \
	$(call toc2.call.generate-rules,c-bins,$(c-bins.list)) > $@
endif
# REMINDER: we MUST include the file, even if doing clean, so that the
# bins cleanup rules actually get performed.
-include $(c-bins.DEPSFILE)
deps: $(c-bins.DEPSFILE)
bins:
c-bins: bins
