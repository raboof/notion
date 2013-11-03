#!/bin/bash

# by Dan Weiner <dbw@uchicago.edu>
# Public Domain

# This script will install the scripts in SCRIPT_DIRS, via symlinks,
# to INSTALL_LOCATION.

# Make sure INSTALL_LOCATION doesn't contain anything important.
# It may get overwritten!

INSTALL_LOCATION="$HOME/.notion/lib"
SCRIPT_DIRS="keybindings scripts statusbar statusd styles"

########################################################################

for DIR in $SCRIPT_DIRS ; do
        [ -d $DIR ] && continue
        echo "$0: error: missing $DIR/." > /dev/stderr ; exit 1
done

mkdir -p "$INSTALL_LOCATION" || exit 1
find $SCRIPT_DIRS -type f |\
        while read FILE ; do
                echo "$FILE"
                ln -s -f "$PWD/$FILE" "$INSTALL_LOCATION" && continue
                echo "$0: error: ln returned $?." > /dev/stderr ; exit 1
        done

exit 0
