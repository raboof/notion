#!/usr/bin/perl -w

#
# Script to verify that the index.html is in good shape.
# This will verify that the entries:
#   1) Are in order per section
#   2) Exist on disk if in the index.html
#   3) Are in the index.html if they exist on disk
#
# Run it from the contrib directory.
#
# tyranix [ tyranix at gmail]
#
#
# verify_index.pl is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2.1 of the License, or
# (at your option) any later version.
#

use strict;
use warnings "all";
use diagnostics;

sub report_error($);

my $return_code = 0;   # Darcs needs a return code for a test suite.
my $line_count = 1;    # Line count inside index.html

{
    open(INDEX, "index.html") || die qq[Could not open "index.html": $!];

    my %index_files = ();        # Files found in "index.html"
    my %script_files = ();       # Files found in the directory
    my %old_ignored_files = ();  # Files marked for ion2 only
    my $section_counter = 1;     # Section number (not same as line number)

    for (my $line = <INDEX>; $line; $line = <INDEX>, $line_count++) {
        chomp($line);

        if ($line =~ m/^\s*<dt>\s*<a\s*href="([^\"]+)">([^<]+)<\/a>/) {
            my ($url, $name) = ($1, $2);
            my ($directory, $filename) = $url =~ m,(.+)/([^/]+),;

            if (!defined($directory) || !defined($filename)) {
                report_error("Malformed entry on line $line_count\n");
            }

            if ($filename ne $name) {
                report_error(qq["$name" should be named "$filename"]);
            }

            # Reset the section_counter and setup the hashes.
            if (!exists($index_files{$directory})) {
                $section_counter = 1;
                $index_files{$directory} = {};
                $script_files{$directory} = {};

                # Add all of the actual files so we can compare the two hashes.
                opendir(FILES, $directory)
                    || die qq[Could not open "$directory": $!];

                while (my $entry = readdir(FILES)) {
                    if ($entry !~ /^\./ && -f "$directory/$entry") {
                        ${$script_files{$directory}}{$entry} = 1;
                    }
                }
                closedir(FILES) || die qq[Could not close "$directory": $!];
            }

            if (exists(${$index_files{$directory}}{$filename})) {
                report_error("Duplicate entry $directory/$filename");
            }

            # Add this file to the hash of this directory for later use.
            ${$index_files{$directory}}{$filename} = $section_counter++;
        } elsif ($line =~ m/^\s*<dt>\s*<strike>\s*<a\s*href="([^\"]+)">([^<]+)<\/a>/) {
            my ($url, $name) = ($1, $2);
            my ($directory, $filename) = $url =~ m,(.+)/([^/]+),;

            if (!defined($directory) || !defined($filename)) {
                report_error("Malformed entry on line $line_count\n");
            }

            if ($filename ne $name) {
                report_error(qq["$name" should be named "$filename"]);
            }

            if (!exists($old_ignored_files{$directory})) {
                $old_ignored_files{$directory} = {};
            }

            if (exists(${$old_ignored_files{$directory}}{$filename})) {
                report_error("Duplicate entry $directory/$filename");
            }

            ${$old_ignored_files{$directory}}{$filename} = 1;
        } elsif ($line =~ m/^\s*<dt>/) {
            # This is a dt entry but it's invalid.
            report_error("Invalid line: $line");
        }
    }
    close(INDEX) || die qq[Could not close "index.html": $!];

    my @out_of_order = ();
    my @not_on_disk = ();

    # Compare the index.html entries to what's on disk.
    for my $dir (keys %index_files) {
        my $count = 1;
        for my $key (sort(keys %{$index_files{$dir}})) {
            if (!exists(${$script_files{$dir}}{$key})) {
                push(@not_on_disk, "$dir/$key");
            } else {
                if ($count != ${$index_files{$dir}}{$key}) {
                    push(@out_of_order, ["$dir/$key", $count, ${$index_files{$dir}}{$key}]);
                }

                # Delete it so we can go through it later and see what wasn't
                # in the html file.
                delete(${$script_files{$dir}}{$key});
            }
            $count++;
        }
    }

    if ($#not_on_disk != -1) {
        $return_code = 1;
        print "Files in the index.html but not on disk:\n";
        for my $key (@not_on_disk) {
            print "\t$key\n";
        }
    }

    if ($#out_of_order != -1) {
        $return_code = 1;
        print  "Files out of order in index.html:\n";
        for my $key (@out_of_order) {
            my ($name, $correct_value, $value_saw) = @{$key};
            print "\t$name \tshould be in spot $correct_value but found in $value_saw\n";
        }
    }

    my $printed_header = undef;
    for my $dir (keys %script_files) {
        for my $key (sort { uc($a) cmp uc($b) } keys %{$script_files{$dir}}) {
            if (!exists(${$old_ignored_files{$dir}}{$key})) {
                if (!defined($printed_header)) {
                    print "Files on the disk but not in index.html:\n";
                    $printed_header = 1;
                    $return_code = 1;
                }
                print "\t$dir/$key\n";
            }
        }
    }

    exit($return_code);
}

sub report_error($)
{
    my ($error) = @_;

    print STDERR qq[$0: Error in "index.html" on line $line_count\n\t$error\n];
    $return_code = 1;
}
