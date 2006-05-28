/*
 * ion/ioncore/stacking.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006.
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "common.h"
#include "region.h"
#include "stacking.h"
#include "window.h"


/*{{{ List processing */


static WStacking *link_lists(WStacking *l1, WStacking *l2)
{
    /* As everywhere, doubly-linked lists without the forward 
     * link in last item! 
     */
    WStacking *tmp=l2->prev;
    l1->prev->next=l2;
    l2->prev=l1->prev;
    l1->prev=tmp;
    return l1;
}


static WStacking *link_list_before(WStacking *l1, 
                                   WStacking *i1,
                                   WStacking *l2)
{
    WStacking *tmp;
    
    if(i1==l1)
        return link_lists(l2, l1);
    
    l2->prev->next=i1;
    i1->prev->next=l2;
    tmp=i1->prev;
    i1->prev=l2->prev;
    l2->prev=tmp;
    
    return l1;
}


static WStacking *link_list_after(WStacking *l1, 
                                  WStacking *i1,
                                  WStacking *l2)
{
    WStacking *tmp;
    
    if(i1==l1->prev)
        return link_lists(l1, l2);
    
    i1->next->prev=l2->prev;
    l2->prev->next=i1->next;
    i1->next=l2;
    l2->prev=i1;
    
    return l1;
}


WStacking *stacking_find(WStacking *st, WRegion *reg)
{
    while(st!=NULL){
        if(st->reg==reg)
            return st;
        st=st->next;
    }
    
    return NULL;
}


WRegion *stacking_remove(WWindow *par, WRegion *reg)
{
    WStacking *regst, *st, *nxt=NULL;
    
    if(par!=NULL && par->stacking!=NULL){
        WStacking *regst=stacking_find(par->stacking, reg);
        
        if(regst!=NULL){
            nxt=stacking_unlink(par, regst);
            free(regst);
        }
    }
    
    return (nxt!=NULL ? nxt->reg : NULL);
}


WStacking *stacking_unlink(WWindow *par, WStacking *regst)
{
    WStacking *nxt=NULL, *st;
    
    st=regst->next;
    
    UNLINK_ITEM(par->stacking, regst, next, prev);
            
    /*while(st!=NULL){*/
    for(st=par->stacking; st!=NULL; st=st->next){
        if(st->above==regst){
            st->above=NULL;
            nxt=st;
        }
        /*st=st->next;*/
    }
    
    return nxt;
}


/*}}}*/


/*{{{ Restack */


static bool cf(WStackingFilter *filt, void *filt_data, WRegion *reg)
{
    return (filt==NULL || filt(reg, filt_data));
}


void stacking_restack(WStacking **stacking, Window other, int mode,
                      WStacking *other_on_list, 
                      WStackingFilter *filt, void *filt_data)
{
    WStacking *st, *stnext, *chain=NULL;
    Window ref=other;
    int mode2=mode;
    
    assert(mode==Above || mode==Below);
    
    for(st=*stacking; st!=NULL; st=stnext){
        stnext=st->next;
        
        if(cf(filt, filt_data, st->reg)){
            Window bottom=None, top=None;
            
            assert(st!=other_on_list);

            region_restack(st->reg, ref, mode2);
            region_stacking(st->reg, &bottom, &top);
            if(top!=None){
                ref=top;
                mode2=Above;
            }
            
            UNLINK_ITEM(*stacking, st, next, prev);
            LINK_ITEM(chain, st, next, prev);
        }
    }
    
    if(chain==NULL)
        return;
    
    if(*stacking==NULL){
        *stacking=chain;
        return;
    }
    
    if(other_on_list==NULL){
        if(mode==Above)
            *stacking=link_lists(*stacking, chain);
        else
            *stacking=link_lists(chain, *stacking);
    }else if(other_on_list!=NULL){
        if(mode==Above)
            *stacking=link_list_before(*stacking, other_on_list, chain);
        else
            *stacking=link_list_after(*stacking, other_on_list, chain);
    }
}

/*}}}*/


/*{{{ Find top/bottom */

void stacking_stacking(WStacking *stacking, Window *bottomret, Window *topret,
                       WStackingFilter *filt, void *filt_data)
{
    WStacking *st;
    
    /* Ignore dummywin if we manage anything in order to not confuse 
     * the global stacking list 
     */
    
    *topret=None;
    *bottomret=None;
    
    st=stacking->prev;
    
    while(1){
        Window bottom=None, top=None;
        if(cf(filt, filt_data, st->reg)){
            region_stacking(st->reg, &bottom, &top);
            if(top!=None){
                *topret=top;
                break;
            }
        }
        if(st==stacking)
            break;
        st=st->prev;
    }
    
    for(st=stacking; st!=NULL; st=st->next){
        Window bottom=None, top=None;
        if(filt==NULL || filt(st->reg, filt_data)){
            region_stacking(st->reg, &bottom, &top);
            if(bottom!=None){
                *bottomret=top;
                break;
            }
        }
    }
}


