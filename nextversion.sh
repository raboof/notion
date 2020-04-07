#!/usr/bin/env bash

function usage {
    echo "Usage: $0 [major|minor|patch]"
    echo "     defaults to patch"
}


CURRENT_VERSION=$(git describe | cut -d- -f1)

if [[ ! $CURRENT_VERSION =~ ^[1-9][0-9]?\.[0-9]+\.[0-9]+$ ]]; then
  (
    echo "Error: '${CURRENT_VERSION}' is not a valid version format";
    echo "";
    usage
  ) 1>&2
  exit 1
fi

CV=( ${CURRENT_VERSION//./ } )

ARG=${1:-patch}
if [[ "${ARG}" == "major"* ]]; then
    ((CV[0]++))
    CV[1]=0
    CV[2]=0
elif [[ "${ARG}" == "minor"* ]]; then
    ((CV[0]+=0))
    ((CV[1]++))
    CV[2]=0
elif [[ "${ARG}" == "patch"* ]]; then
    ((CV[0]+=0))
    ((CV[1]+=0))
    ((CV[2]++))
else
    (
      echo "Unknown argument '${ARG}'";
      echo "";
      usage;
    ) 1>&2
    exit 1
fi

echo "${CV[0]}.${CV[1]}.${CV[2]}"
