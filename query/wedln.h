/*
 * ion/query/wedln.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
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

typedef void WEdlnHandler(WObj *p, char *str, char *userdata);


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

extern WEdln *create_wedln(WWindow *par, WRectangle geom,
						   WEdlnCreateParams *p);
extern void wedln_finish(WEdln *wedln);
extern void wedln_paste(WEdln *wedln);
extern void wedln_draw(WEdln *wedln, bool complete);
extern void wedln_set_completions(WEdln *wedln, ExtlTab completions);
extern void wedln_hide_completions(WEdln *wedln);

#endif /* ION_QUERY_WEDLN_H */
