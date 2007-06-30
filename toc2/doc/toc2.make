#!/do/not/make
########################################################################
# toc2.make.at: template makefile for toc2.make, a core concept of the
# toc2 build process. This is filtered at the end of the configure
# process. toc2.make must be included by your Makefiles, like so:
#
#  include toc2.make
#
# Each makefile which does that will get a toc2.make created during
# the configure process. Each toc2.make is specific to that sub-dir
# (more specifically, each subdir at the same depth gets the same
# toc2.make).
#
# All of the "at-vars" in this file are expected to come in
# via the configure process, either from core tests or from
# the core itself.
#
# Ideally this file should be free of project-specific code:
# put that in $(toc2.top_srcdir)/toc2.$(package.name).make.at and
# in then $(toc2.top_srcdir)/configure.$(package.name) run:
#   toc_test_require toc2_project_makefile
########################################################################
default: all
all:
FORCE: ; @true

########################################################################
# $(package.name) and $(package.version) are the penultimate
# variables, holding the package name and version. They must not
# contain spaces, and some characters may confuse other tools. Most
# notable, a '+' in the name is likely to break the makedist tool.
package.version = 0.1.2
package.name = cheese

########################################################################
# $(SHELL) must be BASH, but the path might be system-specific.
SHELL = /bin/bash
##### prefix and DESTDIR are for autotools 'make install' compatibility
prefix ?= /usr/local
DESTDIR ?=

ifneq (,$(COMSPEC))
$(warning Smells like Windows!)
toc2.flags.smells_like_windows := 1
toc2.platform.file_extensions.dll = .DLL# maintenance reminder: this must stay upper-case!
toc2.platform.file_extensions.lib = .a# yes, use .a for cygwin
toc2.platform.file_extensions.exe = .EXE# maintenance reminder: this must stay upper-case!
else
toc2.flags.smells_like_windows := 0
toc2.platform.file_extensions.dll = .so
toc2.platform.file_extensions.lib = .a
toc2.platform.file_extensions.exe =# no whitespace, please
endif

toc2.files.makefile := $(word 1,$(MAKEFILE_LIST))# accommodate Makefile/GNUmakefile
$(toc2.files.makefile):

########################################################################
# $(toc2.flags.quiet) is used by some toc2 code to enable/disable
# verbose output. Set it to 1 to enable it, or any other value to
# disable it.
toc2.flags.quiet ?= $(if @TOC2_BUILD_QUIETLY@,@TOC2_BUILD_QUIETLY@,0)

toc2.top_srcdir = ../..

##### include configure-created code:
toc2.makefile.config_vars = $(toc2.top_srcdir)/toc2.$(package.name).configure.make
$(toc2.makefile.config_vars):# created by configure process
include $(toc2.makefile.config_vars)


toc2.project_makefile = $(wildcard $(toc2.top_srcdir)/toc2.$(package.name).make)
toc2.project_makefile_at = $(wildcard $(toc2.top_srcdir)/toc2.$(package.name).make.at)

toc2.home ?= /home/dgsiegel/projects/cheese/toc2
# todo: check if this is under $(toc2.top_srcdir), so we can make this path relative.

toc2.dirs.makefiles = $(toc2.home)/make
toc2.dirs.bin = $(toc2.home)/bin
toc2.dirs.sbin = $(toc2.home)/sbin
toc2.dirs.relative_to_top = toc2/doc
# e.g., in lib/foo, toc2.dirs.relative_to_top == lib/foo

toc2.make = toc2.make
# deprecated TOP_toc2.make = $(toc2.top_srcdir)/$(toc2.make)

##### some core utilities:
toc2.bins.awk = /bin/gawk
toc2.bins.perl = /usr/bin/perl
toc2.bins.sed = /bin/sed
toc2.bins.tar = /bin/tar
toc2.bins.gzip = /bin/gzip
toc2.bins.bzip = /bin/bzip2
toc2.bins.zip = /usr/bin/zip
toc2.bins.ar = $(AR)
toc2.bins.installer = /home/dgsiegel/projects/cheese/toc2/bin/install-sh
toc2.bins.makedist = /home/dgsiegel/projects/cheese/toc2/bin/makedist


# The emoticons are now exported directly by toc2_core.sh to toc2.PACKAGE.configure.make:
# toc2.emoticons.okay=[1m:-)[m
# toc2.emoticons.warning=[1m:-O[m
# toc2.emoticons.error=[1m:-([m
# toc2.emoticons.grief=@TOC2_EMOTICON_GRIEF@
# toc2.emoticons.wtf=@TOC2_EMOTICON_WTF@

toc2.clean_files += $(wildcard .toc2.* core *~)
toc2.distclean_files += $(toc2.make)

ifeq (.,$(toc2.top_srcdir))
    toc2.distclean_files += \
		toc2.$(package.name).make \
		toc2.$(package.name).configure.make
endif


include $(toc2.dirs.makefiles)/toc2-functions-core.make
include $(toc2.dirs.makefiles)/toc2-subdirs.make
include $(toc2.dirs.makefiles)/toc2-cleanup.make
include $(toc2.dirs.makefiles)/toc2-install.make
include $(toc2.dirs.makefiles)/toc2-dist.make

########################################################################
# A kludge to get mkdep-toc2 deleted at a proper time... only if we're
# in the top-most dir and mkdep-toc2 exists...
# This code *should* be in toc2-c.make, but that file is not globally
# included.
toc2.bins.mkdep.c := $(wildcard $(toc2.home)/bin/mkdep-toc2.c)
toc2.bins.mkdep := $(if $(toc2.bins.mkdep.c),$(toc2.top_srcdir)/$(basename $(notdir $(toc2.bins.mkdep.c))),)
########################################################################
# Set toc2.bins.mkdep.flags to whatever -I flags you want to use for
# $(toc2.bins.mkdep). Any non-I flags are ignored by mkdep.
toc2.bins.mkdep.flags += $(INCLUDES) $(CPPFLAGS)
ifeq (.,$(toc2.top_srcdir))
  .PHONY: distclean-mkdep-toc2
  distclean-mkdep-toc2:
	@test "x." = "x$(toc2.top_srcdir)" -a \
		x != 'x$(toc2.bins.mkdep)' -a \
		-e "$(toc2.bins.mkdep)" || exit 0; \
		rm -f "$(toc2.bins.mkdep)"
  distclean-.: distclean-mkdep-toc2
endif

########################################################################
# finally, load the project-specific code:
ifneq (,$(toc2.project_makefile))
    include $(toc2.project_makefile)
endif

