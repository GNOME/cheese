# toc2_run_description = $@
#
# toc2_begin_help = 
#
# Tries to compile $@ using ${CC} and returns the error code which the
# compiler returns. $@ may be made up of any flags which you want to
# pass to the compiler, such as -I/some/path.
#
# This code uses ${INCLUDES}. You can easily set it only for the
# duration of this test by doing something like the following:
#
# INCLUDES="-I/path1 -I/path2" toc2_test_require gcc_try_compile myfile.c
#
# This is an alternative to using the more verbose approach of saving
# INCLUDES, adjusting it, calling this test and re-setting INCLUDES.
#
# = toc2_end_help


#test x"$@" = x && {
# ^^^ this no workie when, e.g., -I... is passed in, because -I is a flag used by test :/
#    echo "usage: try_compile cc_flag|file1.{c,cpp...} [cc_flag|file2...]"
#    return 1
#}

toc2_get_make CC
CC=${TOC2_GET_MAKE}
test "x$CC" = x && {
    echo "This test requires that the config variable CC have been set to point to gcc."
    echo "Try running the gnu_cpp_tools test, or another test which sets C/C++-related variables, or call toc2_export_make CC=/path/to/gcc."
    return 1
}

tmpprefix=.toc.try_compile
dotofile=$tmpprefix.o
compiler_out=$tmpprefix.out
cmd="$CC ${INCLUDES} $@ -c -o $dotofile"
echo $cmd > $compiler_out
$cmd >> $compiler_out  2>&1
err=$?

test -f $dotofile && rm $dotofile
test $err != 0 && {
    echo "Compiler output is in $compiler_out (but will be erased the next time this test is run!)."
} || {
    rm $compiler_out
}
return $err
