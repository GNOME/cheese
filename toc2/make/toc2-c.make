#!/do/not/make
########################################################################
# This file contains C/C++-related functions for the toc2 framework.

toc2.makefile.c := $(word $(words MAKEFILE_LIST),$(MAKEFILE_LIST))
$(toc2.makefile.c):

toc2.clean_files += *.o

########################################################################
# If $(toc2.bins.mkdep.c) exists then assume we want to use it to create
# deps info for C/C++ stuff...
ifneq (,$(toc2.bins.mkdep.c))
  toc2.bins.mkdep := $(toc2.top_srcdir)/$(basename $(notdir $(toc2.bins.mkdep.c)))
  $(toc2.bins.mkdep.c):
  $(toc2.bins.mkdep): $(toc2.bins.mkdep.c)
	$(CC) -o $@ $<
else
  toc2.bins.mkdep :=# no C/C++ deps support
endif



########################################################################
# $(call toc2.bins.mkdep,input_files,dest_file) resolves to 'true'
# during the 'clean' target else it resolves to a command for
# generating deps file dest_file from input_files.
ifneq (,$(toc2.bins.mkdep))
  define toc2.bins.mkdep.rule-c
    $(if $(filter clean,$(MAKECMDGOALS)),true,\
		$(toc2.bins.mkdep) $(toc2.bins.mkdep.flags) -- $(2) > $(1); \
                echo '$(1): $(2)' >> $(1); echo '$(1):' >> $(1))
  endef
else
  toc2.bins.mkdep.rule-c := true
endif


########################################################################
# $(call toc2.call.compile-o-c,dest_obj,source(s))
# Compiles one or more C files to a .o file.
#   $1 = destination .o file
#   $2 = source c file(s)
# If you want to pass object-specific flags to this compile then create
# an empty target like this:
#    my.o: CFLAGS+=...
# Unfortunately, make does not allow you to pass more than one
# target-specific variable.
# Alternately, define $(1).CFLAGS = ...
define toc2.call.compile-o-c
  @$(call toc2.bins.mkdep.rule-c,$(1).d,$(2))
  @$(call toc2.call.setx-unless-quiet,"COMPILE C [$(2)] ..."); \
    $(COMPILE.c) $(INCLUDES) -o $(1) $(2)
endef
%.o: %.c $(toc2.bins.mkdep)
	@$(call toc2.call.compile-o-c,$@,$<)

########################################################################
# $(call toc2.call.compile-o-c++,dest_obj,source(s))
# Compiles one or more C++ files to a .o file.
#   $1 = destination .o file
#   $2 = source c++ file(s)
# If you want to pass object-specific flags to this compile then create
# an empty target like this:
#    my.o: CXXFLAGS+=...
# Unfortunately, make does not allow you to pass more than one
# target-specific variable per target, but you can pass multiples
# by making multiple empty targets:
#   my.o: CXXFLAGS+=-DFOO=1
#   my.o: CXXFLAGS+=-DBAR=2
# Alternately, define $(1).CXXFLAGS = ...
define toc2.call.compile-o-c++
  @$(call toc2.bins.mkdep.rule-c,$(1).d,$(2))
  @$(call toc2.call.setx-unless-quiet,"COMPILE C++ [$(2)] ..."); \
    $(COMPILE.cpp) $(INCLUDES) -o $(1) $(2)
endef
########################################################################
# Uses toc3.call.compile-o-c++ to compile $@ from $<.
%.o: %.cpp $(toc2.bins.mkdep) $(toc2.makefile.c)
	@$(call toc2.call.compile-o-c++,$@,$<)
########################################################################
# Uses toc3.call.compile-o-c++ to compile $@ from $<.
%.o: %.c++ $(toc2.bins.mkdep) $(toc2.makefile.c)
	@$(call toc2.call.compile-o-c++,$@,$<)
