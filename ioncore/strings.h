/*
 * ion/ioncore/strings.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_STRINGS_H
#define ION_IONCORE_STRINGS_H

#include "common.h"
#include "gr.h"

#ifdef CF_UTF8
#ifdef CF_LIBUTF8
#include <libutf8.h>
#else
#include <wchar.h>
#endif
#endif

extern char *make_label(GrBrush *brush, const char *str, uint maxw);
extern bool add_shortenrule(const char *rx, const char *rule);

extern int str_nextoff(const char *p);
extern int str_prevoff(const char *p, int pos);

#ifdef CF_UTF8
extern wchar_t str_wchar_at(char *p, int max);
#endif

#endif /* ION_IONCORE_FONT_H */
