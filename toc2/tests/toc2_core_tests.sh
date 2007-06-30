#!/do/not/bash
# toc2_run_description = Looking for required build components...
# toc2_begin_help =
# To be sourced from toc2_core.sh. This is the core sanity checker for
# toc. Any test which MUST pass for a tree to be considered
# toc-capable may be placed in here.
# = toc2_end_help

toc2_run toc2_running_under_cygwin
# toc2_quietly "Looking for required build components:"
for x in \
    bash=SHELL \
    cat=CAT \
    cut=CUT \
    ls=LS \
    perl=PERL \
    sed=SED \
    xargs=XARGS \
    ; do
    f=${x%%=*}
    v=${x##*=}
    toc2_find $f || {
        echo "configure couldn't find required app: $f"
        return 1
    }
    test "x$v" = "xXtocX" && continue
    toc2_export_make $v=${TOC2_FIND_RESULT}
done

toc2_find less || toc2_find more || {
    echo "Found neither 'more' nor 'less'!!!"
    return 1
}

toc2_find_require install-sh ${TOC2_HOME}/bin
# toc2_export_make toc2.bins.installer="\$(top_srcdir)/${TOC2_FIND_RESULT##${PWD}/}"
toc2_export_make toc2.bins.installer="${TOC2_FIND_RESULT}"

toc2_find_require makedist ${TOC2_HOME}/bin
toc2_export_make toc2.bins.makedist="${TOC2_FIND_RESULT}"
# it's very arguable to make makedist a required component :/

for x in \
    awk \
    gnu_make \
    gnu_find \
    gnu_tar \
    ; do
    toc2_test $x || {
        echo "${TOC2_EMOTICON_ERROR} $x test failed."
        return 1
    }
done

#toc2_test gnu_install || {
#    boldecho "Didn't find GNU install. You won't be able to do a 'make install'."
#}

toc2_quietly "Looking for optional build components:"
for x in \
    gzip=GZIP \
    bzip2=BZIP \
    zip=ZIP \
    ; do
    f=${x%%=*}
    v=${x##*=}
    foo=configure_with_${f}
    toc2_find $f
    toc2_export_make $v=${TOC2_FIND_RESULT}
done

return 0
