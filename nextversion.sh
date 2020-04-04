#!/bin/bash

function usage {
    echo "Usage: $0 [-M|-m|-p]"
    echo "    -M Major"
    echo "    -m Minor"
    echo "    -p patch [default]"
}

while getopts ":Mmp" opt; do
    case ${opt} in
        M )
            major=true
            ;;
        m )
            minor=true
            ;;
        p )
            patch=true
            ;;
        * )
            usage
            exit 1
            ;;
    esac
done

CURRENT_VERSION=$(git describe | cut -d- -f1)

if [[ ! $CURRENT_VERSION =~ ^[1-9][0-9]?\.[0-9]+\.[0-9]+$ ]]; then
  echo "Error: '${CURRENT_VERSION}' is not a valid semver format"
  exit 1
fi

CV=( ${CURRENT_VERSION//./ } )

if [ $major ]; then
    ((CV[0]++))
    CV[1]=0
    CV[2]=0
elif [ $minor ]; then
    ((CV[0]+=0))
    ((CV[1]++))
    CV[2]=0
else
    ((CV[0]+=0))
    ((CV[1]+=0))
    ((CV[2]++))
fi

echo "${CV[0]}.${CV[1]}.${CV[2]}"
