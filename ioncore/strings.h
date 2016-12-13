/*
 * ion/ioncore/strings.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_STRINGS_H
#define ION_IONCORE_STRINGS_H

#include "common.h"

#ifdef CF_NO_LOCALE
#include "dummywc.h"
#else
#include <wchar.h>
#include <wctype.h>
#endif

#include "gr.h"

extern bool ioncore_defshortening(const char *rx, const char *rule,
                                  bool always);

extern char *grbrush_make_label(GrBrush *brush, const char *str, uint maxw);

extern int str_nextoff(const char *p, int pos);
extern int str_prevoff(const char *p, int pos);
extern int str_len(const char *p);
extern wchar_t str_wchar_at(char *p, int max);
extern char *str_stripws(char *p);

#endif /* ION_IONCORE_STRINGS_H */
