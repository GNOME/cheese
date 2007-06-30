#!/do/not/make
# ^^^ let's help out emacs :/
########################################################################
# This file contains more-or-less generic call()able Make functions
# for use with the toc2 framework.
########################################################################

########################################################################
# $(call toc2.call.setx-unless-quiet,verbose string)
# An internal hack to enable "quiet" output. $(1) is a string which
# is shown ONLY if toc2.flags.quiet!=1
define toc2.call.setx-unless-quiet
if [[ x1 = "x$(toc2.flags.quiet)" ]]; then echo $(1); else set -x; fi
endef
# Reminder: using set -x is not the same as doing echo $(1) because
# set -x normalizes backslashed newline, extra spaces, and other
# oddities of formatting.
########################################################################

######################################################################
# toc2.call.atparse-file call()able:
# Similar to the toc2_atparse_file shell function of the same name,
# except for different arguments: Filters an input file using toc's
# at-parser (e.g., @TOKENS@), and sends output to $2.
# Args:
# $1 = input template
# $2 = output file
# $3 = list of properties, e.g. FOO=bar BAR=foo
toc2.bins.atparser = $(toc2.dirs.bin)/atsign_parse
$(toc2.bins.atparser):
ifeq (,$(wildcard $(toc2.bins.atparser)))
$(error Could not find toc2.bins.atparser at $(toc2.bins.atparser)!)
endif
toc2.call.atparse-file = \
        echo -n 'toc2.call.atparse-file: $(1) --> $(2)'; $(toc2.bins.atparser) $(3) < $(1) > $(2); echo


######################################################################
# toc2.call.generate-rules() callable:
#   $1 = rules name, e.g., INSTALL_XXX, BIN_PROGRAMS, etc.
#   $2 = arguments to pass to makerules.$(1)
# Sample:
# .toc2.foo.deps:
#     $(toc2.call.generate-rules,FOO,arg1=val1 -arg2)
# Executes script $(toc2.dirs.makefiles)/makerules.FOO, passing it $(2).
# That script is expected to generate Make code, which the caller can
# then include. This workaround was made for cases where $(eval) isn't
# desired. (Originally it was a partial substitute for a missing
# $(eval) when using Make < 3.80, but toc2 explicitly requires 3.80+.)
toc2.call.generate-rules = $(SHELL) $(toc2.dirs.makefiles)/makerules.$(1) $(2)
# the $(SHELL) prefix here is a kludge: systems where /bin/sh is Bourne
# fail to build, but the nature of the errors makes it quite difficult
# to track down where the failure is. The side-effect of this kludge
# is that makerules.FOO *must* be a shell script (as opposed to a Perl
# script or whatever). :/
########################################################################


########################################################################
# $(call toc2.call.remove-dupes,list)
# Returns a string equal to list with duplicate entries removed. It does
# not sort the list.
# Implementation code by Christoph Schulz
toc2.call.remove-dupes = $(if $1,$(strip $(word 1,$1) $(call $0,$(filter-out $(word 1,$1),$1))))


########################################################################
# $(call toc2.call.find-files,wildcard-pattern,path)
# Returns a string containing ALL files or dirs in the given path
# which match the wildcard pattern. The path may be colon- or
# space-delimited. Spaces in path elements or filenames is of
# course evil and will Cause Grief. If either $1 or $2 are empty
# then $(error) is called.
toc2.call.find-files = $(wildcard			\
                 $(addsuffix /$(if $1,$1,$(error toc2.call.find-files requires a wildcard argument for $$1)),		\
                     $(subst :, ,		\
                       $(subst ::,:.:,		\
                         $(patsubst :%,.:%,	\
                           $(patsubst %:,%:.,$(if $2,$2,$(error toc2.call.find-files ($$1=$1) requires a PATH as arg $$2))))))))
########################################################################
# $(call toc2.call.find-programs,wildcard[,path])
# Returns a list of ALL files/dirs matching the given wildcard in the
# given path (default=$(PATH)).
toc2.call.find-programs = $(call toc2.call.find-files,$1,$(if $2,$2,$(PATH)))
########################################################################
# $(call toc2.call.find-file,wildcard-pattern,path)
# Returns a string containing the FIRST file in the given path
# which matches the wildcard pattern. If the path is empty or not
# given then $(error) is called.
toc2.call.find-file = $(firstword $(call toc2.call.find-files,$1,$2))
########################################################################
# $(call toc2.call.find-program,wildcard[,path])
# Returns the FIRST file/dir matching the given wildcard in the given
# path (default=$(PATH)).
toc2.call.find-program = $(firstword $(call toc2.call.find-programs,$1,$2))

########################################################################
# $(call toc2.call.include,makefile-base-name)
# This is essentially the same as:
#   include $(toc2.dirs.makefiles)/makefile-base-name.make
# but this is arguably more maintainable.
toc2.call.include = $(eval include $(toc2.dirs.makefiles)/$(1).make)
toc2.call.-include = $(eval -include $(toc2.dirs.makefiles)/$(1).make)

########################################################################
# A simple help facility, taken from "Managing Projects with GNU Make,
# Third Edition":
.PHONY: list-targets
list-targets-noop:# don't ask
list-targets:
	@$(MAKE) --print-data-base --question list-targets-noop | \
      grep -v -e '^no-such-target' -e '^makefile' |      \
      awk '/^[^.%][-A-Za-z0-9_]*:/{ print substr($$1, 1, length($$1)-1) }' |    \
	sort |                                             \
      pr --omit-pagination --columns=2
