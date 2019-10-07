/*
 * libtu/objp.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004.
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#ifndef LIBTU_OBJP_H
#define LIBTU_OBJP_H

#include "types.h"
#include "obj.h"

typedef void DynFun();

INTRSTRUCT(DynFunTab);

DECLSTRUCT(DynFunTab){
    DynFun *func, *handler;
};

DECLSTRUCT(ClassDescr){
    const char *name;
    ClassDescr *ancestor;
    int funtab_n;
    DynFunTab *funtab;
    void (*destroy_fn)();
};

#define OBJ_TYPESTR(OBJ) ((OBJ) ? ((Obj*)OBJ)->obj_type->name : NULL)

#define IMPLCLASS(CLS, ANCESTOR, DFN, DYN)                         \
        ClassDescr CLASSDESCR(CLS)={                              \
            #CLS, &CLASSDESCR(ANCESTOR), -1, DYN, (void (*)())DFN}

#define OBJ_INIT(O, TYPE) {((Obj*)(O))->obj_type=&CLASSDESCR(TYPE); \
    ((Obj*)(O))->obj_watches=NULL; ((Obj*)(O))->flags=0;}

#define CREATEOBJ_IMPL(OBJ, LOWOBJ, INIT_ARGS)                     \
    OBJ *p;  p=ALLOC(OBJ); if(p==NULL){ warn_err(); return NULL; } \
    OBJ_INIT(p, OBJ);                                             \
    if(!LOWOBJ ## _init INIT_ARGS) { free((void*)p); return NULL; } return p; \
    ((void)0)

#define SIMPLECREATEOBJ_IMPL(OBJ, LOWOBJ)                          \
    OBJ *p;  p=ALLOC(OBJ); if(p==NULL){ warn_err(); return NULL; } \
    OBJ_INIT(p, OBJ);                                              \
    return p;                                                      \
    ((void)0)

#define END_DYNFUNTAB {NULL, NULL}

extern DynFun *lookup_dynfun(const Obj *obj, DynFun *func,
                             bool *funnotfound);
extern bool has_dynfun(const Obj *obj, DynFun *func);

#define CALL_DYN(FUNC, OBJ, ARGS)                                \
    bool funnotfound;                                            \
    lookup_dynfun((Obj*)OBJ, (DynFun*)FUNC, &funnotfound) ARGS;  \
    ((void)0)

#define CALL_DYN_RET(RETV, RET, FUNC, OBJ, ARGS)                 \
    typedef RET ThisDynFun();                                    \
    bool funnotfound;                                            \
    ThisDynFun *funtmp;                                          \
    funtmp=(ThisDynFun*)lookup_dynfun((Obj*)OBJ, (DynFun*)FUNC,  \
                                      &funnotfound);             \
    if(!funnotfound){                                            \
        RETV=funtmp ARGS;                                        \
    } ((void)0)

#define HAS_DYN(OBJ, FUNC) has_dynfun((Obj*)OBJ, (DynFun*)FUNC)

#endif /* LIBTU_OBJP_H */
