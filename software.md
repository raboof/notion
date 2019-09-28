---
layout: default
title: Software
---
# Supplemental Software

The intent of this page is to collect software that is known to integrate well
with a <b>Not</b>ion setup. Because notion is a rather minimalistic window
manager, not a desktop environment, other software is required for a full
working system.

Some tweaking may be required in order to fully integrate it with your setup.

## System tray

These trays can be used to add tray icon functionality either to Notion's integrated status bar (although we recommend using a panel, most of which include support for tray icons, see below).

* [stalonetray](http://stalonetray.sourceforge.net/) shows FreeDesktop (XEmbed) and KDE trayicons
* [trayion](https://code.google.com/archive/p/trayion/) shows FreeDesktop (XEmbed) trayicons (like Qt4 and gtk2), integrates well with <s>Not</s>ion's built-in status bar (mod\_statusbar), but seems abandoned. Can embed itself into mod\_statusbar.

## Panels

* [xfce4-panel](https://docs.xfce.org/xfce/xfce4-panel/start) — the classic panel from XFCE. Includes various widgets, most notably volume control, battery, clock, disk usage, system tray, application launcher.
* [polybar](https://polybar.github.io/) — A status bar written in C++, supporting modules and .ini-file customization. Plugins working with notion include volume control, clock, filesystem usage, keyboard layout, memory usage, cpu usage, battery, temperature

## Clipboard utilities

* [autocutsel](http://www.nongnu.org/autocutsel/) for keeping various clipboards synchronized
* [parcellite](http://parcellite.sourceforge.net/) more advanced clipboard management (with history)

## Notification daemons

* [notification-daemon](https://wiki.gnome.org/NotificationDaemon) shows only 1 notification at a time
* [notify-osd](https://launchpad.net/notify-osd) shows only 1 notification at a time
* [xfce4-notifyd](https://docs.xfce.org/apps/notifyd/start) shows a title bar
* [dunst](https://github.com/dunst-project/dunst/issues) — A customizable and lightweight notification-daemon, can show multiple notifications at a time by stacking them on top of the screen

## Program launchers and input handling

While notion comes with its own mod_query, some external tools have sprung up over the decades, which can be more easily used from shell-scripts to handle input. The most well-known of these are:

* [dmenu](https://tools.suckless.org/dmenu/) — very lightweight
* [rofi](https://github.com/davatorium/rofi) — self-proclaimed replacement for dmenu

## Other utilities

* [arandr](http://christian.amsuess.com/tools/arandr/) — a flexible tool for manipulating your monitor layout, resolutions, orientations

## Untested, please review

* [dzen2](https://wiki.archlinux.org/index.php/Dzen) — highly customizable status bar construction toolkit
* [xmobar](https://xmobar.org/) — status bar originating from xmonad
* [i3bar](https://i3wm.org/) — status bar from i3 (not to be confused with ion3)
* [lxpanel](https://github.com/lxde/lxpanel) — the official LXDE status bar

Generally, this document only serves as a list of links right now. If you feel like contributing a guide for any of these or other notion related tools because you have experience with them, please write a markdown document and send it to us (preferably via pull-request). Feel free to [contact](contact.html) us.
