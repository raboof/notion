#!/usr/bin/perl

# Take the output of addr2line.sh, select only the 'Enter' calls and indent
# them to show the call hierarchy. The result can be browsed for example with
# vim and ':set foldmethod=indent'.

use strict;

my $currentIndent = 4;

while (<STDIN>) {
  if ($_ =~ /^Enter/) {
    print "\t" x $currentIndent ;
    print "$_";

    $currentIndent++;
  } else {
    $currentIndent--;
  }
}
