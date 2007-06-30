#!/do/not/bash
# toc2_run_description = creating included makefiles
# Do not run this test manually: it is run by the toc core.


TOC2_MAKE=toc2.make

toc2_export_make TOC2_MAKE=${TOC2_MAKE}

# set -x
toc2_make_toc2_make ()
{
    # use a function because bash doesn't allow local vars outside of functions.
    local usage="usage: arg1==target makefile basename. arg2=input template"
    local themake=${1?$usage}
    local themakein=${2?$usage}
    echo "Creating $themake ... "
    local makeprops=${TOC2_TOP_SRCDIR}/.toc.make.tmp
    toc2_dump_make_properties > $makeprops
    local thedir
    local tocmake
    local relpath
    local shortform
    local tocmakeprops
    for d in $(find ${TOC2_TOP_SRCDIR} -name Makefile -o -name 'GNU[Mm]akefile' -o -name Makefile.toc \
        | xargs grep -E -l "include.+${themake}" | sort -u); do
        #echo "d=$d" >&2
        thedir=$(dirname $d)
	tocmake=${thedir}/$themake
        toc2_makerelative $thedir
        relpath=${TOC2_MAKERELATIVE}
	tocmake=${tocmake##$TOC2_TOP_SRCDIR/}  # make it short, for asthetic reasons :/
        shortform=${thedir##${PWD}/}
        test "$shortform" = "$PWD" && shortform= # top-most dir
#        echo "tocmake=$tocmake relpath=$relpath shortform=$shortform"

        tocmakeprops=${cmake}.props
        cp $makeprops $tocmakeprops
        cat <<EOF >> $tocmakeprops
TOC2_TOP_SRCDIR=${relpath##./}
TOC2_RELATIVE_DIR=${shortform##./}
EOF
        toc2_atfilter_file $tocmakeprops $themakein $tocmake \
            || toc2_die $? "Error creating $themake!"
        rm $tocmakeprops
        touch $tocmake # required for proper auto-reconfigure :/
    done
}

toc2_make_toc2_make ${TOC2_MAKE} ${TOC2_HOME}/make/${TOC2_MAKE}.at
err=$?
test $err != 0 && {
    echo "Error creating ${TOC2_MAKE} makefiles!"
    return $err
}

return $err





