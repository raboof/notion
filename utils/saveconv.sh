#!/bin/sh
#
# Convert older .lua workspaces savefiles so that can be loaded in
# newer Ion.
#


if test $# -lt 1; then
    echo 'Usage: $0 workspaces-*.lua > workspaces.lua'
    exit 1
fi
    
cat << EOF
local arg={'dummy'}
local name, scrno, subs=nil, nil, {}
local function region_set_name(unused, name_)
    name=name_
end
local function region_manage_new(unused, sub)
    table.insert(subs, sub)
end
local function doit()
    initialise_screen_id(scrno, {name=name, subs=subs})
    subs={}
    name=nil
end
EOF

foo=0
for ws in "$@"; do
    scrno=`echo $ws|sed 's/.*\.\([0-9]\)\+\.lua/\1/p; d'`
    if test x"$scrno" = x; then
    	echo "Unable to figure out screen number from file name $ws"
    fi
    
    if test x$foo = x1; then
    	echo 'doit()'
    fi
    echo "scrno=$scrno"
    cat $ws
    foo=1
done

echo 'doit()'
