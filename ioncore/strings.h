/*
 * ion/ioncore/strings.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_STRINGS_H
#define ION_IONCORE_STRINGS_H

#include "common.h"

#ifdef CF_NO_MB_SUPPORT
#include "dummywc.h"
#else
#include <wchar.h>
#include <wctype.h>
#endif

#include "gr.h"

extern bool ioncore_add_shortenrule(const char *rx, const char *rule,
                                    bool always);

extern char *grbrush_make_label(GrBrush *brush, const char *str, uint maxw);
                                       
extern int str_nextoff(const char *p, int pos);
extern int str_prevoff(const char *p, int pos);
extern wchar_t str_wchar_at(char *p, int max);
extern char *str_stripws(char *p);

#endif /* ION_IONCORE_STRINGS_H */
