/*
 * libtu/debug.h
 *
 * Copyright (c) Tuomo Valkonen 2004. 
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#ifndef LIBTU_DEBUG_H
#define LIBTU_DEBUG_H

#ifdef CF_DEBUG
#define D(X) X
#else
#define D(X)
#endif

#endif /* LIBTU_DEBUG_H */
