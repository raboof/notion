#!/bin/sh

tmp=`tempfile`

list_dir(){
	echo
	echo
	echo "== $1 =="
	
	dir=$2
	for file in ../$dir/*.c; do
		perl ../mkexports.pl -list $file > $tmp
		if test `wc -l $tmp|sed 's/\([0-9]\+\).*/\1/'` -gt 0; then
			echo
			echo "--`basename $file`--"
			echo
			sort $tmp|uniq
		fi
	done
}

list_dir "Ioncore" "ioncore"
list_dir "IonWS module" "ionws"
list_dir "FloatWS module" "floatws"
list_dir "Query module" "query"

rm $tmp
