# toc2_run_description = $@
# toc2_begin_help =
#
# Runs pkg-config --exists $1 and returns the result.
# If $2 is set then it is assumed to be a minimum version number
# and pkg-config --atleast-version $2 $1 is called. If $2 is set
# and the version test fails then this test returns non-zero.
#
# If the test succeeds then it calls toc2_export_make for two
# variables whos names are calculated based on $1. That is,
# $1 is upper-cased and any [.+-] symbols are converted to
# underscores. Then _CFLAGS and _LIBS are appended and the
# values of pkg-config --cflags $1 and --libs $1 are used
# as values. For example:
#
#  toc2_test_require pkg-config-exists foobar-0.10 0.10.13
#
#  If that passes, it will export:
#
#  FOOBAR_0_10_CFLAGS=$(pkg-config --cflags foobar-0.10)
#  FOOBAR_0_10_LIBS=$(pkg-config --libs foobar-0.10)
#
# = toc2_end_help
function pkg_config_check () {

  if pkg-config --exists $1; then
    if [[ ! -z "$2" ]]; then
      if pkg-config --atleast-version $2 $1; then
        toc2_loudly "${TOC2_EMOTICON_OKAY} found $1 >= $2"
      else
        return 2
        # die "$1 >= $2 was not found"
      fi
    else
        toc2_loudly "${TOC2_EMOTICON_OKAY} found $1"
    fi
    varname=$(echo $1 | tr -s '[a-z.+\-]' '[A-Z___]')
    toc2_export_make ${varname}_CFLAGS="$(pkg-config --cflags $1)" || toc2_die $? "toc2_export_make ${varname} failed"
    toc2_export_make ${varname}_LIBS="$(pkg-config --libs $1)" || toc2_die $? "toc2_export_make ${varname} failed"
  else
    echo
    if [[ -z "$2" ]]; then
      # die "$1 was not found"
      return 1
    else
      return 2
      # die "$1 >= $2 was not found"
    fi
  fi
}

pkg_config_check "$@"
return $?

