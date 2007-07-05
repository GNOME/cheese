#/usr/bin/env make -f

package.subdirs = src po data
toc2.dist.suffix_list := tar.gz
include toc2.make

package.dist_files += \
	AUTHORS \
	ChangeLog \
	configure \
	configure.$(package.name) \
	COPYING \
	README \
	TODO \
	INSTALL \
	toc2.$(package.name).make.at \
	toc2.$(package.name).help

ifneq (,$(filter distclean dist,$(MAKECMDGOALS)))
	package.subdirs += toc2
	#subdirs: subdir-toc2
endif

ifneq (,$(filter install,$(MAKECMDGOALS)))
  ifeq (1,$(CHEESE_MAINTAINER_MODE))
    $(error Your tree was configured for maintainer mode! Installing won\'t work)
  endif
endif

all: subdirs

ifeq (,$(DESTDIR))
postinstall-message:
	@echo "Installation complete."
	@echo -n "Updating icon cache... "
	@gtk-update-icon-cache -q -t -f $(prefix)/share/icons/hicolor
	@echo "done"
else
postinstall-message:
	@echo
	@echo "Installation complete."
	@echo
	@echo please execute either
	@echo gtk-update-icon-cache -q -t -f $(DESTDIR)$(prefix)/share/icons/hicolor
	@echo
	@echo or \(depending, where you plan to install $(package.name)\)
	@echo gtk-update-icon-cache -q -t -f $(prefix)/share/icons/hicolor
	@echo
	@echo in order to update the icon cache and activate the icons of $(package.name)
endif


install: postinstall-message

dist-postprocess:
	@$(toc2.bins.md5sum) $(package.dist.tarball_gz)
