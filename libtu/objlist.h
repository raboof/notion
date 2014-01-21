/*
 * libtu/objlist.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005.
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#ifndef LIBTU_OBJLIST_H
#define LIBTU_OBJLIST_H

#include "types.h"
#include "iterable.h"
#include "obj.h"


INTRSTRUCT(ObjList);


DECLSTRUCT(ObjList){
    Watch watch; /* Must be kept at head of structure */
    ObjList *next, *prev;
    ObjList **list;
};


typedef ObjList* ObjListIterTmp;

#define OBJLIST_FIRST(TYPE, LL) ((LL)==NULL ? NULL : (TYPE)(LL)->watch.obj)
#define OBJLIST_LAST(TYPE, LL) ((LL)==NULL ? NULL : (TYPE)(LL)->prev->watch.obj)
#define OBJLIST_EMPTY(LIST) objlist_empty(LIST)

#define FOR_ALL_ON_OBJLIST(TYPE, VAR, LL, TMP) \
    FOR_ALL_ITER(objlist_iter_init, (TYPE)objlist_iter, VAR, LL, &(TMP))

#define FOR_ALL_ON_OBJLIST_REV(TYPE, VAR, LL, TMP)        \
    FOR_ALL_ITER(objlist_iter_rev_init,                   \
                 (TYPE)objlist_iter_rev, VAR, LL, &(TMP))

#define FOR_ALL_ON_OBJLIST_UNSAFE(TYPE, VAR, LL) \
    FOR_ALL_ON_OBJLIST(TYPE, VAR, LL, objlist_iter_tmp)

extern ObjListIterTmp objlist_iter_tmp;

extern bool objlist_insert_last(ObjList **objlist, Obj *obj);
extern bool objlist_insert_first(ObjList **objlist, Obj *obj);
extern bool objlist_reinsert_last(ObjList **objlist, Obj *obj);
extern bool objlist_reinsert_first(ObjList **objlist, Obj *obj);
extern bool objlist_remove(ObjList **objlist, Obj *obj);
extern bool objlist_contains(ObjList *objlist, Obj *obj);
extern void objlist_clear(ObjList **objlist);
extern void objlist_iter_init(ObjListIterTmp *state, ObjList *objlist);
extern Obj *objlist_iter(ObjListIterTmp *state);
extern void objlist_iter_rev_init(ObjListIterTmp *state, ObjList *objlist);
extern Obj *objlist_iter_rev(ObjListIterTmp *state);
extern bool objlist_empty(ObjList *objlist);
extern Obj *objlist_take_first(ObjList **objlist);
extern Obj *objlist_take_last(ObjList **objlist);

#endif /* LIBTU_OBJLIST_H */
