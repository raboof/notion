#!/usr/bin/env bash

set -e

this_rel=$(dirname ${BASH_SOURCE[0]})
rel_root=$this_rel/../..

notion_root=$(realpath $rel_root)

docker build -f Dockerfile.make -t notion $notion_root
docker run --rm -it --name notion-test -v /tmp/.X11-unix:/tmp/.X11-unix notion

