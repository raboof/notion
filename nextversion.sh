#!/bin/sh

PREFIX=3-`date +"%Y%m%d"`

alreadyTagged() {
  #nfound = git tag -l | grep $1 | wc
  return 0;
}

for minor in 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20
do
  VERSION=$PREFIX$minor
  if [ "0" = "`git tag -l | grep $VERSION | wc -l`" ]
  then
    echo $PREFIX$minor;
    exit 1
  fi
done
