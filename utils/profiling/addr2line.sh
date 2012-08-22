#!/bin/sh

# Make a profiling trace (somewhat) more human-readable
#
# Usage: addr2line.sh <executable> <tracelog>

if test ! -f "$1"
then
 echo "Error: executable $1 does not exist."
 exit 1
fi
if test ! -f "$2"
then
 echo "Error: trace log $2 does not exist."
 exit 1
fi
EXECUTABLE="$1"
TRACELOG="$2"
while read LINETYPE FADDR CADDR CTIME; do
 FNAME="$(addr2line -f -e ${EXECUTABLE} ${FADDR}|head -1)"
# CDATE="$(date -Iseconds -d @${CTIME})"
 if test "${LINETYPE}" = "e"
 then
  CNAME="$(addr2line -f -e ${EXECUTABLE} ${CADDR}|head -1)"
  CLINE="$(addr2line -s -e ${EXECUTABLE} ${CADDR})"
  echo "Enter ${FNAME} at ${CTIME}, called from ${CNAME} (${CLINE})"
 else if test "${LINETYPE}" = "x"
 then
  echo "Exit  ${FNAME} at ${CTIME}"
 fi
 fi
done < "${TRACELOG}"
