/*
 * ion/mod_query/fwarn.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_MOD_QUERY_FWARN_H
#define ION_MOD_QUERY_FWARN_H

#include <ioncore/mplex.h>
#include "wmessage.h"

extern WMessage *mod_query_do_message(WMPlex *mplex, const char *p);
extern WMessage *mod_query_do_fwarn(WMPlex *mplex, const char *p);

#endif /* ION_MOD_QUERY_FWARN_H */
