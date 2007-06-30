#!/do/not/bash
# toc2_run_description = If you appear to be stephan beal, set up some common stuff...


test xstephan = "x${USER}" || return 1

got=0

for h in cheyenne ludo jareth hoggle owl; do
    test x$h = "x${HOSTNAME}" && { got=1; break; }
done

test $got = 0 && return 1

echo "Setting up stephan's always-used settings..."

{ # for gnu_cpp_tools toc test:
    echo "Enabling debug/werror/wall."
    export configure_enable_debug=1
    export configure_enable_werror=1
    export configure_enable_wall=1
}

return 0
