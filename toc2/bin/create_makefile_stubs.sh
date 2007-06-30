#!/bin/sh
#
# Generate stub Makefiles for toc.
#
# Usage: see help text, below



dirs=
x_help=
while test x != "x$1" ; do
    arg="$1"
    shift
    case "$arg" in
        -?|--help|-help)
            x_help=1
        ;;
        *) test -d $arg && dirs="$dirs $arg"
        ;;
    esac
done
#
test x1 = "x$x_help" && {
    cat <<EOF
$0:
Creates stub files for a toc environment by looking at the
contents of a directory.

Usage:
$0 dir1 [... dirN]

dir1 defaults to ., which should be the \$(top_srcdir) of a project
tree.

For each dir it creates dir/Makefile.suggestion, containing guesses at
what toc-related content might be useful in the corresponding Makefile.

If called without any options then it acts RECURSIVELY on ".". It does
not act recursively if called with directory arguments. This behaviour
is arguable, but seems to be reasonable for the use-cases to-date (all
1.5 of them ;).

EOF
    exit 0
}


stderr ()
{ # sends $@ to stderr
    echo "#" "$@" 1>&2
}

slashify ()
{ # converts file lists into a format which is more useful to Makefile maintainers.
  # usage: echo list | slashify
    perl -ne 's|\n| |g;s|\s+$||;s|\s+|%|g;s|%$|\n|;s|%| \\\n\t|g; print $_,"\n";'
}

########################################################################
# check_make_XXX() conventions:
#
# $1 is the directory name, but they don't really need it. A chdir is
# done before processing each dir, so all tests can safely assume pwd
# is the directory they are responsible for checking.
#
# They "should" follow the naming convention check_make_XXX so they
# can easily be handled in a loop using only their XXX parts.
########################################################################

########################################################################
check_make_flexes ()
{ # hanles *.flex
    export flext=flex
    local flexes="$(ls *.${flext} 2>/dev/null)"
    test "x$flexes" != "x" && {
        stderr "Adding FLEXES"
        echo "############## FLEXES:"
        echo "# WARNING: FLEXES stuff only works for C++-based flexers"
        echo -n "FLEXES = "
        echo $flexes | sed s/\.${flext}//g
        echo "FLEXES_ARGS = -+ -p"
        for f in $flexes; do
            base=${f%%.${flext}}
            echo "${base}_FLEXES_ARGS = -P${base}"
        done
        echo -n "OBJECTS += "
        echo $flexes | sed s/\.${flext}/.${flext}.o/g
        echo "include \$(TOC_MAKESDIR)/flex.make"
        echo "# Run target FLEXES to process these."
        echo "# REMINDER: add the generated C++ files to your SOURCES, if needed."
    echo "############## /FLEXES"
    }

}
########################################################################
check_make_c ()
{
# checks for c/c++-related stuff

    local headers="$(ls *.h *.hpp 2>/dev/null )"
    test -n "$headers" && {
        stderr "Adding HEADERS"
        echo -n "HEADERS = ";
        echo $headers | slashify
        echo "DIST_FILES += \$(HEADERS)"
        echo "INSTALL_PACKAGE_HEADERS += \$(HEADERS)"
        echo
    }

    local sources="$(ls *.c *.cpp *.c++ *.C *.cxx 2>/dev/null )"
    test -n "$sources" && {
        stderr "Adding SOURCES"
        echo -n "SOURCES = ";
        echo $sources | slashify
        echo "DIST_FILES += \$(SOURCES)"
        echo -n "OBJECTS = "
        echo $sources | perl -pe 's|(\S+)\.\w+\s|$1.o |g' | slashify
        echo
        echo "CLEAN_FILES += \$(OBJECTS)"
        cat <<EOF

build_libs = 0
LIBNAME = libfoo
ifeq (1,\$(build_libs))
  STATIC_LIBS = \$(LIBNAME)
  SHARED_LIBS = \$(LIBNAME)
  \$(LIBNAME)_a_OBJECTS = \$(OBJECTS)
  \$(LIBNAME)_so_OBJECTS = \$(\$(LIBNAME)_a_OBJECTS)
  # \$(LIBNAME)_so_VERSION = \$(PACKAGE_VERSION)
  # \$(LIBNAME)_so_LDADD = 
  include \$(TOC_MAKESDIR)/SHARED_LIBS.make
  include \$(TOC_MAKESDIR)/STATIC_LIBS.make
  # Run targets STATIC_LIBS and SHARED_LIBS build these.
SHARED_LIBS: STATIC_LIBS
endif

build_bins = 0
BINNAME = mybin
ifeq (1,\$(build_bins))
  BIN_PROGRAMS = \$(BINNAME)
  \$(BINNAME)_bin_OBJECTS = \$(OBJECTS)
#  \$(BINNAME)_bin_LDADD =
  include \$(TOC_MAKESDIR)/BIN_PROGRAMS.make
  INSTALL_BINS += \$(BIN_PROGRAMS)
  # Run target BIN_PROGRAMS to build these.
endif

EOF

    } #end SOURCES (and friends)


} # end check_make_c


########################################################################
check_make_extra_dist ()
{ # Add some common filenames/patterns to DIST_FILES
    local xtra=
    # reminder: toc adds Makefile to DIST_FILES by default, because it's ALWAYS needed.
    for x in README LICENSE NEWS ChangeLog \
            *.at *.qmake \
            *.sh *.pl \
            *.bat \
            *.txt *.TXT *.xml *.lyx \
	*.odt *.php *.html \
    ; do
        expr "$x" : '\*' >/dev/null && continue # unexpanded wildcard
        test -e $x || continue
        xtra="$xtra $x"
   done

    test -z "$xtra" && return
    stderr "Adding extra DIST_FILES."
    echo -n "DIST_FILES += "
    echo $xtra | slashify

}

########################################################################
check_make_subdirs ()
{ # add subdirs to SUBDIRS
    local ls="$(ls -d *)"
    test -z "$ls" && return

    local subs=
    for i in $ls; do
        test -d $i || continue;
        test "CVS" = "$i" && continue
        stderr "Adding SUBDIR $i"
        subs="$subs $i"
    done
    test -n "$subs" && echo "SUBDIRS = $subs"
}




# checks for existing Makefile
check_make_makefile ()
{
    true
}



##############################################################################
# main app driver goes here:

test x = "x$dirs" && {
    dirs="$(find . -type d | sed -e '/\/CVS/d')"
}

test x = "x$dirs" && {
    echo "Error: no subdirectories found!"
    exit 1
}

timestamp=$(date)
for d in $dirs; do
    cd $d >/dev/null || {
        err=$?
        echo "Error: could not cd to $d"
        exit $err
    }
    out=Makefile.suggestion
    stderr "Creating $d/$out"
    {
        cat <<EOF
#!/usr/bin/make -f
###################################################
# AUTO-GENERATED guess at a toc-aware Makefile,
# based off of the contents of directory:
#   $d
# Created by $0
# $timestamp
# It must be tweaked to suit your needs.
###################################################
include toc.make
EOF
        for func in subdirs extra_dist flexes c ; do
            check_make_${func} $d
        done
        cat <<EOF
all:
# Don't forget to any any SUBDIRS to 'all', if needed, with:
#  all: subdir-NAME
# or:
#  all: subdirs
###################################################
# end auto-generated rules
###################################################
EOF
    } > $out
    cd - > /dev/null
done 


########################################################################
# the end
########################################################################
