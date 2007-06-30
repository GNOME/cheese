#!/do/not/bash
# toc2_run_description = creating ${PACKAGE_NAME}.pc
# toc2_begin_help =
#
# This test creates a data file for pkg-config, useful for trees
# building libraries. With this data clients of your library can get
# information about how to compile and link against your library via
# the pkg-config tool.

# The output file is called ./${PACKAGE_NAME}.pc, and should be added
# to the top-level makefile's INSTALL_PKGCONFIG and DISTCLEAN_FILES vars.
# It will be installed to ${prefix}/lib/pkgconfig.

# Sample usage:
#  toc2_test_require create_pkg-config

# Please see 'man pkg-config' for more information about pkg-config
# conventions.

# This test makes use of the following environment variables:
#
# - PACKAGE_FRIENDLY_NAME = a "friendly-form" name of the package.
#   Defaults to the toc-conventional PACKAGE_NAME.
#
# - PACKAGE_VERSION = toc-conventional
#
# - PACKAGE_DESCRIPTION = a description of the package. e.g., "A tool to
#   do Blah."
#
# - PACKAGE_LDADD = list of linker flags required to link against this
#   package, include -L and -l options. e.g. -L/usr/X11R6/lib
#   Note that -L${prefix}/lib is automatically added ONLY if this
#   var is NOT set.
#   Corresponds to pkg-config's Libs entry.
#
# - PACKAGE_CFLAGS = list of C preprocessor flags needed to compile
#   against this package. e.g., -I/usr/X11R6/include
#   Note that -I${prefix}/include is automatically added ONLY if this
#   var is NOT set.
#   Corresponds to pkg-config's CFlags entry.
#
# - PACKAGE_CONFLICTS = pkg-config 'Conflicts:' info string
#
# - PACKAGE_REQUIRES = pkg-config 'Requires:' info string
#
# Note that configure code may need append additional information to
# the output to the generated file, like "my_module_version", and
# other not-always-used entries.
#
# Achtung: this test will return with an error in some cases even
# when it creates the output file. For example, if PACKAGE_DESCRIPTION
# is not set then it will do so, primarily to verbosely warn the user
# to set it.
#
# Remember to add ${PACKAGE_NAME}.pc to the INSTALL_PKGCONFIG
# and DISTCLEAN_FILES variables in your ${top_srcdir}/Makefile!
#
# = toc2_end_help

_ret=0
for i in PACKAGE_DESCRIPTION; do
    test x = "x$(eval echo \${$i})" && {
        toc2_boldecho "Warning: variable not set: $i"
        _ret=1
    }

done

pkgconfdir=${prefix}/lib/pkgconfig
echo "x${PKG_CONFIG_PATH}" | grep "${pkgconfdir}" >/dev/null || {
cat <<EOF
${TOC2_BOLD_}Warning: the dir [$pkgconfdir] is not in the current
PKG_CONFIG_PATH. This means that pkg-config may not be able to find
${PACKAGE_NAME}.pc after installation.${_TOC2_BOLD}
EOF

}
unset pkgconfdir

cat <<EOF  > ${PACKAGE_NAME}.pc
# created by toc's create_pkg-config_data test. $(date)
prefix=${prefix}
exec_prefix=\${prefix}
libdir=\${prefix}/lib
includedir=\${prefix}/include

Name: ${PACKAGE_FRIENDLY_NAME-${PACKAGE_NAME}}
Description: ${PACKAGE_DESCRIPTION}
Version: ${PACKAGE_VERSION}
Requires: ${PACKAGE_REQUIRES}
Libs: ${PACKAGE_LDADD-"-L\${libdir}"}
Cflags: ${PACKAGE_CFLAGS-"-I\${includedir}"}

EOF

return $_ret
