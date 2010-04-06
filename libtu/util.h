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

extern void libtu_init(const char *argv0);
extern const char *libtu_progname();
extern const char *libtu_progbasename();

#endif /* LIBTU_UTIL_H */
