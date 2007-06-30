
package.subdirs = sbin bin tests make
include toc2.make

# We don't normally want to include toc's docs in non-toc trees:
ifeq (toc2,$(PACKAGE_NAME))
  subdirs: subdir-doc
  package.subdirs += doc
endif

all: subdirs
