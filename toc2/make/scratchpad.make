#!/do/not/make

$(error do not include this file! It is a placeholder/scratchpad!)

########################################################################
# $(call toc2.call.prepare-c-deps[,source_file_list])
#   [$1] = a single C/C++ source file, defaults to a wildcard search
# For each file F in $1, F.d is created, "-include"d, and added to
# toc2.clean_files.
# If MAKECMDGOALS contains 'clean' then the deps are not generated.
# If $(toc2.bins.mkdep) is empty then this function is a no-op.
#
# Maintenance reminder: Unfortunately we can't mix 'if' and 'define'
# blocks, so we do a bit of ugly work in the $(1).d target here.
#
# $(call toc2.eval.prepare-c-deps-impl,source_file) is the internal
# implementation of toc2.call.prepare-c-deps, and should NOT be called
# by client code. $1=a SINGLE C/C++ file name.
define toc2.eval.prepare-c-deps-impl
  $(1):
  $(basename $(1)).o: $(1).d
  $(1).d: $(1) $(toc2.bins.mkdep)
	@$(if $(toc2.bins.mkdep),\
		$(if $(filter clean,$(MAKECMDGOALS)),@true,\
			$(toc2.bins.mkdep) $(toc2.bins.mkdep.flags) -- $(1) > $$@),); \
	echo '$$$$(warning DEPENDENCIES: including $$@)' >> $$@
  toc2.clean_files += $(1).d
  $(eval -include $(1).d)
endef
define toc2.call.prepare-c-deps
  $(foreach SRC,\
	$(if $(1),$(1),$(wildcard *.cpp *.c *.c++ *.C)),\
	$(eval $(call toc2.eval.prepare-c-deps-impl,$(SRC))))
endef

#define toc2.call.rule.c-deps
#%$(1).d: %$(1)
#	$(toc2.bins.mkdep.rule-c)
##toc2.clean_files += *$(1).d
#endef
#$(foreach EXT,.c .cpp .c++,$(eval $(call toc2.call.rule.c-deps)))
