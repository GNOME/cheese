#!/do/not/bash

# Creates toc.${PACKAGE_NAME}.make from toc.${PACKAGE_NAME}.make.at
# This file is sought by toc2.make (but is always optional).

mf=${1-toc2.${PACKAGE_NAME}.make}
tmpl=$mf.at
test -f $tmpl || {
        toc2_export_make toc2_include_project_makefile=0
cat <<EOF
No project-specific makefile found at [$mf].
This is not technically a problem, but please ensure that your
tree does not need this file. If it does not, then please
call 'touch $tmpl' to create an empty one.
EOF
	return 1
}

toc2_atfilter_as_makefile $tmpl $mf || {
    err=$?
    echo "Error filtering $tmpl!"
    return $err
}

# toc2_export_make toc2_include_project_makefile="\$(top_srcdir)/${mf}"
