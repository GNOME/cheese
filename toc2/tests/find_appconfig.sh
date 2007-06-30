#!/do/not/bash
# toc2_run_description = looking for $1-config

# toc2_begin_help =
# This test searches for $1-config in the path $2.
# The search path defaults to ${prefix}/bin:$PATH.
#
#
# The config var ${1}_config is exported, and will contain the path of
# the config app or empty if it was not found. Note that any "-"
# characters in $1 are converted to "_" for this purpose. 

# FIND_APPCONFIG is also exported to the path of the found config app,
# but will be reset on each call to this function. It is intended to
# be helpful when running this test in loops, e.g., for x in foo bar;
# do ... done

# Example:
#
#    toc2_test_require find_appconfig my-lib
#
# will search for my-lib-config in ${prefix}/bin:$PATH
# and will export ${my_lib_config} to the path where it
# is found.
#
# = toc2_end_help

_app=${1}

test x = "x$_app" && {
    cat <<EOF
Usage error: this test requires the base name of a lib or app as it's
first argument, and an optional PATH as it's second argument.
Example:
    toc2_test_require find_appconfig gtk
will search for gtk-config in ${prefix}/bin:$PATH
EOF
    return 1

}

_spath=${2-${prefix}/bin:$PATH}

toc2_find ${_app}-config $_spath
_app=$(echo $_app | sed -e s/-/_/g)
toc2_export ${_app}_config=${TOC2_FIND_RESULT}
toc2_export FIND_APPCONFIG=${TOC2_FIND_RESULT}
unset _app
unset _spath
test x != "x${TOC2_FIND_RESULT}"
return $?

