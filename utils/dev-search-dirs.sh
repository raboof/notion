#!/bin/sh

this_rel=$(dirname "$0")
rel_root=$this_rel/..

notion_root=$(realpath $rel_root)

src_search_dirs() {
    prefix='-searchdir '
    ls -d \
       $notion_root/.notion \
       $notion_root/de \
       $notion_root/etc \
       $notion_root/ioncore \
       $notion_root/contrib/scripts \
       $notion_root/mod_statusbar/ion-statusd \
       $notion_root/mod_* \
        | sed "s/^/$prefix/" | xargs
}

src_search_dirs
