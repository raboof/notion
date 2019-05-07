/*
 * ion/mod_query/history.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_MOD_QUERY_HISTORY_H
#define ION_MOD_QUERY_HISTORY_H

#include <ioncore/common.h>
#include <libextl/extl.h>

extern const char *mod_query_history_get(int n);
extern bool mod_query_history_push(const char *s);
extern void mod_query_history_push_(char *s);
extern void mod_query_history_clear();
extern int mod_query_history_search(const char *s, int from, bool bwd,
                                    bool exact);
extern uint mod_query_history_complete(const char *s, char ***h_ret);
extern ExtlTab mod_query_history_table();

#endif /* ION_MOD_QUERY_HISTORY_H */
