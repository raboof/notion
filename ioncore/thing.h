/*
 * wmcore/thing.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_THING_H
#define WMCORE_THING_H

#include "common.h"

INTROBJ(WThing)
INTRSTRUCT(WWatch)

#define WTHING_DESTREMPTY	0x00001
#define WTHING_SUBDEST		0x10000

#define WTHING_CHILDREN(THING, TYPE) ((TYPE*)((WThing*)(THING))->t_children)
#define WTHING_PARENT(THING, TYPE) ((TYPE*)((WThing*)(THING))->t_parent)
#define WTHING_HAS_PARENT(THING, WT) \
	(((WThing*)(THING))->t_parent!=NULL && \
	 WTHING_IS(((WThing*)(THING))->t_parent, WT))


#define FIRST_THING(NAM, TYPE) (TYPE*)first_thing((WThing*)NAM, &OBJDESCR(TYPE))
#define NEXT_THING(NAM, TYPE) (TYPE*)next_thing((WThing*)NAM, &OBJDESCR(TYPE))
#define NEXT_THING_FB(NAM, TYPE, FB) (TYPE*)next_thing_fb((WThing*)NAM, &OBJDESCR(TYPE), (WThing*)FB)
#define LAST_THING(NAM, TYPE) (TYPE*)last_thing((WThing*)NAM, &OBJDESCR(TYPE))
#define PREV_THING(NAM, TYPE) (TYPE*)prev_thing((WThing*)NAM, &OBJDESCR(TYPE))
#define PREV_THING_FB(NAM, TYPE, FB) (TYPE*)prev_thing_fb((WThing*)NAM, &OBJDESCR(TYPE), (WThing*)FB)
#define FIND_PARENT(NAM, TYPE) (TYPE*)find_parent((WThing*)NAM, &OBJDESCR(TYPE))
#define FIND_PARENT1(NAM, TYPE) (TYPE*)find_parent1((WThing*)NAM, &OBJDESCR(TYPE))
#define NTH_THING(NAM, N, TYPE) (TYPE*)nth_thing((WThing*)NAM, N, &OBJDESCR(TYPE))
#define FOR_ALL_TYPED(NAM, NAM2, TYPE) \
	for(NAM2=FIRST_THING(NAM, TYPE); NAM2!=NULL; NAM2=NEXT_THING(NAM2, TYPE))

#define WTHING_IS(X, Y) WOBJ_IS(X, Y)

extern void init_thing(WThing *thing);
extern void link_thing(WThing *parent, WThing *child);
/*extern void link_thing_before(WThing *before, WThing *child);*/
extern void link_thing_after(WThing *after, WThing *thing);
extern void unlink_thing(WThing *thing);
extern void destroy_subthings(WThing *thing);
extern void destroy_thing(WThing *thing);

extern WThing *next_thing(WThing *first, const WObjDescr *descr);
extern WThing *next_thing_fb(WThing *first, const WObjDescr *descr, WThing *fb);
extern WThing *prev_thing(WThing *first, const WObjDescr *descr);
extern WThing *prev_thing_fb(WThing *first, const WObjDescr *descr, WThing *fb);
extern WThing *first_thing(WThing *parent, const WObjDescr *descr);
extern WThing *last_thing(WThing *parent, const WObjDescr *descr);
extern WThing *find_parent(WThing *p, const WObjDescr *descr);
extern WThing *find_parent1(WThing *p, const WObjDescr *descr);
extern WThing *nth_thing(WThing *parent, int n, const WObjDescr *descr);

extern bool thing_is_ancestor(WThing *thing, WThing *thing2);
extern bool thing_is_child(WThing *thing, WThing *thing2);

typedef void WWatchHandler(WWatch *watch, WThing *t);

extern void setup_watch(WWatch *watch, WThing *thing,
						WWatchHandler *handler);
extern void reset_watch(WWatch *watch);
extern bool watch_ok(WWatch *watch);


#define WWATCH_INIT {NULL, NULL, NULL, NULL}


DECLSTRUCT(WWatch){
	WThing *thing;
	WWatch *next, *prev;
	WWatchHandler *handler;
};


DECLOBJ(WThing){
	WObj obj;
	int flags;
	void *screen;
	WWatch *t_watches;
	WThing *t_parent, *t_children;
	WThing *t_next, *t_prev;
};

#endif /* WMCORE_THING_H */
