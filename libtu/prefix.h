/*
 * libtu/prefix.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#ifndef _LIBTU_PREFIX_H
#define _LIBTU_PREFIX_H

extern void prefix_set(const char *binloc, const char *dflt);
extern char *prefix_add(const char *s);
extern bool prefix_wrap_simple(bool (*fn)(const char *s), const char *s);

#endif /* _LIBTU_PREFIX_H */