toc2.clean_files += *.d# i hate this wildcard here, but i don't see a way around it right now
toc2.bins.mkdep.dfiles := $(wildcard *.d)
$(if $(toc2.bins.mkdep.dfiles),$(eval include $(toc2.bins.mkdep.dfiles)),)

########################################################################
# $(call toc2.call.link-bin-o,dest_bin,source_objects)
# Links binary $(1) from $(2).
#  $1 = destination binary
#  $2 = source object file(s)
# To pass binary-specific linker flags, do:
#   $(1): LDFLAGS+=...
define toc2.call.link-bin-o
  $(call toc2.call.setx-unless-quiet,"LINK [$(1)] ..."); \
    $(LINK.cpp) $(LDFLAGS) -o $(1) $(2)
endef
# WTF does LINK.xx contain the CFLAGS/CPPFLAGS/CXXFLAGS???
########################################################################
# Uses toc2.call.link-bin-o to link $@ from $<.
%: %.o $(toc2.makefile.c)
	@$(call toc2.call.link-bin-o,$@,$<)


########################################################################
# toc2.eval.rules.c-bin is intended to be called like so:
# $(eval $(call toc2.eval.rules.c-bin,MyApp))
#
# It builds a binary named $(1) by running $(CXX) and passing it:
#
# INCLUDES, $(1).BIN.INCLUDES
# CFLAGS, $(1).BIN.CFLAGS
# CXXFLAGS, $(1).BIN.CXXFLAGS
# CPPFLAGS, $(1).BIN.CPPFLAGS
# LDFLAGS, $(1).BIN.LDADD
# $(1).BIN.OBJECTS $(1).BIN.SOURCES
#
# Note that we have to pass both CFLAGS and CPPFLAGS because .SOURCES might
# contain either of C or C++ files.
define toc2.eval.rules.c-bin
$(1).BIN := $(1)$(toc2.platform.file_extensions.exe)
$(1).BIN: $$($(1).BIN)
# Many developers feel that bins should not be cleaned by 'make
# clean', but instead by distclean, but i'm not one of those
# developers. i subscribe more to the school of thought that distclean
# is for cleaning up configure-created files.
# As always: hack it to suit your preference:
package.clean_files += $$($(1).BIN)
$$($(1).BIN): CFLAGS+=$$($(1).BIN.CFLAGS)
$$($(1).BIN): CPPFLAGS+=$$($(1).BIN.CPPFLAGS)
$$($(1).BIN): CXXFLAGS+=$$($(1).BIN.CXXFLAGS)
$$($(1).BIN): INCLUDES+=$$($(1).BIN.INCLUDES)
$$($(1).BIN): LDFLAGS+=$$($(1).BIN.LDADD)
$$($(1).BIN): $$($(1).BIN.SOURCES) $$($(1).BIN.OBJECTS) $(toc2.bins.mkdep) $(toc2.makefile.c)
	@test x = "x$$($(1).BIN.OBJECTS)$$($(1).BIN.SOURCES)" && { \
		echo "toc2.eval.rules.c-bin: $$@.bin: $(1).BIN.OBJECTS and/or $(1).BIN.SOURCES is undefined!"; \
		exit 1; }; true
	@test x != 'x$$($(1).BIN.SOURCES)' && {\
		$(call toc2.bins.mkdep.rule-c,$$(@).d,$$($(1).BIN.SOURCES)); }; true
	@$(call toc2.call.setx-unless-quiet,"CXX [$$@] ..."); \
        $$(CXX) -o $$@ \
                $$(INCLUDES) \
                $$(CFLAGS) \
                $$(CXXFLAGS) \
                $$(CPPFLAGS) \
                $$(LDFLAGS) \
                $$($(1).BIN.OBJECTS) $$($(1).BIN.SOURCES)
