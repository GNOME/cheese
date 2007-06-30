#!/do/not/make
########################################################################
# This file contains rules for the 'make dist' features of the toc2
# framework.
#
# Maintenance notes:
#
# - Make dist is intended to be run only from the $(toc2.top_srcdir)
#   directory. Starting the dist process from sub-dir will have
#   undefined results.
#
# - The order that the dest: prerequisites are declared is very
#   significant. Don't change these without a good reason.
#
# - The toc2-dist-print-package-dist-files stuff requires a sub-make
#   so that we can properly expand all var refs and shell-style
#   wildcards in $(package.dist_files).
########################################################################

package.dist_files += $(firstword $(MAKEFILE_LIST))

ifneq (dist,$(MAKECMDGOALS))
    toc2.flags.making_dist = 1
else
    toc2.flags.making_dist = 0
endif

toc2.dist.file_list = $(toc2.top_srcdir)/.toc2.package.dist_files
ifeq (.,$(toc2.top_srcdir))
  toc2.clean_files += $(wildcard $(toc2.dist.file_list))
endif

ifeq (,$(wildcard $(toc2.bins.makedist)))
  $(error toc2.bins.makedist must be set to the path of the toc2 makedist app (normally $(toc2.home)/bin/makedist))
endif

ifeq (.,$(toc2.top_srcdir))
  toc2.dist.pre-cleanup: ; @-rm -f $(toc2.dist.file_list)
  dist: toc2.dist.pre-cleanup
  # .INTERMEDIATE and .DELETE_ON_ERROR don't seem to be doing what
  # i think they should???
  .INTERMEDIATE: $(toc2.dist.file_list)
  .DELETE_ON_ERROR: $(toc2.dist.file_list)
endif


########################################################################
# internal target for parsing out package.dist_files entries...
toc2-dist-print-package-dist-files:
	@echo $(sort $(package.dist_files))

dist-.:
	@$(MAKE) --no-print-directory toc2-dist-print-package-dist-files \
	| perl -pe 's|(\S+) ?|./$(if $(toc2.dirs.relative_to_top),$(toc2.dirs.relative_to_top)/,)$$1\n|g' \
	| sed -e '/^$$/d' \
	>> $(toc2.dist.file_list)


$(toc2.dist.file_list): FORCE $(toc2.files.makefile)
	test x = "x$(package.dist_files)" && exit 0; \
		pwd=$$(pwd); \
		echo "Adding package.dist_files to [$@]..."; \
		for f in "" $(package.dist_files); do test x = "x$$f" && continue; \
			echo $$pwd/$$f; \
		done >> $@

dist-postprocess:# implement this in client-side code if you want to do something
dist: subdirs-dist dist-.

ifeq (.,$(toc2.top_srcdir))
  ############################################################
  # toc2.dist.suffix_list defines a list of distribution
  # archives to create. Valid values are:
  # tar.gz tar.bz2 zip
  # If used, it must be set BEFORE INCLUDING toc2.make.
  toc2.dist.suffix_list ?= tar.gz tar.bz2 zip
  ifeq (,$(toc2.bins.tar))
    # todo: we don't NEED tar if we're building a zip archive.
    $(error This makefile needs GNU tar in order to build a distribution archive!)
    # toc2.dist.suffix_list := $(filter-out tar tar.gz tar.bz2,$(toc2.dist.suffix_list))
  endif
  ifeq (,$(toc2.bins.gzip))
    toc2.dist.suffix_list := $(filter-out tar.gz,$(toc2.dist.suffix_list))
  endif
  ifeq (,$(toc2.bins.bzip))
    toc2.dist.suffix_list := $(filter-out tar.bz2,$(toc2.dist.suffix_list))
  endif
  ifeq (,$(toc2.bins.zip))
    toc2.dist.suffix_list := $(filter-out zip,$(toc2.dist.suffix_list))
  endif

  ifeq (,$(strip $(toc2.dist.suffix_list)))
    $(warning The variable $$(toc2.dist.suffix_list) is empty, so no dist archives will be created!)
  endif

  ############################################################
  # create $(package.dist_name).tar
  #ifneq (,$(filter tar,$(toc2.dist.suffix_list)))
    toc2.bins.md5sum := $(call toc2.call.find-program,md5sum)
    package.dist_name ?= $(package.name)-$(package.version)
    package.dist.tarball ?= $(package.dist_name).tar
    toc2.clean_files += $(package.dist.tarball)
    $(package.dist.tarball): FORCE
	@echo -n "Making dist tarball [$(package.dist.tarball)] ... "; \
		$(toc2.bins.makedist) $(toc2.dist.file_list) $(package.dist_name)
	@echo "$(toc2.emoticons.okay)"
    dist: $(package.dist.tarball)
  #endif

  ############################################################
  # create $(package.dist_name).tar.gz
  ifneq (,$(filter tar.gz,$(toc2.dist.suffix_list)))
    ifneq (,$(toc2.bins.gzip))
      package.dist.tarball_gz := $(package.dist.tarball).gz
      toc2.clean_files += $(package.dist.tarball_gz)
      $(package.dist.tarball_gz): $(package.dist.tarball)
	@echo -n "Creating $@... "; \
		$(toc2.bins.gzip) -c $< > $@ || exit $$?
	@echo "$(toc2.emoticons.okay)"
      dist: $(package.dist.tarball_gz)
    endif # gzip
  endif

  ############################################################
  # create $(package.dist_name).tar.bz2
  ifneq (,$(filter tar.bz2,$(toc2.dist.suffix_list)))
    ifneq (,$(toc2.bins.bzip))
      package.dist.tarball_bz := $(package.dist.tarball).bz2
      toc2.clean_files += $(package.dist.tarball_bz)
      $(package.dist.tarball_bz): $(package.dist.tarball)
	@echo -n "Creating $@... "; \
		$(toc2.bins.bzip) -c $< > $@ || exit $$?
	@echo "$(toc2.emoticons.okay)"
      dist: $(package.dist.tarball_bz)
    endif # bzip
  endif

  ############################################################
  # create $(package.dist_name).zip
  ifneq (,$(filter zip,$(toc2.dist.suffix_list)))
    ifneq (,$(toc2.bins.zip))
      package.dist.tarball_zip := $(package.dist_name).zip
      toc2.clean_files += $(package.dist.tarball_zip)
      $(package.dist.tarball_zip): $(package.dist.tarball)
	@echo -n "Creating $@... "; \
		test -e $@ && rm -f $@; \
		test -d $(package.dist_name) && rm -fr $(package.dist_name); \
		tar xf $(package.dist.tarball); \
		$(toc2.bins.zip) -qr $@ $(package.dist_name) || exit $$?;
	@rm -fr $(package.dist_name)
	@echo "$(toc2.emoticons.okay)"
      dist: $(package.dist.tarball_zip)
    endif # zip
  endif

  toc2.dist.post-cleanup:
	@-rm -f $(toc2.dist.file_list) $(package.dist.tarball) >/dev/null 2>&1
  toc2.dist.show-dist-tars:
	@ls -la $(package.dist_name).*
  dist: toc2.dist.post-cleanup toc2.dist.show-dist-tars
  dist: dist-postprocess
endif # . == $(toc2.top_srcdir)
