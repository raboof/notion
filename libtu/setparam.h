/*
 * libtu/setparam.h
 *
 * Copyright (c) Tuomo Valkonen 2005.
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#ifndef LIBTU_SETPARAM_H
#define LIBTU_SETPARAM_H

#include "types.h"

enum{
    SETPARAM_UNKNOWN,
    SETPARAM_SET,
    SETPARAM_UNSET,
    SETPARAM_TOGGLE
};

extern int libtu_string_to_setparam(const char *str);
extern bool libtu_do_setparam_str(const char *str, bool val);
extern bool libtu_do_setparam(int sp, bool val);
extern int libtu_setparam_invert(int sp);

#endif /* LIBTU_SETPARAM_H */
