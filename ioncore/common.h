/*
 * ion/ioncore/common.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_COMMON_H
#define ION_IONCORE_COMMON_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <libtu/types.h>
#include <libtu/output.h>
#include <libtu/misc.h>
#include <libtu/dlist.h>
#include <libtu/obj.h>

#include "../config.h"
#include "classes.h"

#if 0
#define D(X) X
#else
#define D(X)
#endif

#define EXTL_EXPORT
#define EXTL_EXPORT_AS(T, F)
#define EXTL_EXPORT_MEMBER

#if __STDC_VERSION__ >= 199901L
#define WARN_FUNC(...) warn_obj(__func__, __VA_ARGS__)
#define WARN_ERR_FUNC() warn_err_obj(__func__)
#else
#define WARN_FUNC warn
#define WARN_ERR_FUNC() warn_err()
#endif

#ifdef CF_NO_LOCALE

#define TR(X) X
#define DUMMY_TR(X) X

#else

#include <libintl.h>

#define TR(X) gettext(X)
#define DUMMY_TR(X) X

#endif

#endif /* ION_IONCORE_COMMON_H */
