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
#define WOBJ_CAST(OBJ, TYPE) (TYPE*)wobj_cast((WObj*)OBJ, &OBJDESCR(TYPE))

#define INTRSTRUCT(STRU)                                    \
	struct STRU##_struct; typedef struct STRU##_struct STRU;
#define DECLSTRUCT(STRU)  \
	struct STRU##_struct

#define INTROBJ(OBJ) INTRSTRUCT(OBJ)
#define DECLOBJ(OBJ) extern WObjDescr OBJDESCR(OBJ); DECLSTRUCT(OBJ)

INTROBJ(WObj)
INTRSTRUCT(WObjDescr)

extern bool wobj_is(const WObj *obj, const WObjDescr *descr);
extern const void *wobj_cast(const WObj *obj, const WObjDescr *descr);

DECLOBJ(WObj){
	WObjDescr *obj_type;
};

#define DYNFUN

#endif /* ION_IONCORE_WOBJ_H */
