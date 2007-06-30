# toc2_run_description = building under cygwin?

test -d /cygdrive/c
err=$?
if test $err = 0 ; then
    toc2_add_config SMELLS_LIKE_CYGWIN=1
    echo "Detected cygwin."
else
    toc2_add_config SMELLS_LIKE_CYGWIN=0
fi
return $err


