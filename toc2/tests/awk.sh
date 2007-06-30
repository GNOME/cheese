# toc2_run_description = looking for gawk/awk/nawk

err=0
toc2_find gawk || toc2_find awk || toc2_find nawk || err=1
toc2_export_make AWK=$TOC2_FIND_RESULT
return $err
