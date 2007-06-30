# toc2_run_description = search for GNU find


toc2_find find || return 1
app=${TOC2_FIND_RESULT}
"$app" --version | grep -i GNU > /dev/null || {
        echo "Your 'find' ($app) appears to be non-GNU."
        return 1
}
toc2_export_make FIND=$app
return 0

