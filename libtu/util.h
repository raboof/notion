/*
 * libtu/util.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#ifndef LIBTU_UTIL_H
#define LIBTU_UTIL_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "optparser.h"

/** 
 * @parame argv0 The program name used to invoke the current program, with 
 * path (if specified). Unfortunately it is generally not easy to determine 
 * the encoding of this string, so we don't require a specific one here.
 * 
 * @see http://stackoverflow.com/questions/5408730/what-is-the-encoding-of-argv
 */
extern void libtu_init(const char *argv0);
/** 
 * The program name used to invoke the current program, with path (if 
 * supplied). Unfortunately the encoding is undefined.
 */
extern const char *libtu_progname();
/** 
 * The program name used to invoke the current program, without path.
 * Unfortunately the encoding is undefined. 
 */
extern const char *libtu_progbasename();

#endif /* LIBTU_UTIL_H */
