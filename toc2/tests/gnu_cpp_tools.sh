# toc2_run_description = looking for GNU C/C++ compiler/linker tools.
# toc2_begin_help =
#  It takes the following configure arguments:
#  --enable-debug    causes stuff to be built -g instead of -02.  This is a
#                    little weird, but you can control the optimization level
#                    with --enable-debug="-O3 -fno-inline" etc.
#  --enable-warn     causes stuff to be built -Wall.  To turn on -Werror, do: --enable-warn="-Wall -Werror" etc.
#   --enable-werror  same as --enable-warn="-Wall -Werror"
#
#  It calls toc2_export for the following:
#  - CC         /path/to/gcc?
#  - LD         /path/to/ld
#  - LDD        /path/to/ldd
#  - CXX        /path/to/g++
#  - AR         /path/to/ar
#  - NM         /path/to/nm (optional component)
#  - STRIP      /path/to/strip (optional component)
#  - INCLUDES   probably empty
#  - CPPFLAGS   probably empty
#  - CFLAGS_OPT same as OPT (prefered for naming's sake)
#  - WARN       -Wall (or some other value specified by --enable-warn or --enable-werror)
#  - CFLAGS     probably empty (or maybe -pipe)
#  - CXXFLAGS   probably empty (or maybe -pipe)
#  - LDFLAGS    probably empty
# = toc2_end_help

CC=

: ${configure_enable_debug=0}
: ${configure_enable_warn=0}
: ${configure_enable_werror=0}

for foo in \
    ar-AR gcc-CC g++-CXX ld-LD ldd-LDD \
    ; do
    key=${foo%%-*}
    var=${foo##*-}
    toc2_find $key || {
        echo "Couldn't find required app: $key in PATH [$PATH]"
        return 1
    }
    toc2_export $var=${TOC2_FIND_RESULT}
done

for foo in \
    nm-NM strip-STRIP \
    ; do
    key=${foo%%-*}
    var=${foo##*-}
    toc2_find $key
    toc2_export $var="${TOC2_FIND_RESULT}"
done


toc2_export INCLUDES="$INCLUDES"
toc2_export CPPFLAGS="$CPPFLAGS"
if test "x${configure_enable_debug}" = x1; then
    CFLAGS_OPT="-g -DDEBUG -D_DEBUG $CFLAGS_OPT"
elif test "x${configure_enable_debug}" = x0; then
    #  What's a sensible default here?  -O2?  Put it first in the hopes
    #  that any values already in $CFLAGS_OPT will take precedence.
    CFLAGS_OPT="-O2 $CFLAGS_OPT -DNDEBUG"
else
    #  They specified some flags.
    CFLAGS_OPT="$configure_enable_debug"
fi

toc2_export CFLAGS_OPT="$CFLAGS_OPT"

if test "x${configure_enable_warn}" = x1; then
    WARN="-Wall $WARN"
elif test "x${configure_enable_warn}" = x0; then
	WARN=
else
    #  They specified some flags.
    WARN="$configure_enable_warn"
fi

if test "x${configure_enable_werror}" != x0; then
    if test "x${configure_enable_werror}" = x1; then
        WARN="-Wall -Werror $WARN"
    else
        WARN="$WARN ${configure_enable_werror}"
    fi
fi
    


toc2_export WARN="$WARN"

#  Presumably we could determine whether -pipe works instead of assuming
#  it does...
CFLAGS="-pipe $CFLAGS ${WARN} ${CFLAGS_OPT}"
CXXFLAGS="-pipe $CXXFLAGS ${WARN} ${CFLAGS_OPT}"
toc2_export CFLAGS="$CFLAGS"
toc2_export CXXFLAGS="$CXXFLAGS"
toc2_export LDFLAGS="$LDFLAGS"
