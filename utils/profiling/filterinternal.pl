#!/usr/bin/perl

use strict;
use warnings;

while (<STDIN> =~ /(\w)\t([^\t]*)\t(.*)/) 
{
    my ($action, $called, $rest) = ($1, $2, $3);
    if ($called !~ /extl/ && $called !~ /exports.c/) {
        print "$1\t$2\t$3\n";
    }
}
