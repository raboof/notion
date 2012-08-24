#!/usr/bin/perl

use strict;
use warnings;

my $executablename = $ARGV[0];
my %entries = ();

print "events: Milliseconds\n";

my @msAlreadyAccountedFor = (0,0,0,0);
my @callstack = ();

while (<STDIN> =~ /^(\w+)\s+(\S+) ([^:]+):(\d+) at (\d+)\.(\d{5})\d* called from (\S+) ([^:]+):(\d+)/) {
  my ($action, $calledfunction, $calledfile, $calledline, $sec, $msec,
	$callerfunction, $callerfile, $callerline) = ($1, $2, $3, $4, $5, $6, $7, $8, $9);
  my $callkey = "$calledfile$calledline $callerfile$callerline";
  my $entryArrayRef = $entries{$callkey};
  if ($action eq "e") {
    push(@msAlreadyAccountedFor, 0);
    
    my %callframe = ();
    $callframe{"function"} = $calledfunction;
    $callframe{"file"} = $calledfile;
    $callframe{"line"} = $calledline;
    push(@callstack, \%callframe);

    if (!defined($entryArrayRef)) {
      $entryArrayRef = [];
      $entries{$callkey} = $entryArrayRef;
    }
    push(@$entryArrayRef, "$sec$msec");
  }
  if ($action eq "x") {
    if(defined($entryArrayRef) && (scalar(@$entryArrayRef) > 0)) {
      my $entry = pop(@$entryArrayRef);
      
      # Record the 'own time' spent inside the function we're exiting
      print "fl=$calledfile\nfn=$calledfunction\n";
      my $timespentinchildren = pop(@msAlreadyAccountedFor);
      my $timespentinclusive  = "$sec$msec" - $entry; 
      my $timespentself = $timespentinclusive - $timespentinchildren;
      $msAlreadyAccountedFor[-1] = $msAlreadyAccountedFor[-1] + $timespentinclusive;
      print "$calledline $timespentself\n";

      # Record the fact that the parent function called the function we're exiting
      pop(@callstack);
      if (scalar(@callstack) > 0) {
        my %parentframe = %{$callstack[-1]};
        my $callerfile = $parentframe{"file"};
        my $callerfunction = $parentframe{"function"};
        my $callerline = $parentframe{"line"};
        print "fl=$callerfile\nfn=$callerfunction\n";
        print "cfl=$calledfile\ncfn=$calledfunction\n";
        print "calls=1 $calledline\n$callerline $timespentinclusive\n"
      }
    }
  }
}
