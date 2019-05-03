Notion
======

[![Build Status](https://travis-ci.org/raboof/notion.svg?branch=master)](https://travis-ci.org/raboof/notion)

Copyright (c) the Notion team 2010-2019.
Copyright (c) Tuomo Valkonen 1999-2009.

https://notionwm.net
          

Building and installing
-----------------------

1. Get the source code.

```
git clone https://github.com/raboof/notion
```

2. In addition to the standard C library headers and the GNU toolchain, you 
   will need the following tools and libraries for building Notion.

    * Lua interpreter and header files, 5.1 or later <http://www.lua.org/>
    * Xlib header files <http://cgit.freedesktop.org/xorg/lib/libX11/>
    * libXext header files <http://cgit.freedesktop.org/xorg/lib/libXext/>
    * libSM header files <http://cgit.freedesktop.org/xorg/lib/libSM/>
    * gettext <http://www.gnu.org/software/gettext/> 

   If you want to build the mod_xinerama and mod_xrandr module, which provide 
   enhanced multihead support, you will further need the following libraries.

    * Xinerama header files <https://sourceforge.net/projects/xinerama/>
    * XRandR header files <http://www.x.org/wiki/Projects/XRandR/> 

   If you want to enable support for Xft fonts, you need libxft:

    * libxft <https://gitlab.freedesktop.org/xorg/lib/libxft>

   On a Debian based system, these dependencies are provided by the following
   packages.
     build-essential lua5.1 liblua5.1-0-dev libx11-dev libxext-dev libsm-dev gettext
     libxinerama-dev libxrandr-dev libxft-dev

3. If the default build settings don't suit you, review system-autodetect.mk
   and either override values from the environment or in a newly added
   system-local.mk or make changes directly to system-autodetect.mk.

4. If you want to build some extra modules now or do not want to build
   some of the standard modules, edit `modulelist.mk`.
   
5. Run `make`. Note that `make` here refers to GNU make which is usually
   named `gmake` on systems with some other implementation of make as 
   default.
   
6. Run `make install`, as root if you set `$PREFIX` in `system.mk` to a 
   directory that requires those privileges.
   
   YOU SHOULD NOT SKIP THIS STEP unless you know what you are doing. Notion
   will refuse to start if it can not find all the necessary uncorrupt
   configuration files either in `$PREFIX/etc/notion/` or in `~/.notion/`.

7. How to best set up `startx` or whatever to start Notion instead of your
   current window manager depends on your system's setup. A good guess
   is creating or modifying an executable shell script `.xsession` in your
   home directory to start Notion. This should usually (but not always) work
   if you're using some X display/login manager. If `~/.xsession` does not 
   help and you're not using a display manager, modifying `~/.xinitrc` or 
   creating one based on your system's `xinitrc` (wherever that may be; 
   use `locate`) may be what you need to do. Note that unlike `.xsession`, 
   a `.xinitrc` should usually do much more setup than simply start a few
   programs of your choice.


Some optional installation steps
--------------------------------

1. The F5 and F6 keys expect to find the program `run-mailcap` to select
   a program to view a file based on its guessed MIME type. Unless you are
   using Debian, most likely you don't have it, but any other similar 
   program (or just plain old text editor) will do as well -- just modify the
   bindings in `cfg_notioncore.lua`. Of course, if you don't want to use the 
   feature at this time or never, you may simply skip this step. If you want
   to use `run-mailcap`, it can be found from the following address, as a 
   source tarball as well:
   
       <http://www.debian.org/Packages/unstable/net/mime-support.html>

2. Notion supports caching known man-pages in a file for faster man-page
   completion in the F1 man page query. To enable this feature, you must
   periodically run a cronjob to build this list. To create a system-wide
   man page cache, run `crontab -e` (might vary depending on platform) as
   root and enter a line such as follows:

        15 05 * * * $SHAREDIR/ion-completeman -mksyscache

   Replace `$SHAREDIR` with the setting from `system.mk`. This example 
   runs daily at 05:15, but you may modify the  run times to your needs;
   see the crontab manual. 
   
   If you can't or do not want to build a system-wide man page cache, run
   `crontab -e` as your normal user and replace `-mksyscache` with
   `-mkusercache` above. The cache file will be `~/.notion/mancache`.
   
   It may also be useful to run `ion-completeman` with the suitable
   `-mk*cache` argument once manually to build the initial cache.
   
   If the `MANPATH` environment variable is not set on your system and it
   does not have the `manpath` command (or it does not print anything 
   sensible), you may also want to set the `ION_MANPATH` environment
   variable to the list of paths where the system stores manual pages.


Configuration
-------------

For help on modifying Notion's configuration files, PLEASE READ THE DOCUMENT
"Configuring and extending Notion with Lua" available from the Notion web page,
listed at the top of this file.


Questions, comments, problems?
------------------------------

If the available documentation does not answer your question, please
join our IRC channel, [#notion on freenode](https://webchat.freenode.net/?channels=#notion).

Contributing
------------

Contributions to Notion are very welcome! Please join us on [GitHub](https://github.com/raboof/notion/).
The [Notes for the module and patch writer](http://notion.sourceforge.net/notionnotes/) might be helpful.

Credits
-------

Notion was written by the Notion team, based on Ion which was written by Tuomo
Valkonen.

The dock module was written by Tom Payne and Per Olofsson.

`utils/ion-completefile/ion-completefile.c` is based on editline, (c)
1992 Simmule Turner and Rich Salz. See the file for details.

The code that `de/fontset.c` is based on seems to have been originally
written by Tomohiro Kubota, but see the file for details.

Various (minor) patches have been contributed by other individuals 
unlisted  here. See the mailing list archives and the darcs source 
repository history (previously at <http://iki.fi/tuomov/repos/>).
For translators see the individual `.po` files in `po/`.

The code in `de/unicode' (producing `de/precompose.c') is taken from
xterm.

See `libtu/README` for code by others integrated into libtu.
