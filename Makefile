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

dist-postprocess:
	@$(toc2.bins.md5sum) $(package.dist.tarball_gz)
