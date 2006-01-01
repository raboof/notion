/*
 * ion/mod_query/history.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_QUERY_HISTORY_H
#define ION_MOD_QUERY_HISTORY_H

#include <ioncore/common.h>
#include <libextl/extl.h>

extern const char *mod_query_history_get(int n);
extern bool mod_query_history_push(const char *s);
extern void mod_query_history_push_(char *s);
extern void mod_query_history_clear();
extern int mod_query_history_search(const char *s, int from, bool bwd);
extern ExtlTab mod_query_history_table();

#endif /* ION_MOD_QUERY_HISTORY_H */
