#!/bin/bash
#
# run the local dev build in xephyr
#
# usage:
#
# ./start-xephyr.sh &
# ./xephyr-notion.sh

display=${NOTION_TEST_DISPLAY:-:1.0}

this_rel=$(dirname ${BASH_SOURCE[0]})
rel_root=$this_rel/../..

notion_root=$(realpath $rel_root)

src_search_dirs() {
    prefix='-searchdir '
    ls -d \
       $notion_root/de \
       $notion_root/etc \
       $notion_root/ioncore \
       $notion_root/contrib/scripts \
       $notion_root/mod_* \
        | sed "s/^/$prefix/" | xargs
}

notion_exe=$notion_root/notion/notion

$notion_exe \
$(src_search_dirs) \
-nodefaultconfig \
-smclientid testinxephyr \
-session $notion_root/.session \
-display $display \
$@
