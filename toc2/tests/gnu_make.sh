# toc2_run_description = search for a genuine GNU make, the Make of makes.

toc2_find gmake || toc2_find make || return
toc2_export MAKE=${TOC2_FIND_RESULT}
"$MAKE" --version | grep -qi GNU > /dev/null || {
        echo "Your make appears to be non-GNU."
        return 1
}

