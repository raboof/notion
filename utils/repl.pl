#!/usr/bin/perl

use strict;

print "lua> ";

while (my $command = <STDIN>) {
  if ($command =~ /^\s*([a-zA-Z_0-9]+)\s*=(.*)/) {
    $command = "$command; return $1;";
  } elsif ($command =~ /return/) {
    $command = $command;
  } else {
    $command = "return $command";
  }
  print `notionflux -e "$command"`;
  print "lua> ";
}
