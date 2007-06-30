#!/do/not/make
########################################################################
# cleanup-related rules for the toc2 framework.

toc2.clean_files ?=
toc2.distclean_files ?=
package.clean_files ?=
package.distclean_files ?=

ifneq (,$(strip $(filter distclean clean,$(MAKECMDGOALS))))
    toc2.flags.making_clean = 1
else
    toc2.flags.making_clean = 0
endif

######################################################################
# $(call toc2.call.clean-files,file_list)
#
# Deletes all files/dirs passed in via $(1). BE CAREFUL, as all
# entries are UNCEREMONIOUSLY DELETED!!!
toc2.call.clean-files = \
	test "x$(strip $(1))" = "x"  && exit 0; \
	if test x1 != 'x$(toc2.flags.quiet)'; then \
		deep=0; while [[ $$deep -lt $(MAKELEVEL) ]]; do echo -n "  "; deep=$$((deep+1)); done; \
		echo "Cleaning up: [$(1)]"; \
	fi; \
	for x in $(1) ""; do test -z "$${x}" && continue; \
		test -w $$x || continue; \
		rmargs="-f"; test -d $$x && rmargs="-fr"; \
		rm $$rmargs $$x || exit; \
	done; \
	exit 0

########################################################################
# $(eval $(call toc2.call.cleanup-template,clean|distclean,file_list))
# For toc2 internal use only.
#  $(1) should be one of 'clean' or 'distclean'.
#  $(2) is a file/dir list. BE CAREFUL: all entries will be
#       UNCEREMONIOUSLY DELETED when make $(1) is called!
define toc2.call.cleanup-template
.PHONY: $(1) $(1)-.
.PHONY: $(1)-subdirs
$(1)-subdirs:
	@$$(call toc2.call.make-subdirs,$$(package.subdirs),$(1))
$(1)-.:
$(1): $(1)-subdirs $(1)-.
# /$(1)
########################################################################
endef
# Create clean:
$(eval $(call toc2.call.cleanup-template,clean))
# Create distclean:
$(eval $(call toc2.call.cleanup-template,distclean))

########################################################################
# $(call toc2.call.define-cleanup-set,SET_NAME)
# Adds target clean-set-SET_NAME as a prerequisite of clean-. and
# creates that target to clean up files defined in
# SET_NAME.clean_files. It does the same for distclean.
# This can be used to get around command-line length limitations when
# cleaning up large numbers of files by defining new cleanup sets.
define toc2.eval.define-cleanup-set
  clean-set-$(1):
	@$$(call toc2.call.clean-files,$$(wildcard $$($(1).clean_files)))
  clean-.: clean-set-$(1)
  distclean-set-$(1):
	@$$(call toc2.call.clean-files,$$(wildcard $$($(1).distclean_files)))
  distclean-.: clean-set-$(1) distclean-set-$(1)
endef
toc2.call.define-cleanup-set = $(eval $(call toc2.eval.define-cleanup-set,$(1)))

$(call toc2.call.define-cleanup-set,toc2)
$(call toc2.call.define-cleanup-set,package)
