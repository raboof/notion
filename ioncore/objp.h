/*
 * ion/ioncore/objp.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_WOBJP_H
#define ION_IONCORE_WOBJP_H

#include <libtu/types.h>
#include "obj.h"

typedef void DynFun();

INTRSTRUCT(DynFunTab);

DECLSTRUCT(DynFunTab){
	DynFun *func, *handler;
};

DECLSTRUCT(WClassDescr){
	const char *name;
	WClassDescr *ancestor;
	int funtab_n;
	DynFunTab *funtab;
	void (*destroy_fn)();
};

#define OBJ_TYPESTR(OBJ) (((WObj*)OBJ)->obj_type->name)

#define IMPLCLASS(CLS, ANCESTOR, DFN, DYN)                         \
		WClassDescr CLASSDESCR(CLS)={                              \
			#CLS, &CLASSDESCR(ANCESTOR), -1, DYN, (void (*)())DFN}

#define OBJ_INIT(O, TYPE) {((WObj*)(O))->obj_type=&CLASSDESCR(TYPE); \
	((WObj*)(O))->obj_watches=NULL; ((WObj*)(O))->flags=0;}

#define CREATEOBJ_IMPL(OBJ, LOWOBJ, INIT_ARGS)                     \
	OBJ *p;  p=ALLOC(OBJ); if(p==NULL){ warn_err(); return NULL; } \
	OBJ_INIT(p, OBJ);                                             \
	if(!LOWOBJ ## _init INIT_ARGS) { free((void*)p); return NULL; } return p

#define SIMPLECREATEOBJ_IMPL(OBJ, LOWOBJ)                          \
	OBJ *p;  p=ALLOC(OBJ); if(p==NULL){ warn_err(); return NULL; } \
	OBJ_INIT(p, OBJ);                                             \
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
