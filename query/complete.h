/*
 * ion/query/complete.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_QUERY_COMPLETE_H
#define ION_QUERY_COMPLETE_H

#include "edln.h"

extern void edln_complete(Edln *edln);
extern int edln_do_completions(Edln *edln, char **completions, int ncomp,
							   const char *beg);

#endif /* ION_QUERY_COMPLETE_H */
