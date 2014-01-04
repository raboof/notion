#!/bin/sh

cat index.html \
	| sed -e 's/href="scripts\/\([^"]*\)"/href="gitweb.cgi\?p=notion\/contrib;a=blob_plain;f=scripts\/\1;hb=HEAD"/g' \
	| sed -e 's/href="keybindings\/\([^"]*\)"/href="gitweb.cgi\?p=notion\/contrib;a=blob_plain;f=keybindings\/\1;hb=HEAD"/g' \
	| sed -e 's/href="styles\/\([^"]*\)"/href="gitweb.cgi\?p=notion\/contrib;a=blob_plain;f=styles\/\1;hb=HEAD"/g' \
	| sed -e 's/href="statusbar\/\([^"]*\)"/href="gitweb.cgi\?p=notion\/contrib;a=blob_plain;f=statusbar\/\1;hb=HEAD"/g' \
	| sed -e 's/href="statusd\/\([^"]*\)"/href="gitweb.cgi\?p=notion\/contrib;a=blob_plain;f=statusd\/\1;hb=HEAD"/g' \
	> index_git.html
