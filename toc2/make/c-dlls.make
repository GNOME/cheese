#!/do/not/make
# Links objects into a dynamically-linked library using $(CXX)
# (only tested with g++).
#
# Usage: 
#
# c-dlls.list = foo
# c-dlls.OBJECTS = # optional list of .o files to link to all $(c-dlls.list)
# # optional: c-dlls.DO_INSTALL = 0# If you do NOT want INSTALL rules created for any
#                                      of the $(c-dlls.list). Default = 1.
# foo.DLL.OBJECTS = list of object files to link to foo.so
# foo.DLL.LDADD = # optional libraries passed to linker (e.g., -lstdc++)
# foo.DLL.VERSION = # optional Major.Minor.Patch # e.g. "1.0.1" it MUST match this format, but
#                    need not be numeric. Default is no version number.
# foo.DLL.INSTALL_DEST = # optional installation path. Defaults to $(package.install.dlls.dest)
# foo.DLL.DO_INSTALL = 0 # optional: suppresses generation of installation rules.
#                    i.e., foo.so will not be installed. Default = $(c-dlls.DO_INSTALL)
# foo.DLL.SONAME = foo.so.NUMBER # optional: explicitely defines the -soname tag
# include $(toc2.dirs.makefiles)/toc2-c-dlls.make
#
# If foo.DLL.VERSION is not set then no version number is used, and the
# conventional "versioned symlinks" are not created. This is normally
# used when linking plugins, as opposed to full-fledged libraries.
#
# Run:
#    all: c-dlls
#
# Effect:
# Creates foo.so, and optionally foo.so<version>, by linking
# $(foo.DLL.OBJECTS). It also sets up install/uninstall rules for
# handling the various version-number symlinks conventionally used
# to link mylib.so.1.2.3 to mylib.so.
########################################################################

ifeq ($(c-dlls.list),)
  $(error c-dlls.list must be defined before including this file (read this file for the full docs))
endif

c-dlls.makefile := $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))

########################################################################
# toc2-link-c-dll call()able function
# $1 = basename
# $2 = extra args for the linker
toc2-link-c-dll = { soname=$(1).so; wholename=$${soname}.$($(1).DLL.VERSION.MAJOR).$($(1).DLL.VERSION.MINOR).$($(1).DLL.VERSION.PATCH); \
			if test x = "x$($(1).DLL.SONAME)"; then stamp=$${soname}.$($(1).DLL.VERSION.MAJOR); \
			else stamp=$($(1).DLL.SONAME); fi; \
			test x = "x$($(1).DLL.VERSION.MAJOR)" && { wholename=$${soname}; stamp=$(1).so;}; \
			cmd="$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $${wholename} --export-dynamic -shared \
				-Wl,-soname=$${stamp} $($(1).DLL.OBJECTS) $(c-dlls.OBJECTS) $(c-dlls.LDADD) $($(1).DLL.LDADD) $(2)"; \
			if [[ x1 = x$(toc2.flags.quiet) ]]; then echo "Linking DLL [$(1)]..."; else echo $$cmd; fi; \
			$$cmd || exit; \
			test x = "x$($(1).DLL.VERSION.MAJOR)" || { \
				ln -fs $${wholename} $${soname}.$($(1).DLL.VERSION.MAJOR).$($(1).DLL.VERSION.MINOR); \
				ln -fs $${wholename} $${soname}.$($(1).DLL.VERSION.MAJOR); \
				ln -fs $${wholename} $${soname}; \
				}; \
			}
# symlinking methods:
# method 1:
#			ln -fs $${wholename} $${soname}.$($(1).DLL.VERSION.MAJOR).$($(1).DLL.VERSION.MINOR); \
#			ln -fs $${soname}.$($(1).DLL.VERSION.MAJOR).$($(1).DLL.VERSION.MINOR) $${soname}.$($(1).DLL.VERSION.MAJOR); \
#			ln -fs $${soname}.$($(1).DLL.VERSION.MAJOR) $${soname}; \
# method 1.5:
#			link1=$${soname}.$($(1).DLL.VERSION.MAJOR).$($(1).DLL.VERSION.MINOR); ln -fs $${wholename} $$link1; \
#			link2=$${soname}.$($(1).DLL.VERSION.MAJOR); ln -fs $$link1 $$link2; \
#			link3=$${soname}; ln -fs $$link2 $$link3; \
# method 2:
#			ln -fs $${wholename} $${soname}.$($(1).DLL.VERSION.MAJOR).$($(1).DLL.VERSION.MINOR); \
#			ln -fs $${wholename} $${soname}.$($(1).DLL.VERSION.MAJOR); \
#			ln -fs $${wholename} $${soname}; \


#			for l in $${soname} \
#				$${soname}.$($(1).DLL.VERSION.MAJOR) \
#				$${soname}.$($(1).DLL.VERSION.MAJOR).$($(1).DLL.VERSION.MINOR) \
#				; do \
#				ln -fs $${wholename} $$l; \
#			done; \

# c-dlls.SOFILES = $(patsubst %,%.so,$(c-dlls))

c-dlls.RULES_GENERATOR := $(toc2.dirs.makefiles)/makerules.c-dlls

c-dlls.DEPSFILE := .toc2.c-dlls.make
ifneq (1,$(toc2.flags.making_clean))
$(c-dlls.DEPSFILE): Makefile $(c-dlls.makefile) $(c-dlls.RULES_GENERATOR)
	@echo "Generating c-dlls rules: $(c-dlls.list)"; \
	$(call toc2.call.generate-rules,c-dlls,$(c-dlls.list)) > $@
else
$(c-dlls.DEPSFILE): ;@true
endif
# REMINDER: we MUST include the file, even if doing clean, so that the
# dll cleanup rules actually get performed.
-include $(c-dlls.DEPSFILE)
deps: $(c-dlls.DEPSFILE)

# package.clean_files += $(c-dlls.SOFILES) $(wildcard $(patsubst %,%.*,$(c-dlls.SOFILES)))
c-dlls: libs

