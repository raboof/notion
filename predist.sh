#!/bin/sh

# Prepare a Notion release.
#
# Run this script in a freshly checked out notion directory: never commit the
# changes made by this script (but perhaps tag them).
#
# Usage:
#
#   ~/dev/releases/notion$ ./predist.sh -snapshot
# or
#   ~/dev/releases/notion-3-20110219$ ./predist.sh

if test "$NOTION_REPOS" = ""; then
    NOTION_REPOS=git://notion.git.sourceforge.net/gitroot/notion
fi

do_git_export() {
	url="$1"
	project="$2"

	set -e
	
	! test -f "${project}" || { echo "${project} exists"; exit 1; }

	git clone "${url}" "${project}"
	cd "${project}" ; git log > exact-version ; cd ..
	rm -rf "${project}/.git"
}

##
## Versioning
##

pwd=`pwd`
dir=`basename "$pwd"`

if test "$1" != "-snapshot"; then
    release=`echo "$dir"|sed 's/^[^-]\+-\([^-]\+-[0-9]\+\(-[0-9]\+\)\?\)$/\1/p; d'`

    if test "$release" = ""; then
        echo "Invalid package name $dir."
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


##
## Libs
##

getlib() {
    do_git_export $1 $2
    rm $2/build/rules.mk $2/system.mk
    #ln -s ../../build/rules.mk $2/build/rules.mk
    cat > $2/build/system-inc.mk << EOF
TOPDIR := \$(TOPDIR)/..
include \$(TOPDIR)/build/system-inc.mk
EOF

}

##
## Modules
##

getmod() {
    do_git_export $1 $2
    sed -i -e "s/TOPDIR=..\/notion/TOPDIR=..\//" $2/Makefile
}

getlib $NOTION_REPOS/libtu libtu
getlib $NOTION_REPOS/libextl libextl
getmod $NOTION_REPOS/mod_xinerama mod_xinerama
getmod $NOTION_REPOS/mod_xrandr mod_xrandr
getmod $NOTION_REPOS/mod_xkbevents mod_xkbevents
do_git_export $NOTION_REPOS/contrib contrib

##
## Makefiles
##

mkdist() {
    perl -n -i -e 'if(s/^#DIST: (.*)/$1/){ print; <>; } else { print; }' "$@"
}

mkdist Makefile system.mk
sed -i -e "s/de/de mod_xinerama mod_xrandr mod_xkbevents/" modulelist.mk

##
## Scripts
##

rm predist.sh
chmod a+x install-sh

cd ..
echo Creating notion-${release}.tar.gz
tar --exclude-vcs -czf notion-${release}-src.tar.gz $dir
echo Creating notion-${release}.tar.bz2
tar --exclude-vcs -cjf notion-${release}-src.tar.bz2 $dir
