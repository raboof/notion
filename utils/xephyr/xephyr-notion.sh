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
search_dir_script=$notion_root/utils/dev-search-dirs.sh

notion_exe=$notion_root/notion/notion

$notion_exe \
$($search_dir_script) \
-nodefaultconfig \
-smclientid testinxephyr \
-session $notion_root/.session \
-display $display \
$@
