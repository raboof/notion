/*
 * ion/query/query.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_QUERY_QUERY_H
#define ION_QUERY_QUERY_H

#include <ioncore/common.h>
#include <ioncore/genframe.h>
#include <ioncore/extl.h>

extern void query_query(WGenFrame *frame, const char *prompt, const char *dflt,
						ExtlFn handler, ExtlFn completor);

#endif /* ION_QUERY_QUERY_H */
