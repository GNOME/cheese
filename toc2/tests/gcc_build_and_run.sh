# toc2_run_description = $@
#
# toc2_begin_help = 
#
# Tries to compile $@ using ${CC} and returns the error code which the
# compiler returns. $@ may be made up of any flags which you want to
# pass to the compiler, such as -I/some/path. Do not pass -c nor
# -o, as those are handled by this test.
#
# This code uses ${INCLUDES}. You can easily set it only for the
# duration of this test by doing something like the following:
#
# INCLUDES="-I/path1 -I/path2" toc2_test_require gcc_build_and_run myfile.c
#
# This is an alternative to using the more verbose approach of saving
# INCLUDES, adjusting it, calling this test and re-setting INCLUDES.
#
# = toc2_end_help


toc2_get_make CC
CC=${TOC2_GET_MAKE}
test "x$CC" = x && {
    echo "This test requires that the config variable CC have been set to point to gcc."
    echo "Try running the gnu_cpp_tools test, or another test which sets C/C++-related variables, or call toc2_export_make CC=/path/to/gcc."
    return 1
}

tmpprefix=.toc2.build_and_run
binfile=./$tmpprefix
compiler_out=$tmpprefix.out
cmd="$CC ${INCLUDES} $@ -o $binfile"
echo $cmd > $compiler_out
$cmd >> $compiler_out  2>&1
err=$?
test x0 = x$err || {
    echo "Compiler output is in $compiler_out (but will be erased the next time this test is run!)."
    return $err
    err=$?
}
echo $binfile >> $compiler_out
$binfile >> $compiler_out 2>&1
err=$?
test x0 != x$err && {
    echo "output is in $compiler_out (but will be erased the next time this test is run!)."
} || {
    rm $binfile $compiler_out
}
return $err
