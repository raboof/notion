/*
 * ion/ioncore/objp.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_WOBJP_H
#define ION_IONCORE_WOBJP_H

#include "obj.h"

typedef void DynFun();

INTRSTRUCT(DynFunTab);

DECLSTRUCT(DynFunTab){
	DynFun *func, *handler;
};

DECLSTRUCT(WObjDescr){
	const char *name;
	WObjDescr *ancestor;
	DynFunTab *funtab;
	void (*destroy_fn)();
};

#define WOBJ_TYPESTR(OBJ) (((WObj*)OBJ)->obj_type->name)

#define IMPLOBJ(OBJ, ANCESTOR, DFN, DYN)                         \
		WObjDescr OBJDESCR(OBJ)={#OBJ, &OBJDESCR(ANCESTOR), DYN, \
								 (void (*)())DFN}

#define WOBJ_INIT(O, TYPE) {((WObj*)(O))->obj_type=&OBJDESCR(TYPE); \
	((WObj*)(O))->obj_watches=NULL;}

#define CREATEOBJ_IMPL(OBJ, LOWOBJ, INIT_ARGS)                          \
	OBJ *p;  p=ALLOC(OBJ); if(p==NULL){ warn_err(); return NULL; }      \
	WOBJ_INIT(p, OBJ);                                                  \
	if(!init_##LOWOBJ INIT_ARGS){ free((void*)p); return NULL; } return p

#define SIMPLECREATEOBJ_IMPL(OBJ, LOWOBJ, INIT_ARGS)                    \
	OBJ *p;  p=ALLOC(OBJ); if(p==NULL){ warn_err(); return NULL; }      \
	WOBJ_INIT(p, OBJ);                                                  \
	return p;

#define END_DYNFUNTAB {NULL, NULL}

extern DynFun *lookup_dynfun(const WObj *obj, DynFun *func,
							 bool *funnotfound);
extern bool has_dynfun(const WObj *obj, DynFun *func);

#define CALL_DYN(FUNC, OBJ, ARGS)                                \
	bool funnotfound;                                            \
	lookup_dynfun((WObj*)OBJ, (DynFun*)FUNC, &funnotfound) ARGS;

#define CALL_DYN_RET(RETV, RET, FUNC, OBJ, ARGS)                 \
	typedef RET ThisDynFun();                                    \
	bool funnotfound;                                            \
	ThisDynFun *funtmp;                                          \
	funtmp=(ThisDynFun*)lookup_dynfun((WObj*)OBJ, (DynFun*)FUNC, \
									  &funnotfound);             \
	if(!funnotfound)                                             \
		RETV=funtmp ARGS;

#define HAS_DYN(OBJ, FUNC) has_dynfun((WObj*)OBJ, (DynFun*)FUNC)

#endif /* ION_IONCORE_WOBJP_H */
