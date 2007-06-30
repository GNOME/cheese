#!/do/not/make
# a (big) snippet to build API docs using doxygen
#
# Edit the file Doxyfile template, Doxyfile.at, using
# @at-token@ conventions.
#
# Define:
#  doxygen.inputs = list of dirs to recurse into for doxygen
#  OPTIONAL: doxygen.predef = predfined C vars
#
# Sample:
#  INCLUDES_DIRS = $(addprefix $(top_includesdir)/,lib1 otherlib)
#  doxygen.predef = \
#	HAVE_CONFIG_H=1
#  doxygen.inputs = $(INCLUDES_DIRS)
#
# Of course your Doxyfile.at must have the appropriate @tokens@ in it,
# but the one shipped with this file is already set up for you.

ifeq (,$(toc2.bins.doxygen))
  toc2.bins.doxygen := $(call toc2.call.find-program,doxygen)
endif

ifeq (,$(toc2.bins.doxygen))
  $(error The variable toc2.bins.doxygen must be set before including this file.)
endif

doxygen.doxyfile = Doxyfile
doxygen.dirs.out ?= html
doxygen.doxyfile.at = Doxyfile.at
doxygen.index = Doxygen-index.txt
package.dist_files += $(doxygen.doxyfile.at) $(doxygen.index)

doxygen.install-dir.basename ?= doxygen-$(PACKAGE_NAME)-$(PACKAGE_VERSION)
docs: doxygen
install: install-doxygen

doxygen.bins.dot := $(call toc2.call.find-program,dot)
ifeq (,$(doxygen.bins.dot))
  doxygen.use_dot := 0
else
  doxygen.use_dot ?= 0# dot significantly slows down the build
endif

ifeq (1,$(doxygen.use_dot))
  doxygen.use_dot.text := YES
else
  doxygen.use_dot.text := NO
endif

doxygen.inputs ?= $(toc2.top_srcdir)
doxygen.flags.atparse ?= \
	DOXYGEN_INPUT="$(doxygen.index) $(doxygen.inputs)"
doxygen.flags.atparse += \
	top_srcdir="$(toc2.top_srcdir)" \
	PERL="$(toc2.bins.perl)" \
	PACKAGE_NAME="$(package.name)" \
	PACKAGE_VERSION="$(package.version)" \
	PREDEFINED="$(doxygen.predef)" \
	HTML_OUTPUT_DIR="$(doxygen.dirs.out)" \
	USE_DOT="$(doxygen.use_dot.text)"

doxygen.makefile := $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))
$(doxygen.makefile):
$(toc2.top_srcdir)/configure:
$(doxygen.doxyfile): $(doxygen.makefile) $(doxygen.doxyfile.at) $(toc2.files.makefile) $(toc2.top_srcdir)/configure
	@$(call toc2.call.atparse-file,$(doxygen.doxyfile.at),$@, \
		$(doxygen.flags.atparse) \
	)

toc2.doxygen.clean_files = $(doxygen.doxyfile) $(doxygen.dirs.out) latex
$(call toc2.call.define-cleanup-set,toc2.doxygen)

package.install.doxygen.dest = $(DESTDIR)$(package.install.docs.dest)

doxygen: $(doxygen.dirs.out)
$(doxygen.dirs.out): clean-set-toc2.doxygen $(doxygen.doxyfile)
	@echo "Running doxygen..."; \
	if [[ x0 != "x$(doxygen.use_dot)" ]]; then \
		echo "Warning: 'dot' is enabled, so this might take a while."; \
	fi
	@$(toc2.bins.doxygen) 
	@echo "Doxygen HTML output is in '$(doxygen.dirs.out)'."
doxygen.finaldest = $(package.install.doxygen.dest)/$(doxygen.install-dir.basename)
install-doxygen: doxygen
	@echo "Installing HTML docs to $(doxygen.finaldest)"
	@test -d $(package.install.doxygen.dest) || exit 0; rm -fr $(package.install.doxygen.dest)
	@mkdir -p $(package.install.doxygen.dest)
	@cp -r $(doxygen.dirs.out) $(doxygen.finaldest)

uninstall-doxygen:
	@echo "Uninstalling doxgen-generated docs: $(doxygen.finaldest)"
	@-test -d $(doxygen.finaldest) || exit 0; rm -fr $(doxygen.finaldest)

uninstall: uninstall-doxygen

# all: docs
