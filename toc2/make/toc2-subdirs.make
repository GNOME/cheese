#!/do/not/make
# ^^^ help out emacs
# Makefile snippet to traverse subdirs.
# Sample usage:
#    package.subdirs = foo bar
#    include toc2.make
#    all: subdirs
#
# NOTES:
#
#  - package.subdirs must be defined BEFORE including this file. This is a
# side-effect of the implementation: make expands this var
# at compile time, so to say, so it can build the proper subdir
# targets. Adding to the package.subdirs var later doesn't affect which targets
# get created, but ARE necessary for targets like install-subdirs :/.
#
#  - You can add subdirectories post-include by doing this:
#    subdirs: subdir-XXXX
# where XXX is a directory, but that has the disadvantage of not adding
# the subdir to the, e.g., cleanup or install list when those traverse
# subdirs.
#
#  - You can also use:
#    subdirs: subdirs-XXX
#  which runs XXX in all $(package.subdirs).
#
# DO NOT put "." in the package.subdirs! Instead, client Makefiles completely
# control dir build order via dependencies.

########################################################################
# $(call toc2.call.make-subdirs,dirs_list[,target_name=all])
# Walks each dir in $(1), calling $(MAKE) $(2) for each one.
#
#   $1 = list of dirs
#   $2 = make target
#
# Note that this approach makes parallel builds between the dirs in
# $(1) impossible, so it should only be used for targets where
# parallelizing stuff may screw things up or is otherwise not desired
# or not significant.
toc2.call.make-subdirs = \
	test "x$(1)" = "x" -o "x." = "x$(1)" && exit 0; \
	tgt="$(if $(2),$(2),all)"; \
	make_nop_arg=''; \
	test 1 = 1 -o x1 = "x$(toc2.flags.quiet)" && make_nop_arg="--no-print-directory"; \
	for b in $(1) "x"; do test ".x" = ".$$b" && break; \
		pwd=$$(pwd); \
		deep=0; while [[ $$deep -lt $(MAKELEVEL) ]]; do echo -n "  "; deep=$$((deep+1)); done; \
		echo "Making '$$tgt' in $${PWD}/$${b}"; \
		$(MAKE) -C $${b} $${make_nop_arg} $${tgt} || exit; \
		cd $$pwd || exit; \
	done

.PHONY: subdirs $(package.subdirs)
$(package.subdirs):
	@$(call toc2.call.make-subdirs,$@,all)
subdirs: $(package.subdirs)
subdir-%:# run all in subdir $*
	@$(call toc2.call.make-subdirs,$*,all)
subdirs-%:# run target % in $(package.subdirs)
	@$(call toc2.call.make-subdirs,$(package.subdirs),$*)
