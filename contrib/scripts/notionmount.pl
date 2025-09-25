#!/usr/bin/perl -ls

# Copyright (c) 2025 Matthias S. Benkmann
# You may do everything with this code except misrepresent its origin.
# PROVIDED `AS IS' WITH ABSOLUTELY NO WARRANTY OF ANY KIND!

# For use with scripted_dynamic_menu.lua: Creates menu entries to un/mount removable
# drives like USB flash drives. You add it to a Notion menu like this:
#
#   submenu("USB", function() return cmd2menu('notionmount.pl -menu') end),
#
# The submenu will look like this
#
#  [_] Drive label 1
#  [X] Drive label 2
#
# The entries with [_] are unmounted and choosing them will mount the drive.
# The entries with [X] are mounted and choosing them will unmount the drive.
#
# Mounting and unmounting is performed with pmount/pumount, so they must be installed.
#
# The script will beep after successful un/mount.
# To hear these beeps you must have a program "beep" in your path. Ubuntu
# has a package "beep" for instance, that installs such a program.
#
# If you want to know when a device is ready to be mounted (because that takes some time
# after plugging it in), you could create a file
# /etc/udev/rules.d/99-usb-beep.rules with the following single line:
#
# ACTION=="add", SUBSYSTEM=="block", KERNEL=="sd*[0-9]", SUBSYSTEMS=="usb", RUN="/lib/udev/beep"
#
# and a shell script /lib/udev/beep with the following contents:
# #!/bin/sh
# echo $'\a' >/dev/console


use strict;
use warnings;

use Cwd;

#main()
{
our ($help, $mount, $unmount, $menu);
my $USAGE="Usage: \n$0 -menu\n" .
"    generates ouput for Notion WM scripted_dynamic_menu.lua with un/mount commands for USB devices.\n".
"$0 -mount <device> <label>\n" .
"    mounts <device> via pmount on /dev/<label>. Beeps if successful\n" .
"$0 -unmount <device>\n" .
"    attempts to unmount <device> via pumount. Beeps if successful.";

if (defined($help))
{
  print ($USAGE);
  return 0;
}
elsif (defined($menu))
{
  (scalar(@ARGV) == 0) or error($USAGE);
  menu();
}
elsif (defined($mount))
{
  (scalar(@ARGV) == 2) or error($USAGE);
  mount();
}
elsif (defined($unmount))
{
  (scalar(@ARGV) == 1) or error($USAGE);
  unmount();
}
else 
{
  error($USAGE);
}

} # end main()

sub menu
{
  # devmap maps a real device name such as "sda" to the alias to be used for
  # presenting the device to the user. Entries start out with the identity
  # mapping and are later overwritten with entries from /dev/disk/by-id if present
  # and even later with entries from /dev/disk/by-label if present.
  my %devmap=();
  
  foreach my $dir (</sys/block/sd[a-z]>)
  {
    chdir($dir)  or error("Cannot chdir into '$dir': $!");
    my $devicedir = readlink("$dir/device")  or print STDERR "Can't readlink '$dir/device': $!";
    open(my $removable, "<", "$dir/removable") or next;
    my $is_removable = 0;
    read($removable, $is_removable, 1); # we're only interested in removable devices
    open(my $sizefile, "<", "$dir/size");
    read($sizefile, my $is_present, 10);
    chomp $is_present;
    $is_present = ($is_present ne "0" ); # skip e.g. card-readers with no card inside
    ($is_removable and $is_present) or next;
    $devicedir = Cwd::realpath($devicedir);
    
    my $havepartitions = 0;
    foreach my $subdir (<$dir/sd[a-z]*>)
    {
      $havepartitions = 1;
      (my $devname) = ($subdir =~ m%/(sd[^/]+)$%);
      $devmap{$devname} = $devname;
    }

    if (not $havepartitions)
    {
      (my $devname) = ($dir =~ m%/(sd[^/]+)$%);
      $devmap{$devname} = $devname;
    }
  }
  
  foreach my $dev (</dev/disk/by-id/usb-*>)
  {
    (my $devname) = ($dev =~ m%/usb-([^/]+)$%);
    my $real_dev = readlink($dev);
    (my $real_devname) = ($real_dev =~ m%/(sd[^/]+)$%);
    if (defined($real_devname) and exists($devmap{$real_devname}))
    {
      $devmap{$real_devname} = $devname;
    }
  }
  
  foreach my $dev (</dev/disk/by-label/*>)
  {
    (my $devname) = ($dev =~ m%/([^/]+)$%);
    my $real_dev = readlink($dev);
    (my $real_devname) = ($real_dev =~ m%/(sd[^/]+)$%);
    if (defined($real_devname) and exists($devmap{$real_devname}))     
    {
      $devmap{$real_devname} = $devname;
    }
  }
  
  # Entries for devices currently mounted will be moved from %devmap here
  my %mounted_devmap;
  open(my $in, "<", "/proc/mounts") or error("Error opening \"/proc/mounts\": $!");
  while (<$in>)
  {
    if (m{^/dev/(sd[^/]+) /media})
    {
      my $devname = $1;
      if (exists($devmap{$devname}))
      {
        $mounted_devmap{$devname} = $devmap{$devname};
        delete $devmap{$devname};
      }
    }
  }
 
  foreach my $dev (keys %mounted_devmap)
  {
    print "[X] $mounted_devmap{$dev}\0ioncore.exec('$0 -unmount /dev/$dev')";
  }
  
  foreach my $dev (keys %devmap)
  {
    print "[_] $devmap{$dev}\0ioncore.exec('$0 -mount /dev/$dev $devmap{$dev}')";
  }
  
  
  return 0;
}

sub mount
{
  if (exe("pmount", "pmount", $ARGV[0], $ARGV[1]) == 0)
  {
    exe("beep","beep");
  }
}

sub unmount
{
  exe("sync", "sync");
  if (exe("pumount", "pumount", $ARGV[0]) == 0)
  {
    exe("beep","beep");
  }
}

# exe($program, @args)
# $args[0] is passed to the program as program name.
# Returns the exit code of the program (the real exit code,
# not the code that includes signals etc. as $? contains after system()).
# This means that if a C program terminates with exit(3), this
# function would return 3 if used to execute that C program.
# Aborts with an error if the program cannot be run.
sub exe
{
  my $program = shift;
  $? = 0;
  system({$program} @_);
  $? >= 0 or error("Could not execute \"$program\" ($!)");
  return ($? >> 8);
}

sub error
{
  print STDERR @_;
  exit 1;
}
