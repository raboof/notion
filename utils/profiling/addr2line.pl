#!/usr/bin/perl

use strict;
use warnings;

my $executable = $ARGV[0];

my %lines = ();

sub getline {
  my $addr = shift;

  my $line = $lines{$addr};
  if (!defined($line)) {
    $line = `addr2line -f -e $executable $addr`;
    chomp($line);
    $line =~ s/\n/ /g;
    $lines{$addr} = $line;
  }
  return $line;
}

while (<STDIN> =~ /(\w) (\w+) (\w+) (\S+)/) {
  my ($action, $calledaddr, $calleraddr, $time) = ($1, $2, $3, $4);

  my $called = getline($calledaddr);
  my $caller = getline($calleraddr);

  print "$action $called at $time called from $caller\n";
}
