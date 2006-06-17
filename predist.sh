#!/bin/sh

do_darcs_export() {
	url="$1"
	project="$2"

	set -e
	
	! test -f "${project}" || { echo "${project} exists"; exit 1; }
	! test -f "${project}.tmp" || { echo "${project}.tmp exists"; exit 1; }

	darcs get --partial "${url}" "${project}.tmp"
	darcs changes --context --repo="${project}.tmp" > "${project}.context"
	mv "${project}.tmp/_darcs/current" "${project}"
	mv "${project}.context" "${project}/exact-version"
	rm -r "${project}.tmp"
}

##
## Versioning
##

if test "$1" != "-snapshot"; then
    pwd=`pwd`
    dir=`basename "$pwd"`

    release=`echo "$dir"|sed 's/^[^-]\+-\([^-]\+-[0-9]\+\(-[0-9]\+\)\?\)$/\1/p; d'`

    if test "$release" == ""; then
        echo "Invalid package name $dir."
        exit 1
    else
        versdef="#define ION_VERSION \"${release}\""
        perl -p -i -e "s/^#define ION_VERSION.*/$versdef/" version.h
        perl -p -i -e "s/ION_VERSION/$release/" configure.ac
    fi
fi


##
## Libs
##

getlib() {
    do_darcs_export $1 $2
    rm $2/rules.mk $2/system.mk
    ln -s ../rules.mk $2/rules.mk
    cat > $2/system-inc.mk << EOF
TOPDIR := \$(TOPDIR)/..
include \$(TOPDIR)/system-inc.mk
EOF

}

getlib http://iki.fi/tuomov/repos/libtu-3 libtu
getlib http://iki.fi/tuomov/repos/libextl-3 libextl

##
## Makefiles
##

mkdist() {
    perl -n -i -e 'if(s/^#DIST: (.*)/$1/){ print; <>; } else { print; }' "$@"
}

mkdist Makefile system.mk
mv build/libs.mk.dist build/libs.mk

##
## Autoconf
##

#autoconf
#rm -rf autom4te.cache

##
## Scripts
##

rm predist.sh
chmod a+x install-sh
