/*
 * ion/ioncore/obj.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_OBJ_H
#define ION_IONCORE_OBJ_H

#include <libtu/types.h>

#define CLASSDESCR(TYPE) TYPE##_classdescr

#define OBJ_IS(OBJ, TYPE) obj_is((WObj*)OBJ, &CLASSDESCR(TYPE))
#define OBJ_CAST(OBJ, TYPE) (TYPE*)obj_cast((WObj*)OBJ, &CLASSDESCR(TYPE))

#define INTRSTRUCT(STRU) \
	struct STRU##_struct; typedef struct STRU##_struct STRU
#define DECLSTRUCT(STRU)  \
	struct STRU##_struct

#define INTRCLASS(OBJ) INTRSTRUCT(OBJ); extern WClassDescr CLASSDESCR(OBJ)
#define DECLCLASS(OBJ) DECLSTRUCT(OBJ)

INTRSTRUCT(WClassDescr);
INTRCLASS(WObj);
INTRSTRUCT(WWatch);

extern bool obj_is(const WObj *obj, const WClassDescr *descr);
extern bool obj_is_str(const WObj *obj, const char *str);
extern const void *obj_cast(const WObj *obj, const WClassDescr *descr);

extern void destroy_obj(WObj *obj);

DECLCLASS(WObj){
	WClassDescr *obj_type;
	WWatch *obj_watches;
	int flags;
};

#define OBJ_DEST 0x0001
#define OBJ_EXTL_CACHED 0x0002

#define OBJ_IS_BEING_DESTROYED(OBJ) (((WObj*)(OBJ))->flags&OBJ_DEST)

#define DYNFUN

typedef void WWatchHandler(WWatch *watch, WObj *obj);

#define WWATCH_INIT {NULL, NULL, NULL, NULL}

DECLSTRUCT(WWatch){
	WObj *obj;
	WWatch *next, *prev;
	WWatchHandler *handler;
};

extern bool watch_setup(WWatch *watch, WObj *obj,
						WWatchHandler *handler);
extern void watch_reset(WWatch *watch);
extern bool watch_ok(WWatch *watch);
extern void watch_init(WWatch *watch);
extern void watch_calles(WObj *obj);

#endif /* ION_IONCORE_OBJ_H */
