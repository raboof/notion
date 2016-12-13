/*
 * libtu/private.h
 *
 * Copyright (c) Tuomo Valkonen 2004.
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#ifndef LIBTU_PRIVATE_H
#define LIBTU_PRIVATE_H

#ifndef CF_NO_GETTEXT

#include <libintl.h>

#define TR(X) dgettext("libtu", X)

#else

#define TR(X) (X)

#endif

#define DUMMY_TR(X) X

#endif /* LIBTU_PRIVATE_H */
