/*
 * ion/query/wedln.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_QUERY_WEDLN_H
#define ION_QUERY_WEDLN_H

#include <ioncore/common.h>
#include <ioncore/obj.h>
#include <ioncore/window.h>
#include <ioncore/xic.h>
#include <ioncore/extl.h>
#include "listing.h"
#include "input.h"
#include "edln.h"

INTROBJ(WEdln);
INTRSTRUCT(WEdlnCreateParams);

DECLSTRUCT(WEdlnCreateParams){
	const char *prompt;
	const char *dflt;
	ExtlFn handler;
	ExtlFn completor;
};


DECLOBJ(WEdln){
	WInput input;

	WListing complist;
	Edln edln;

	char *prompt;
	int prompt_len;
	int prompt_w;
	int vstart;
	
	ExtlFn handler;
	ExtlFn completor;
};

extern WEdln *create_wedln(WWindow *par, const WRectangle *geom,
						   WEdlnCreateParams *p);
extern void wedln_finish(WEdln *wedln);
extern void wedln_paste(WEdln *wedln);
extern void wedln_draw(WEdln *wedln, bool complete);
extern void wedln_set_completions(WEdln *wedln, ExtlTab completions);
extern void wedln_hide_completions(WEdln *wedln);

#endif /* ION_QUERY_WEDLN_H */
