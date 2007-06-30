# toc2_run_description = looking for libdl
# toc2_begin_help =
#
# Looks for libdl. It exports the following config variables:
#
#	HAVE_LIBDL = 0 or 1
#	HAVE_DLFCN_H = currently always the same as HAVE_LIBDL.
#	LDADD_LIBDL = empty or "-ldl -rdynamic", possibly with -L/path...
#	LDADD_DL = same as LDADD_LIBDL
#
# Note that LDADD_LIBDL is specific to the libdl test, whereas LDADD_DL is used
# by both the libdl and libltdl tests, and possibly other libdl-like tests.
#
# = toc2_end_help

err=1

if toc2_test gcc_build_and_run \
    ${TOC2_HOME}/tests/c/check_for_dlopen_and_friends.c -rdynamic -ldl ; then
    err=0
    toc2_export HAVE_LIBDL=1
    toc2_export HAVE_DLFCN_H=1
    toc2_export HAVE_DLOPEN=1
    toc2_export LDADD_LIBDL="-ldl -rdynamic"
    toc2_export LDADD_DL="${LDADD_LIBDL}"
fi

return $err
