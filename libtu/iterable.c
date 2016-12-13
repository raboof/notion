/*
 * libtu/iterable.c
 *
 * Copyright (c) Tuomo Valkonen 2005.
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#include "iterable.h"


void *iterable_nth(uint n, VoidIterator *iter, void *st)
{
    void *p;

    while(1){
        p=iter(st);
        if(p==NULL || n==0)
            break;
        n--;
    }

    return p;
}


bool iterable_is_on(void *p, VoidIterator *iter, void *st)
{
    while(1){
        void *p2=iter(st);
        if(p2==NULL)
            return FALSE;
        if(p==p2)
            return TRUE;
    }
}


void *iterable_find(BoolFilter *f, void *fparam,
                    VoidIterator *iter, void *st)
{
    while(1){
        void *p=iter(st);
        if(p==NULL)
            return NULL;
        if(f(p, fparam))
            return p;
    }
}

