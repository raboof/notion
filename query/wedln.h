/*
 * query/wedln.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef QUERY_WEDLN_H
#define QUERY_WEDLN_H

#include <wmcore/common.h>
#include <wmcore/thing.h>
#include <wmcore/window.h>
#include <wmcore/xic.h>
#include "listing.h"
#include "input.h"
#include "edln.h"

INTROBJ(WEdln)
INTRSTRUCT(WEdlnCreateParams)

typedef void WEdlnHandler(WThing *p, char *str, char *userdata);


DECLSTRUCT(WEdlnCreateParams){
	WEdlnHandler *handler;
	const char *prompt;
	const char *dflt;
};


DECLOBJ(WEdln){
	WInput input;

	WListing complist;
	Edln edln;

	char *prompt;
	int prompt_len;
	int prompt_w;
	int vstart;
	
	WEdlnHandler *handler;
	char *userdata;
};

extern WEdln *create_wedln(WRegion *par, WRectangle geom,
						   WEdlnCreateParams *p);
extern void wedln_set_completion_handler(WEdln *wedln,
										 EdlnCompletionHandler *h, void *d);
extern void deinit_wedln(WEdln *edln);

extern void wedln_finish(WEdln *wedln);
extern void wedln_paste(WEdln *wedln);
extern void wedln_draw(WEdln *wedln, bool complete);

#endif /* QUERY_WEDLN_H */
