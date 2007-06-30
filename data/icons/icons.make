#!/do/not/make
# To be included by subdir makefiles
#
# Must set:

ICON_GEOMETRY := $(notdir $(shell echo $$PWD))
$(if $(ICON_GEOMETRY),,$(error You must set ICON_GEOMETRY before including this file!))

PNG = $(wildcard $(package.name).png)
ifneq (,$(PNG))
  $(call toc2.call.define-install-set,icons-png-$(ICON_GEOMETRY),$(prefix)/share/icons/hicolor/$(ICON_GEOMETRY)/apps)
  package.install.icons-png-$(ICON_GEOMETRY) = $(PNG)
  package.dist_files += $(PNG)
endif

SVG = $(wildcard $(package.name).svg)
ifneq (,$(SVG))
  package.dist_files += $(SVG)
  ifeq (,$(PNG))
    ########################################################################
    # Only install SVG when there is no PNG.
    $(call toc2.call.define-install-set,icons-svg-scalable,$(prefix)/share/icons/hicolor/scalable/apps)
    package.install.icons-svg-scalable = $(SVG)
  endif
endif
