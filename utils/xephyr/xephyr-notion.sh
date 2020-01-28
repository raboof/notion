#!/usr/bin/env bash
#
# run the local dev build in xephyr
#
# usage:
#
# ./start-xephyr.sh &
# ./xephyr-notion.sh

set -e

display=${NOTION_TEST_DISPLAY:-:1.0}

this_rel=$(dirname ${BASH_SOURCE[0]})
rel_root=$this_rel/../..

notion_root=$(realpath $rel_root)
search_dir_script=$notion_root/utils/dev-search-dirs.sh

notion_exe=$notion_root/notion/notion
notion_args=( $($search_dir_script) \
	-nodefaultconfig -smclientid testinxephyr \
	-session "$notion_root/.session" -display $display )

debugger=none # gdb or lldb?
[[ ${BASH_SOURCE[0]} == *-lldb.sh ]] && debugger=lldb
[[ ${BASH_SOURCE[0]} == *-gdb.sh ]] && debugger=gdb

while [[ $# -gt 0 ]]; do
	if [[ $1 = "--" ]]; then
      shift
      break
  else
      notion_args+=("$1")
      shift
  fi
done

case "$debugger" in
    lldb) lldb "$@" -- $notion_exe ${notion_args[*]} ;;
    gdb)  gdb  "$@"    $notion_exe -ex "run ${notion_args[*]}" ;;
    none) $notion_exe ${notion_args[*]} ;;
    *) echo "error: unknown debugger: $debugger"; exit 1 ;;
esac
