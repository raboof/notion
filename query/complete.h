/*
 * ion/query/complete.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_QUERY_COMPLETE_H
#define ION_QUERY_COMPLETE_H

#include "edln.h"

extern void edln_complete(Edln *edln);
extern int edln_do_completions(Edln *edln, char **completions, int ncomp,
							   const char *beg);

#endif /* ION_QUERY_COMPLETE_H */
