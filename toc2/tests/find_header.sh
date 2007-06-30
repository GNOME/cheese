#!/do/not/bash
# toc2_run_description = Looking for header file $1
# toc2_begin_help =
#
# Searches for a header file, passed as $1,
# in the search path $2. $2 defaults to:
# ${prefix}/include:/usr/include:/usr/local/include
#
# After running this the variable TOC2_FIND_RESULT
# holds the position of the file, or is empty
# if the file is not found.
#
# The environment variable FIND_HEADER_PATH is set to
# to path where $1 was found. Note that it is reset
# on every call to this test, and is not toc2_exported.
#
# = toc2_end_help


test x = "x$1" && {
    cat <<EOF
Usage error: 
    \$1 should be the name of a header file.
    \$2 is an optional search path.
EOF
    return 1
}

header=$1
shift
defname=HAVE_$(echo ${header} | tr '[a-z/.]' '[A-Z__]')
toc2_find $header ${@-${prefix}/include:/usr/include:/usr/local/include}
export FIND_HEADER_PATH="${TOC2_FIND_RESULT%%/$header}"
if test -n "${FIND_HEADER_PATH}"; then
    toc2_export ${defname}=1
else
    toc2_export ${defname}=0
fi
unset header
test x != "x${TOC2_FIND_RESULT}"
return $?




