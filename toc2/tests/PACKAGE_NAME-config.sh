#!/do/not/bash
# toc2_run_description = creating ${PACKAGE_NAME}-config
# toc2_begin_help =
#
# This test creates a script, ${top_srcdir}/${PACKAGE_NAME}-config,
# useful for trees building libraries. Clients of your library (in
# particular, their configure scripts) can get information about how
# to compile and link against your library via this script.
#
# Sample usage:
#  toc2_test_require package_config infile \
#	PACKAGE_PREFIX=LIBMYLIB_ \
#	CLIENT_LDADD="-L${prefix}/lib -lmylib" \
# 	CLIENT_INCLUDES="-I${prefix}/include"
#
#
# All variables passed to this test are passed directly on to
# ${TOC2_HOME}/bin/atsign_parse, with the exception of $1: if $1 is an
# existing file it is stripped from the list and use as an input
# template, otherwise it is passed on.
#
# A default template file for the script is provided with this test,
# toc/tests/PACKAGE_NAME-config.at, and it is used if $1 is not an
# existing file. The default template depends on the arguments
# show in the usage sample above. Client-provided templates can
# of course use whatever variables they like.
#
# In the default template, PACKAGE_PREFIX is a prefix which gets
# prepended to some variable names in the output, to allow
# package-config scripts from multiple libraries to produce
# non-colliding output. This value must be a single token, and should
# probably be something like LIBMYLIB_.
#
# Run the generated ${PACKAGE_NAME}-config script to see
# what it does.
#
# Remember to add ${PACKAGE_NAME}-config to the INSTALL_BINS
# and DISTCLEAN_FILES variables in your ${top_srcdir}/Makefile!
#
# = toc2_end_help

infile="$1"

if [ ! -e "$infile" ] ; then
    infile=${TOC2_HOME}/tests/PACKAGE_NAME-config.at
else
    shift
fi

ofile=${PACKAGE_NAME}-config

${TOC2_HOME}/bin/atsign_parse \
    PACKAGE_NAME="${PACKAGE_NAME}" \
    PACKAGE_VERSION="${PACKAGE_VERSION}" \
    prefix="${prefix}" \
    "$@" \
    < $infile > ${ofile} || {
    err=$?
    echo "Error filtering $infile to $ofile!"
    return $err
}
chmod +x ${ofile}
