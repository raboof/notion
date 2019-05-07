/*
 * ion/mod_query/query.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_MOD_QUERY_QUERY_H
#define ION_MOD_QUERY_QUERY_H

#include <ioncore/common.h>
#include <ioncore/mplex.h>
#include <libextl/extl.h>
#include "wedln.h"

extern WEdln *mod_query_do_query(WMPlex *mplex,
                                 const char *prompt, const char *dflt,
                                 ExtlFn handler, ExtlFn completor,
                                 ExtlFn cycle, ExtlFn bcycle);

#endif /* ION_MOD_QUERY_QUERY_H */
