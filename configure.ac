dnl Process this file with autoconf to produce a configure script.
dnl Please report bugs in this autoconf script Tom Payne <ion@tompayne.org>
dnl $Header: /home/twp/cvsroot/twp/ion/ion-devel-autoconf/configure.ac,v 1.16 2004/01/14 11:32:19 twp Exp $

AC_INIT([Ion Window Manager],
		[ION_VERSION],
		[tuomov@iki.fi],
		[ion])
AC_PREREQ([2.58])
AC_REVISION([$Revision: 1.16 $])

dnl Checks for programs. {{{
AC_PROG_CC()
AC_PROG_EGREP()
AC_PROG_INSTALL()
AC_PROG_RANLIB()
AC_CHECK_TOOL([STRIP], [strip], [:])
AC_CHECK_TOOL([AR], [ar], [])
dnl }}}

dnl Checks for libraries. {{{
AC_PATH_XTRA()
test "${X_DISPLAY_MISSING+set}" == set && exit 1
dnl }}}

dnl XINERAMA_LIBS, _DCF_NO_XINERAMA {{{

AC_SUBST([XINERAMA_LIBS])
AC_SUBST([_DCF_NO_XINERAMA])

AC_ARG_ENABLE([xinerama],
			  [AS_HELP_STRING([--disable-xinerama],
							  [Disable Xinerama support])])

if test "x$enable_xinerama" != xno; then
	AC_CHECK_LIB([Xext],
				 [XMissingExtension],
				 [X_LIBS="$X_LIBS -lXext"],
				 [exit 1],
				 [$X_PRE_LIBS -lX11 $X_LIBS $X_EXTRA_LIBS])
	AC_CHECK_LIB([Xinerama],
				 [XineramaQueryExtension],
				 [XINERAMA_LIBS="-lXinerama"],
				 [exit 1],
				 [$X_PRE_LIBS -lX11 $X_LIBS $X_EXTRA_LIBS])
else
	_DCF_NO_XINERAMA="-DCF_NO_XINERAMA"
fi

dnl }}}

dnl _DCF_XFREE86_TEXTPROP_BUG_WORKAROUND {{{

AC_SUBST([_DCF_XFREE86_TEXTPROP_BUG_WORKAROUND])

AC_ARG_ENABLE([xfree86-textprop-bug-workaround],
			  [AS_HELP_STRING([--disable-xfree86-textprop-bug-workaround],
							  [Disable the XFree86 textprop bug workaround])])

if test "x$enable_xfree86_textprop_bug_workaround" != xno; then
	_DCF_XFREE86_TEXTPROP_BUG_WORKAROUND="-DCF_XFREE86_TEXTPROP_BUG_WORKAROUND"
fi

dnl }}}

dnl _DCF_DE_USE_XUTF8 {{{

AC_SUBST([_DCF_DE_USE_XUTF8])

AC_ARG_ENABLE([Xutf8],
			  [AS_HELP_STRING([--enable-Xutf8],
							  [Use the Xutf8 routines (XFree86 extension)
							   instead of Xmb routines in an UTF-8 locale given
							   the -i18n command line option])])

if test "x$enable_Xutf8" == xyes; then
	AC_MSG_CHECKING([for Xutf8*])
	save_CFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS $X_CFLAGS"
	AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <X11/Xlib.h>

int main()
{
#ifdef X_HAVE_UTF8_STRING
	exit(0);
#else
	exit(1);
#endif
}
                  ]])],
				  [AC_MSG_RESULT([yes])
				   _DCF_DE_USE_XUTF8="-DCF_DE_USE_XUTF8"],
				  [AC_MSG_RESULT([no])
				   exit 1])
	CFLAGS="$save_CFLAGS"
fi

dnl }}}

dnl LIBTOOL, LTDL_INCLUDES, LTDL_LIBS {{{

AC_SUBST([LTDL_INCLUDES])
AC_SUBST([LTDL_LIBS])

AC_ARG_WITH([libtool],
			[AS_HELP_STRING([--with-libtool=DIR],
							[Libtool files are in DIR])])

if test "x$with_libtool" != x; then
	LTDL_INCLUDES="-I$with_libtool/include"
	LTDL_LIBS="-L$with_libtool/lib"
	libtool_path="$with_libtool/bin"
else
	libtool_path="$PATH"
fi
AC_PATH_PROG([LIBTOOL], [libtool], [], [$libtool_path])
test "x$LIBTOOL" == x && exit 1
AC_MSG_CHECKING([for libtool version >= 1.4.0])
libtool_version=$($LIBTOOL --version | head -n 1 | cut -d' ' -f4)
dnl libtool_version=$($LIBTOOL --version | $EGREP -o '[0-9]+(\.[0-9]+)+' | head -n 1)
case $libtool_version in
0.* | 1.0.* | 1.1.* | 1.2.* | 1.3.*)
	AC_MSG_RESULT([no (found $libtool_version)])
	exit 1
	;;
*)
	AC_MSG_RESULT([yes (found $libtool_version)])
	;;
esac

AC_CHECK_HEADER([ltdl.h], [], [exit 1], [$LDTL_INCLUDES])
AC_CHECK_LIB([ltdl], [lt_dlinit], [LTDL_LIBS="-lltdl"], [exit 1], [$LTDL_LIBS])

dnl }}}

