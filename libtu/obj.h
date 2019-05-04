/*
 * libtu/obj.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#ifndef LIBTU_OBJ_H
#define LIBTU_OBJ_H

#include "types.h"

#define CLASSDESCR(TYPE) TYPE##_classdescr

#define OBJ_IS(OBJ, TYPE) obj_is((Obj*)OBJ, &CLASSDESCR(TYPE))
#define OBJ_CAST(OBJ, TYPE) (TYPE*)obj_cast((Obj*)OBJ, &CLASSDESCR(TYPE))

#define INTRSTRUCT(STRU) \
    struct STRU##_struct; typedef struct STRU##_struct STRU
#define DECLSTRUCT(STRU)  \
    struct STRU##_struct

#define INTRCLASS(OBJ) INTRSTRUCT(OBJ); extern ClassDescr CLASSDESCR(OBJ)
#define DECLCLASS(OBJ) DECLSTRUCT(OBJ)

INTRSTRUCT(ClassDescr);
INTRCLASS(Obj);
INTRSTRUCT(Watch);

extern bool obj_is(const Obj *obj, const ClassDescr *descr);
extern bool obj_is_str(const Obj *obj, const char *str);
extern const void *obj_cast(const Obj *obj, const ClassDescr *descr);

extern void destroy_obj(Obj *obj);

DECLCLASS(Obj){
    ClassDescr *obj_type;
    Watch *obj_watches;
    int flags;
};

#define OBJ_DEST 0x0001
#define OBJ_EXTL_CACHED 0x0002
#define OBJ_EXTL_OWNED 0x0004

#define OBJ_IS_BEING_DESTROYED(OBJ) (((Obj*)(OBJ))->flags&OBJ_DEST)

#define DYNFUN

typedef void WatchHandler(Watch *watch, Obj *obj);

#define WATCH_INIT {NULL, NULL, NULL, NULL}

DECLSTRUCT(Watch){
    Obj *obj;
    Watch *next, *prev;
    WatchHandler *handler;
};

extern bool watch_setup(Watch *watch, Obj *obj,
                        WatchHandler *handler);
extern void watch_reset(Watch *watch);
extern bool watch_ok(Watch *watch);
extern void watch_init(Watch *watch);
extern void watch_call(Obj *obj);

#endif /* LIBTU_OBJ_H */
