#!/bin/sh

# Prepare a Notion release.
#
# Run this script in a freshly checked out notion directory
#
# Usage:
#
#   ~/dev/releases/notion$ ./predist.sh -snapshot
# or
#   ~/dev/releases/notion-3-20110219$ ./predist.sh

##
## Versioning
##

pwd=`pwd`
dir=`basename "$pwd"`

if test "$1" != "-snapshot"; then
    release=`echo "$dir"|sed 's/^[^-]\+-\([^-]\+-[0-9]\+\(-[0-9]\+\)\?\)$/\1/p; d'`

    if test "$release" = ""; then
        echo "Invalid package name $dir. Use the '-snapshot' option to create a snapshot package"
        exit 1
    else
        versdef="#define NOTION_RELEASE \"${release}\""
        perl -p -i -e "s/^#define NOTION_RELEASE.*/$versdef/" version.h
        #perl -p -i -e "s/NOTION_RELEASE/$release/" build/ac/configure.ac
    fi
else
    release=snapshot-`date +"%Y%m%d"`
    cd .. ; mv $dir "$dir-$release" ; cd "$dir-$release"
    dir="$dir-$release"
fi

cd ..
echo Creating notion-${release}.tar.gz
tar --exclude-vcs -czf notion-${release}-src.tar.gz $dir
echo Creating notion-${release}.tar.bz2
tar --exclude-vcs -cjf notion-${release}-src.tar.bz2 $dir