dnl PRELOAD_MODULES {{{

AC_SUBST(PRELOAD_MODULES)

AC_ARG_ENABLE([shared],
			  [AS_HELP_STRING([--disable-shared],
							  [Do not dynamically load modules (preload modules
							   instead)])])

if test "x$enable_shared" == xno; then
	PRELOAD_MODULES="1"
fi

dnl }}}

dnl LUA, LUAC, LUA_INCLUDES, LUA_LIBS {{{

AC_SUBST([LUA])
AC_SUBST([LUAC])
AC_SUBST([LUA_INCLUDES])
AC_SUBST([LUA_LIBS])

dnl Arguments {{{
AC_ARG_WITH([lua-prefix],
			[AS_HELP_STRING([--with-lua-prefix=DIR],
							[Lua files are in DIR])])
AC_ARG_WITH([lua-includes],
			[AS_HELP_STRING([--with-lua-includes=DIR],
							[Lua include files are in DIR])])
AC_ARG_WITH([lua-libraries],
			[AS_HELP_STRING([--with-lua-libraries=DIR],
							[Lua library files are in DIR])])
AC_ARG_WITH([lua-suffix],
			[AS_HELP_STRING([--with-lua-suffix=ARG],
							[Lua binary and library files are suffixed with
							 ARG])])
dnl }}}

dnl LUA {{{
if test "x$with_lua_prefix" == x; then
	lua_search_path="$PATH"
else
	lua_search_path="$with_lua_prefix/bin"
fi
if test "x$LUA" == x; then
	AC_PATH_PROG([LUA], [lua$with_lua_suffix], [], [$lua_search_path])
	test "x$LUA" == x && exit 1
fi
dnl }}}

dnl lua_version {{{
AC_MSG_CHECKING([for lua version >= 5.0])
lua_version=$($LUA -v 2>&1 | head -n 1 | cut -d' ' -f2)
dnl lua_version=$($LUA -v 2>&1 >/dev/null | head -n 1 | $EGREP -o '[0-9]+(\.[0-9]+)+')
case $lua_version in
0.* | 1.* | 2.* | 3.* | 4.*)
	AC_MSG_RESULT([no (found $lua_version)])
	exit 1
	;;
*)
	AC_MSG_RESULT([yes (found $lua_version)])
	;;
esac
dnl }}}

dnl LUAC {{{
if test "x$LUAC" == x; then
	AC_PATH_PROG([LUAC], [luac$with_lua_suffix], [], [$lua_search_path])
	test "x$LUAC" == x && exit 1
fi
dnl }}}

dnl luac_version {{{
AC_MSG_CHECKING([for lua version == luac version])
luac_version=$($LUAC -v 2>&1 | head -n 1 | cut -d' ' -f2)
if test "x$lua_version" == "x$luac_version"; then
	AC_MSG_RESULT([yes (found $luac_version)])
else
	AC_MSG_RESULT([no (found $luac_version)])
	exit 1
fi
dnl }}}

dnl LUA_INCLUDES {{{
if test "x$with_lua_includes" != x; then
	LUA_INCLUDES="-I$with_lua_includes"
elif test "x$with_lua_prefix" != x; then
	LUA_INCLUDES="-I$with_lua_prefix/include"
fi
save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $LUA_INCLUDES"
AC_CHECK_HEADERS([lua.h lualib.h], [], [exit 1], [])
CFLAGS="$save_CFLAGS"
dnl }}}

dnl LUA_LIBS {{{
if test "x$with_lua_libraries" != x; then
	LUA_LIBS="-L$with_lua_libraries"
elif test "x$with_lua" != x; then
	LUA_LIBS="-L$with_lua_prefix/lib"
fi
AC_CHECK_LIB([lua$with_lua_suffix],
			 [lua_open],
			 [LUA_LIBS="$LUA_LIBS -llua$with_lua_suffix"],
			 [exit 1],
			 [$LUA_LIBS])
AC_CHECK_LIB([dl], [dlopen], [lua_extra_libs="$lua_extra_libs -ldl"], [])
AC_CHECK_LIB([m], [exp], [lua_extra_libs="$lua_extra_libs -lm"], [])
AC_CHECK_LIB([lualib$with_lua_suffix],
			 [luaopen_loadlib],
			 [LUA_LIBS="$LUA_LIBS $lua_extra_libs -llualib$with_lua_suffix"],
			 [exit 1],
			 [$LUA_LIBS $lua_extra_libs])
dnl }}}

dnl liblua_version {{{
AC_MSG_CHECKING([for liblua version >= 5.0])
save_CFLAGS="$CFLAGS"
save_LIBS="$LIBS"
CFLAGS="$CFLAGS $LUA_INCLUDES"
LIBS="$LIBS $LUA_LIBS"
AC_RUN_IFELSE([AC_LANG_SOURCE([[

#include <lua.h>
#include <stdlib.h>
#include <string.h>

int main()
{
	if(strcmp(lua_version(), LUA_VERSION)!=0){
		printf("Error: lua_version() does not match LUA_VERSION. ");
		exit(EXIT_FAILURE);
	}
	printf("(found \"%s\")... ", lua_version());
	if(strcmp(lua_version(), "Lua 5.0")<0)
		exit(EXIT_FAILURE);
	exit(EXIT_SUCCESS);
}

]])],
              [AC_MSG_RESULT([yes])],
			  [AC_MSG_RESULT([no])
			   exit 1])
CFLAGS="$save_CFLAGS"
LIBS="$save_LIBS"
dnl }}}

dnl }}}

