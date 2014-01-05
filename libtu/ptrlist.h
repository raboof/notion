/*
 * libtu/ptrlist.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005.
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#ifndef LIBTU_PTRLIST_H
#define LIBTU_PTRLIST_H

#include "types.h"
#include "iterable.h"


INTRSTRUCT(PtrList);


DECLSTRUCT(PtrList){
    void *ptr;
    PtrList *next, *prev;
};


typedef PtrList* PtrListIterTmp;

#define PTRLIST_FIRST(TYPE, LL) ((LL)==NULL ? NULL : (TYPE)(LL)->ptr)
#define PTRLIST_LAST(TYPE, LL) ((LL)==NULL ? NULL : (TYPE)(LL)->prev->ptr)
#define PTRLIST_EMPTY(LIST) ((LIST)==NULL)

#define FOR_ALL_ON_PTRLIST(TYPE, VAR, LL, TMP) \
    FOR_ALL_ITER(ptrlist_iter_init, (TYPE)ptrlist_iter, VAR, LL, &(TMP))

#define FOR_ALL_ON_PTRLIST_REV(TYPE, VAR, LL, TMP)        \
    FOR_ALL_ITER(ptrlist_iter_rev_init,                   \
                 (TYPE)ptrlist_iter_rev, VAR, LL, &(TMP))

#define FOR_ALL_ON_PTRLIST_UNSAFE(TYPE, VAR, LL) \
    FOR_ALL_ON_PTRLIST(TYPE, VAR, LL, ptrlist_iter_tmp)

extern PtrListIterTmp ptrlist_iter_tmp;

extern bool ptrlist_insert_last(PtrList **ptrlist, void *ptr);
extern bool ptrlist_insert_first(PtrList **ptrlist, void *ptr);
extern bool ptrlist_reinsert_last(PtrList **ptrlist, void *ptr);
extern bool ptrlist_reinsert_first(PtrList **ptrlist, void *ptr);
extern bool ptrlist_remove(PtrList **ptrlist, void *ptr);
extern bool ptrlist_contains(PtrList *ptrlist, void *ptr);
extern void ptrlist_clear(PtrList **ptrlist);
extern void ptrlist_iter_init(PtrListIterTmp *state, PtrList *ptrlist);
extern void *ptrlist_iter(PtrListIterTmp *state);
extern void ptrlist_iter_rev_init(PtrListIterTmp *state, PtrList *ptrlist);
extern void *ptrlist_iter_rev(PtrListIterTmp *state);
extern void *ptrlist_take_first(PtrList **ptrlist);
extern void *ptrlist_take_last(PtrList **ptrlist);

#endif /* LIBTU_PTRLIST_H */
