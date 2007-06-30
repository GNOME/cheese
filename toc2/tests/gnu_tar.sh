# toc2_run_description = search for a genuine GNU tar
# toc2_begin_help =
# Looks for a genuine GNU tar. It first looks for gtar, and then tar, and checks
# the binary to see if it is a GNU version.
# It honors the configure argument --with-tar=/path/to/tar.
#
# Exports:
# 	- TAR_BIN=/path/to/tar (or empty string)
#	- TAR=/path/to/tar (same as TAR_BIN, but deprecated)
#
# = toc2_end_help

toc2_export TAR=
toc2_export TAR_BIN=

gtar=${configure_with_TAR-gtar}
toc2_find $gtar || toc2_find tar || {
        echo "tar/gtar not found in PATH"
        return 1
}
gtar=$TOC2_FIND_RESULT
"$gtar" --version | grep -i GNU > /dev/null || {
        echo "Your 'tar' ($gtar) appears to be non-GNU."
        return 1
}

toc2_find ${TOC2_HOME}/make/TARBALL.make
toc2_export TARBALL_MAKE=${TOC2_FIND_RESULT}
toc2_export TAR=$gtar
toc2_export TAR_BIN=$gtar

return 0