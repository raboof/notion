/*
 * ion/ioncore/obj.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_WOBJ_H
#define ION_IONCORE_WOBJ_H

#define OBJDESCR(TYPE) TYPE##_objdescr

#define WOBJ_IS(OBJ, TYPE) wobj_is((WObj*)OBJ, &OBJDESCR(TYPE))
/*#define WOBJ_CAST(OBJ, TYPE) (TYPE*)wobj_cast((WObj*)OBJ, &OBJDESCR(TYPE))*/

#define INTRSTRUCT(STRU) \
	struct STRU##_struct; typedef struct STRU##_struct STRU
#define DECLSTRUCT(STRU)  \
	struct STRU##_struct

#define INTROBJ(OBJ) INTRSTRUCT(OBJ); extern WObjDescr OBJDESCR(OBJ)
#define DECLOBJ(OBJ) DECLSTRUCT(OBJ)

INTRSTRUCT(WObjDescr);
INTROBJ(WObj);
INTRSTRUCT(WWatch);

extern bool wobj_is(const WObj *obj, const WObjDescr *descr);
extern bool wobj_is_str(const WObj *obj, const char *str);
/*extern const void *wobj_cast(const WObj *obj, const WObjDescr *descr);*/

extern void destroy_obj(WObj *obj);

DECLOBJ(WObj){
	WObjDescr *obj_type;
	WWatch *obj_watches;
	int flags;
};

#define WOBJ_DEST 0x0001
#define WOBJ_IS_BEING_DESTROYED(OBJ) (((WObj*)(OBJ))->flags&WOBJ_DEST)

#define DYNFUN

typedef void WWatchHandler(WWatch *watch, WObj *obj);

#define WWATCH_INIT {NULL, NULL, NULL, NULL}

DECLSTRUCT(WWatch){
	WObj *obj;
	WWatch *next, *prev;
	WWatchHandler *handler;
};

extern bool setup_watch(WWatch *watch, WObj *obj,
						WWatchHandler *handler);
extern void reset_watch(WWatch *watch);
extern bool watch_ok(WWatch *watch);
extern void init_watch(WWatch *watch);
extern void call_watches(WObj *obj);

#endif /* ION_IONCORE_WOBJ_H */
