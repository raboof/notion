/*
 * libtu/iterable.h
 *
 * Copyright (c) Tuomo Valkonen 2005.
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#ifndef LIBTU_ITERABLE_H
#define LIBTU_ITERABLE_H

#include "types.h"
#include "obj.h"

typedef void *VoidIterator(void *);
typedef Obj *ObjIterator(void *);

typedef bool BoolFilter(void *p, void *param);

#define FOR_ALL_ITER(INIT, ITER, VAR, LL, TMP) \
    for(INIT(TMP, LL), (VAR)=ITER(TMP); (VAR)!=NULL; VAR=ITER(TMP))

extern void *iterable_nth(uint n, VoidIterator *iter, void *st);
extern bool iterable_is_on(void *p, VoidIterator *iter, void *st);
extern void *iterable_find(BoolFilter *f, void *fparam, 
                           VoidIterator *iter, void *st);
    
#endif /* LIBTU_ITERABLE_H */
