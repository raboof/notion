/*
 * ion/query/fwarn.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_QUERY_FWARN_H
#define ION_QUERY_FWARN_H

#include <ioncore/genframe.h>

extern void query_message(WGenFrame *frame, const char *p);
extern void query_fwarn(WGenFrame *frame, const char *p);

#endif /* ION_QUERY_FWARN_H */
