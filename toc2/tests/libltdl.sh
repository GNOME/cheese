# toc2_run_description = looking for libltdl
# toc2_begin_help =
#
# Looks for libltdl. It calls toc2_export for the
# following variables:
#
#	HAVE_LIBLTDL = 0 or 1
#	LDADD_LIBLTDL = -lltdl or empty
#	LDADD_DL = empty or "-lltdl -rdynamic", possibly with -L/path...
#
# Note that LDADD_LIBLTDL is specific to the libltdl test, whereas
# LDADD_DL is used by both the libdl and libltdl tests.
#
# = toc2_end_help
# Many thanks to Roger Leigh for introducing me to ltdl!

toc2_add_config HAVE_LIBLTDL=0
toc2_add_config LDADD_LIBLTDL=
toc2_export LDADD_DL=

err=1

if toc2_test gcc_build_and_run ${TOC2_HOME}/tests/c/check_for_ltdlopen_and_friends.c -rdynamic -lltdl; then
    err=0
    toc2_export HAVE_LIBLTDL=1
    toc2_export LDADD_LIBLTDL="-lltdl -rdynamic"
    toc2_export LDADD_DL="${LDADD_LIBLTDL}"
fi

return $err



