#!/usr/bin/perl

use strict; 
use warnings;

my @stack = ();

sub firstfield {
  my $arg = shift;
  if (!defined($arg)) { return undef; }
  $arg =~ /([^\t]*)/;
  return $1;
}

while (<STDIN> =~ /(\w)\t([^\t]*)\t(.*)/) {
  my ($action, $called, $rest) = ($1, $2, $3);
  if ($action eq "e") {
    push(@stack, "$called\t$rest");
    print "e\t$called\t$rest\n";
  }
  elsif ($action eq "x") {
    my $parent = pop(@stack);
    if (defined $parent) {
      my $parentcalled = firstfield($parent);
      while (defined $parent and $parentcalled ne $called) {
        print "x\t$parentcalled\t$rest\n";
        $parent = pop(@stack);
        $parentcalled = firstfield($parent);
      }
    }
    print "x\t$called\t$rest\n";
  }
}
