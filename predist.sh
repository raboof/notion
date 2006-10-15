#!/bin/sh

do_darcs_export() {
	url="$1"
	project="$2"

	set -e
	
	! test -f "${project}" || { echo "${project} exists"; exit 1; }

	darcs get --partial "${url}" "${project}"
	darcs changes --context --repo="${project}" > "${project}/exact-version"
	rm -r "${project}/_darcs"
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
        perl -p -i -e "s/ION_VERSION/$release/" build/ac/configure.ac
    fi
fi


##
## Libs
##

getlib() {
    do_darcs_export $1 $2
    rm $2/build/rules.mk $2/system.mk
    ln -s ../build/rules.mk $2/build/rules.mk
    cat > $2/build/system-inc.mk << EOF
TOPDIR := \$(TOPDIR)/..
include \$(TOPDIR)/build/system-inc.mk
EOF

}

getlib http://modeemi.fi/~tuomov/repos/libtu-3 libtu
getlib http://modeemi.fi/~tuomov/repos/libextl-3 libextl

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
