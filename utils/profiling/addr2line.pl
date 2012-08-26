#!/usr/bin/perl

use strict;
use warnings;

my $executable = $ARGV[0];

my %lines = ();
my %functionNames = ();

sub getline {
  my $addr = shift;

  if ($addr eq "") {
    return "?? ??:0";
  }
  if ($addr !~ /^0x/) {
    return $addr;
  }

  my $line = $lines{$addr};
  if (!defined($line)) {
    $line = `addr2line -f -e $executable $addr`;
    chomp($line);
    $line =~ s/\n/ /g;
    $lines{$addr} = $line;
  }
  return $line;
}

while (<STDIN> =~ /(\w)\t([^\t]+)\t([^t]*)\t(\S+)/) {
  my ($action, $calledaddr, $calleraddr, $time) = ($1, $2, $3, $4);

  my $called = getline($calledaddr);
  my $caller = getline($calleraddr);

  if ($called =~ /^\(null\) (.*)/) {
    if (defined $functionNames{$1}) {
       $called = "$functionNames{$1} $1";
    } else {
       $called = "$1 $1";
    }
  }
  elsif ($called =~ /^(\S+) (.*)/) {
    $functionNames{$2} = $1;
  }

  print "$action\t$called\t$caller\t$time\n";
}
