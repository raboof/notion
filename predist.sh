#!/bin/sh

##
## Versioning
##

if test "$1" != "-snapshot"; then
    pwd=`pwd`
    dir=`basename "$pwd"`

    release=`echo "$dir"|sed 's/^[^-]\+-\([^-]\+-[0-9]\+\)$/\1/p; d'`

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
    svn export -q http://tao.uab.es/ion/svn/$1/trunk $1
    rm $1/rules.mk $1/system.mk
    ln -s ../rules.mk $1/rules.mk
    cat > $1/system-inc.mk << EOF
TOPDIR := \$(TOPDIR)/..
include \$(TOPDIR)/system-inc.mk
EOF

}

getlib libtu
getlib libextl

##
## Makefiles
##

mkdist() {
    perl -n -i -e 'if(s/^#DIST: (.*)/$1/){ print; <>; } else { print; }' "$@"
}

mkdist Makefile system.mk
mv libs.mk.dist libs.mk

##
## Autoconf
##

autoconf
