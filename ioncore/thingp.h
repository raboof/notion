/*
 * ion/ioncore/thingp.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_THINGP_H
#define ION_IONCORE_THINGP_H

#include "common.h"
#include "thing.h"
#include "screen.h"
#include "objp.h"


typedef void WThingDeinitFn(WThing*);
typedef void WThingRemoveChildFn(WThing*, WThing*);


#define CREATETHING_IMPL(OBJ, LOWOBJ, INIT_ARGS)                          \
	OBJ *p;  p=ALLOC(OBJ); if(p==NULL){ warn_err(); return NULL; }        \
	WTHING_INIT(p, OBJ);                                                  \
	if(!init_##LOWOBJ INIT_ARGS){ free((void*)p); return NULL; } return p

#define WTHING_INIT(P, TYPE)         \
	WOBJ_INIT(P, TYPE);              \
	((WThing*)(P))->flags=0;         \
	((WThing*)(P))->t_watches=NULL;  \
	((WThing*)(P))->t_parent=NULL;   \
	((WThing*)(P))->t_children=NULL; \
	((WThing*)(P))->t_next=NULL;     \
	((WThing*)(P))->t_prev=NULL;

#endif /* ION_IONCORE_THINGP_H */
