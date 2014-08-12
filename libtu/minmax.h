/*
 * libtu/minmax.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#ifndef LIBTU_MINMAX_H
#define LIBTU_MINMAX_H

#if defined(__GNUC__) || defined(__clang__)

#define MINOF(a, b) __extension__ ({  \
	__typeof(a) a_ = (a); __typeof(b) b_ = (b); \
	((a_) < (b_) ? (a_) : (b_)); })
#define MAXOF(a, b) __extension__ ({  \
	__typeof(a) a_ = (a); __typeof(b) b_ = (b); \
	((a_) > (b_) ? (a_) : (b_)); })

#else

#define MINOF(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAXOF(X,Y) ((X) > (Y) ? (X) : (Y))
#endif

#endif /* LIBTU_MINMAX_H */

