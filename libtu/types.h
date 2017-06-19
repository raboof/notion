/*
 * libtu/types.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002.
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#ifndef LIBTU_TYPES_H
#define LIBTU_TYPES_H

#include <sys/types.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifndef LIBTU_TYPEDEF_UXXX

 /* All systems seem to define these whichever way they want to
  * despite -D_*_SOURCE etc. so there is no easy way to know whether
  * they can be typedef'd or not. Unless you want to go through using
  * autoconf or similar methods. ==> Just stick to #define. :-(
  */

#ifndef uchar
#define uchar unsigned char
#endif

#ifndef ushort
#define ushort unsigned short
#endif

#ifndef uint
#define uint unsigned int
#endif

#ifndef ulong
#define ulong unsigned long
#endif

#else /* LIBTU_TYPEDEF_UXXX */

#ifndef uchar
typedef unsigned char uchar;
#endif

#ifndef ushort
typedef unsigned short ushort;
#endif

#ifndef uint
typedef unsigned int uint;
#endif

#ifndef ulong
typedef unsigned long ulong;
#endif

#endif /* LIBTU_TYPEDEF_UXXX */


#ifndef LIBTU_TYPEDEF_BOOL

#ifndef bool
#define bool int
#endif

#else /* LIBTU_TYPEDEF_BOOL */

#ifndef bool
typedef int bool;
#endif

#endif /* LIBTU_TYPEDEF_BOOL */

#endif /* LIBTU_TYPES_H */