/*}}}*/


/*{{{ Raise/lower */


static void restack_above(WStacking **stacking, WStacking *regst,
                          Window fb_other)
{
    Window bottom, top, other;
    WStacking *stabove, *stnext;
    WStacking *sttop;

    region_stacking(regst->reg, &bottom, &top);
    
    other=(top==None ? fb_other : top);
    
    if(other==None)
        return;
    
    sttop=regst;
    
    for(stabove=*stacking; stabove!=NULL; stabove=stnext){
        stnext=stabove->next;
        
        if(stabove==regst)
            continue;
        
        if(stabove->above==regst){
            UNLINK_ITEM(*stacking, stabove, next, prev);
            region_restack(stabove->reg, other, Above);
            LINK_ITEM_AFTER(*stacking, sttop, stabove, next, prev);
            region_stacking(stabove->reg, &bottom, &top);
            if(top!=None)
                other=top;
            sttop=stabove;
        }
    }
}


void stacking_do_raise(WStacking **stacking, WRegion *reg, bool initial,
                       Window fb_win,
                       WStackingFilter *filt, void *filt_data)
{
    WStacking *regst, *st, *sttop=NULL;
    Window bottom=None, top=None, other=None;
    
    if(*stacking==NULL)
        return;
    
    regst=stacking_find(*stacking, reg);
    
    if(regst==NULL)
        return;

    /* Find top and bottom of everything above regst except stuff to
     * be stacked above it.
     */
    for(st=(*stacking)->prev; st!=*stacking; st=st->prev){
        if(st==regst)
            break;
        if(st->above!=regst && cf(filt, filt_data, st->reg)){
            region_stacking(st->reg, &bottom, &top);
            if(top!=None){
                other=top;
                sttop=st;
                break;
            }
        }
    }

    /* Restack reg */
    
    if(sttop!=NULL){
        UNLINK_ITEM(*stacking, regst, next, prev);
        region_restack(reg, other, Above);
        LINK_ITEM_AFTER(*stacking, sttop, regst, next, prev);
    }else if(initial){
        other=fb_win;
        region_restack(reg, other, Above);
    }
    
    if(!initial){
        /* Restack stuff above reg */
        restack_above(stacking, regst, other);
    }
}


void stacking_do_lower(WStacking **stacking, WRegion *reg, Window fb_win,
                       WStackingFilter *filt, void *filt_data)
{
    WStacking *regst=NULL, *st, *stbottom=NULL;
    Window bottom=None, top=None, other=None;
    
    if(*stacking==NULL)
        return;

    for(st=*stacking; st!=NULL; st=st->next){
        if(st->reg==reg){
            regst=st;
            break;
        }
        if(stbottom==NULL && cf(filt, filt_data, st->reg)){
            region_stacking(st->reg, &bottom, &top);
            if(bottom!=None){
                other=bottom;
                stbottom=st;
            }
        }
    }
    
    if(regst!=NULL){
        if(stbottom==NULL){
            region_restack(reg, fb_win, Above);
        }else{
            UNLINK_ITEM(*stacking, regst, next, prev);
            region_restack(reg, other, Below);
            LINK_ITEM_BEFORE(*stacking, stbottom, regst, next, prev);
        }
    }
}


/*}}}*/


/*{{{ Stacking lists */


WStacking **window_get_stackingp(WWindow *wwin)
{
    return &(wwin->stacking);
}


WStacking *window_get_stacking(WWindow *wwin)
{
    return wwin->stacking;
}


/*}}}*/


/*{{{ Stacking list iteration */


void stacking_iter_init(WStackingIterTmp *tmp, 
                        WStacking *st,
                        WStackingFilter *filt,
                        void *filt_data)
{
    tmp->st=st;
    tmp->filt=filt;
    tmp->filt_data=filt_data;
}


WStacking *stacking_iter_nodes(WStackingIterTmp *tmp)
{
    WStacking *next=NULL;
    
    while(tmp->st!=NULL){
        next=tmp->st;
        tmp->st=tmp->st->next;
        if(tmp->filt==NULL || tmp->filt(next->reg, tmp->filt_data))
            break;
        next=NULL;
    }
    
    return next;
}


WRegion *stacking_iter(WStackingIterTmp *tmp)
{
    WStacking *st=stacking_iter_nodes(tmp);
    return (st!=NULL ? st->reg : NULL);
}

    
/*}}}*/

