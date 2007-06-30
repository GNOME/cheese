#!/do/not/make
########################################################################
# a makefile snippet for use with toc2, to create .mo files from .po
# files and to install/uninstall .po files.



############################################################
# $(call toc2.call.setup-pomo-rules,LANG)
# $1 = language code. $(1).po must exist.
#
# Sets up rule $(1).mo: $(1).po which calls msgfmt
# to build $(1).mo and sets up un/install rules for
# $(1).mo, which require special handling compared to
# normal installs.
#
# Sample usage:
#  $(foreach LANG,de es gl it pt,\
#    $(call toc2.call.setup-pomo-rules,$(LANG)))
#
# You may set $(toc2.bins.msgfmt.flags) to pass
# flags to msgfmt (e.g. --statistics or --verbose). Not
# all flags will work here, e.g. -o and --qt will likely
# be incompatible with the generated logic.
############################################################
# Maintenance reminder: we cannot use toc2's default
# INSTALL function because the source file and target file
# have different names. We CAN use the toc2 UNINSTALL code
# and we do so because it can remove empty directories
# for us.
define toc2.eval.setup-pomo-rules
  ifeq (,$$(wildcard $(1).po))
    $$(error toc2.call.setup-pomo-rules: file $(1).po does not exist)
  endif
  ifeq (,$$(toc2.bins.msgfmt))
    toc2.bins.msgfmt ?= $$(call toc2.call.find-program,msgfmt)
  endif
  ifeq (,$$(toc2.bins.msgfmt))
    $$(error Could not find msgfmt in PATH. Cannot generate $(1).mo)
  endif
  install: install-pomo-$(1)
  install-pomo-$(1)-dest := $$(prefix)/share/locale/$(1)/LC_MESSAGES
  install-pomo-$(1)-destfile := $$(DESTDIR)$$(install-pomo-$(1)-dest)/$$(package.name).mo
  install-pomo-$(1):
	@echo "Installing $(1).mo ==> $$(install-pomo-$(1)-destfile)"
	@test -d $$(DESTDIR)$$(install-pomo-$(1)-dest) && exit 0; mkdir -p $$(DESTDIR)$$(install-pomo-$(1)-dest)
	@cp $(1).mo $$(install-pomo-$(1)-destfile)
  uninstall: uninstall-pomo-$(1)
  uninstall-pomo-$(1):
	@$$(call toc2.call.uninstall,$$(package.name).mo,$$(install-pomo-$(1)-dest))
  $(1).mo: $(1).po
	@$(call toc2.call.setx-unless-quiet,"msgfmt [$$@]"); \
	$$(toc2.bins.msgfmt) $$(toc2.bins.msgfmt.flags) -o $$@ $$<
  toc2.clean_files += $(1).mo
  package.mo_files += $(1).mo
endef
toc2.call.setup-pomo-rules = $(eval $(call toc2.eval.setup-pomo-rules,$(1)))
toc2.bins.msgfmt.flags ?=