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
	toc2.$(package.name).make.at \
	TODO

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
	@echo -n "$(package.dist.tarball_gz) "
	@cat $(package.dist.tarball_gz).md5