dnl HAS_SYSTEM_ASPRINTF {{{

AC_SUBST([HAS_SYSTEM_ASPRINTF])

AC_CHECK_FUNC([asprintf],
			  [AC_CHECK_FUNC([vasprintf],
							 [HAS_SYSTEM_ASPRINTF="1"])])

dnl }}}

dnl XOPEN_SOURCE {{{

AC_SUBST([XOPEN_SOURCE])

AC_ARG_ENABLE([xopen-source],
			  [AS_HELP_STRING([--enable-xopen-source],
							  [Enable X/Open C flags (development use only)])])

if test "x$enable_xopen_source" == xyes; then
	AC_MSG_CHECKING([for $CC flags to accept X/Open C])
	case $host in
	# FIXME: what flags are required for other hosts/compilers?
	*-*-solaris* | *-*-sunos*)
		XOPEN_SOURCE="-ansi -D__EXTENSIONS__"
		;;
	*)
		XOPEN_SOURCE="-ansi -D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED"
		;;
	esac
	AC_MSG_RESULT([$XOPEN_SOURCE])
fi

dnl }}}

dnl C99_SOURCE {{{

AC_SUBST([C99_SOURCE])

AC_ARG_ENABLE([c99-source],
			  [AS_HELP_STRING([--enable-c99-source],
							  [Enable C99 source (development use only)])])

if test "x$enable_c99_source" == xyes; then
	AC_MSG_CHECKING([for $CC flags to accept C99])
	case $CC in
	# FIXME: what flags are required for other hosts/compilers?
	gcc*)
		C99_SOURCE="-std=c99"
		;;
	esac
	AC_MSG_RESULT([$C99_SOURCE])
fi

dnl }}}

dnl _DCF_HAS_VA_COPY {{{

AC_SUBST([_DCF_HAS_VA_COPY])

dnl check for va_copy {{{

AC_MSG_CHECKING([for va_copy])
AC_RUN_IFELSE([AC_LANG_SOURCE([[

#include <stdarg.h>
#include <stdlib.h>

void f(void *last, ...)
{
	va_list ap, aq;
	va_start(ap, last);
	va_copy(aq, ap);
	va_end(ap);
	exit(va_arg(aq, int));
}

int main()
{
	f(NULL, EXIT_SUCCESS);
	exit(EXIT_FAILURE);
}

]])],
				  [AC_MSG_RESULT([yes])
				   _DCF_HAS_VA_COPY="-DCF_HAVE_VA_COPY"],
				  [AC_MSG_RESULT([no])])

dnl }}}

dnl check whether -ansi breaks va_copy {{{

if test "x${XOPEN_SOURCE/-ansi /}" != "x$XOPEN_SOURCE"; then
	AC_MSG_CHECKING([whether -ansi breaks va_copy])
	save_CFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS -ansi"
	AC_LINK_IFELSE([AC_LANG_SOURCE([[

#include <stdarg.h>
#include <stdlib.h>

void f(void *last, ...)
{
	va_list ap, aq;
	va_start(ap, last);
	va_copy(aq, ap);
	va_end(ap);
	va_end(aq);
}

int main()
{
	f(NULL);
	exit(EXIT_SUCCESS);
}

]])],
					  [AC_MSG_RESULT([no])],
					  [AC_MSG_RESULT([yes, stripping -ansi from X/Open C flags])
					   XOPEN_SOURCE="${XOPEN_SOURCE/-ansi /}"])
	CFLAGS="$save_CFLAGS"
fi
dnl }}}

dnl }}}

AC_OUTPUT([system-ac.mk])

if test "x$lua_version" == "x5.0"; then
	AC_MSG_NOTICE([**])
	AC_MSG_NOTICE([** lua version 5.0 is buggy, consider upgrading to 5.0.1])
	AC_MSG_NOTICE([** see http://lua-users.org/lists/lua-l/2003-10/msg00039.html])
	AC_MSG_NOTICE([**])
fi

dnl vim: foldmethod=marker tabstop=4 shiftwidth=4