bins: $$($(1).BIN)
endef
########################################################################
# $(call toc2.call.rules.c-bin,[list]) calls and $(eval)s
# toc2.eval.rules.c-bin for each entry in $(1)
define toc2.call.rules.c-bin
$(foreach bin,$(1),$(eval $(call toc2.eval.rules.c-bin,$(bin))))
endef
# end ShakeNMake.CALL.RULES.bin and friends
########################################################################



########################################################################
# $(call toc2.eval.rules.dll,dll_basename) builds builds
# $(1)$(toc2.platform.file_extensions.dll) from object files defined by
# $(1).DLL.OBJECTS and $(1).DLL.SOURCES. Flags passed on to the linker
# include:
#
#   LDFLAGS, $(1).DLL.LDADD, -shared -export-dynamic
#   $(1).DLL.CPPFLAGS, $(1).DLL.OBJECTS, $(1).DLL.SOURCES
#
# Also defines the var $(1).dll, which expands to the filename of the DLL,
# (normally $(1)$(toc2.platform.file_extensions.dll)).
define toc2.eval.rules.c-dll
#$(if $($(1).DLL.OBJECTS)$($(1).DLL.SOURCES),,\
#     $(error toc2.eval.rules.c-dll $$(1)=$(1): either $(1).DLL.OBJECTS and/or $(1).DLL.SOURCES must be set.))
$(1).DLL = $(1)$(toc2.platform.file_extensions.dll)
ifneq (.DLL,$(toc2.platform.file_extensions.dll))
$(1).DLL: $$($(1).DLL)
endif
toc2.clean_files += $$($(1).DLL)
$$($(1).DLL): $$($(1).DLL.SOURCES)
$$($(1).DLL): $$($(1).DLL.OBJECTS)
$$($(1).DLL): $(toc2.bins.mkdep) $(toc2.makefile.c)
	@test x = "x$$($(1).DLL.OBJECTS)$$($(1).DLL.SOURCES)" && { \
	echo "toc2.eval.rules.c-dll: $$@: $(1).DLL.OBJECTS and/or $(1).DLL.SOURCES are/is undefined!"; exit 1; }; \
	$(call toc2.call.setx-unless-quiet,"CXX [$$@] ..."); \
	 $$(CXX) -o $$@ -shared -export-dynamic $$(LDFLAGS) \
		$$($(1).DLL.LDADD) $$($(1).DLL.OBJECTS) $$($(1).DLL.SOURCES) \
		$$($(1).DLL.CPPFLAGS)
endef
########################################################################
# $(call toc2.call.rules.c-dll,basename_list) calls and $(eval)s
# toc2.eval.rules.c-dll for each entry in $(1)
define toc2.call.rules.c-dll
$(foreach dll,$(1),$(eval $(call toc2.eval.rules.c-dll,$(dll))))
endef
# end toc2.call.rules.dll and friends
########################################################################


########################################################################
# $(call toc2.eval.rules.c-lib,libname) creates rules to build static
# library $(1)$(toc2.platform.file_extensions.lib) from
# $(1).LIB.OBJECTS.
define toc2.eval.rules.c-lib
$(1).LIB = $(1)$(toc2.platform.file_extensions.lib)
ifneq (.LIB,$(toc2.platform.file_extensions.lib))
  $(1).LIB: $$($(1).LIB)
endif
toc2.clean_files += $$($(1).LIB)
$$($(1).LIB): $$($(1).LIB.OBJECTS) $(toc2.makefile.c)
	@test x = "x$$($(1).LIB.OBJECTS)" && { \
	echo "toc2.eval.rules.c-lib: $$@: $(1).LIB.OBJECTS is undefined!"; exit 1; }; true
	@$(call toc2.call.setx-unless-quiet,"AR [$$@] ..."); \
		$$(toc2.bins.ar) crs $$@ $$($(1).LIB.OBJECTS)
endef
define toc2.call.rules.c-lib
$(foreach liba,$(1),$(eval $(call toc2.eval.rules.c-lib,$(liba))))
endef
# end toc2.eval.rules.c-lib
########################################################################
