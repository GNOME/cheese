#!/do/not/bash
# toc2_run_description = creating $2

test x = "x$2" && {
    echo "Error: this test requires args \$1 and \$2:"
    echo "\$1 = input (template) file."
    echo "\$2 = output file."
    return 1
}

toc2_atfilter_as_makefile $1 $2
