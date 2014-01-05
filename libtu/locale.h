/*
 * libtu/locale.h
 *
 * Copyright (c) Tuomo Valkonen 2004. 
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#ifndef LIBTU_LOCALE_H
#define LIBTU_LOCALE_H

#ifdef CF_NO_GETTEXT

#define TR(X) X
#define DUMMY_TR(X) X

#else

#include <libintl.h>

#define TR(X) gettext(X)
#define DUMMY_TR(X) X

#endif

#endif /* LIBTU_LOCALE_H */
