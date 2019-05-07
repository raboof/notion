#!/usr/bin/perl

use strict;
use warnings;

my $executablename = $ARGV[0];
my %entries = ();

print "events: Microseconds\n";

my @msAlreadyAccountedFor = (0,0,0,0,0,0,0);
my @callstack = ();

my %callframe = ();
$callframe{"function"} = "toplevel";
$callframe{"file"} = "toplevel";
$callframe{"line"} = 0;
push(@callstack, \%callframe);

while (<STDIN> =~ /^([ex])\t(\S+) ([^:]+):(\d+)\t(\S+) ([^:]+):(\d+)\t(\d+)\.(\d{6})\d*/) {
  my ($action, $calledfunction, $calledfile, $calledline,
	$callerfunction, $callerfile, $callerline, $sec, $msec) = ($1, $2, $3, $4, $5, $6, $7, $8, $9);
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
      if ($timespentself < 0) {
          print STDERR "Spent negative amount of time in $calledfunction ($entry-$sec$msec=$timespentinclusive minus $timespentinchildren is $timespentself) !?\n";
          exit 1;
      }
      #print "Time spent in $calledfunction (inclusive): $timespentinclusive\n";
      my $accountedForSoFar = $msAlreadyAccountedFor[-1];
      my $accountedFor = $accountedForSoFar + $timespentinclusive;
      $msAlreadyAccountedFor[-1] = $accountedFor;
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
        print "calls=1 $calledline\n$callerline $timespentinclusive\n";

        #print "Time spent in children of $callerfunction so far: $accountedForSoFar + $timespentinclusive = $accountedFor\n";
      }
    }
  }
}
