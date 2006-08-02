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
#include "sizepolicy.h"


/*{{{ Alloc */


WStacking *create_stacking()
{
    WStacking *st=ALLOC(WStacking);
    
    if(st!=NULL){
        st->reg=NULL;
        st->above=NULL;
        st->level=0;
        st->szplcy=SIZEPOLICY_DEFAULT;
        st->llist_next=NULL;
        st->llist_prev=NULL;
        st->llist_phs=NULL;
    }
    
    return st;
}


/*}}}*/


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


WStacking *stacking_find_mgr(WStacking *st, WRegion *reg)
{
    while(st!=NULL){
        if(st->reg==reg)
            return st;
        st=st->mgr_next;
    }
    
    return NULL;
}


WStacking *stacking_unstack(WWindow *par, WStacking *regst)
{
    WStacking *nxt=NULL, *st;
    
    /*st=regst->next;*/
    
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


static bool cf(WStackingFilter *filt, void *filt_data, WStacking *st)
{
    return (filt==NULL || filt(st, filt_data));
}


static bool check_unweave(WStacking *st)
{
    /* 2: unknown, 1: yes, 0: no */
    
    if(st->to_unweave==2){
        if(st->above!=NULL)
            st->to_unweave=check_unweave(st->above);
        else
            st->to_unweave=0;
    }
    
    return st->to_unweave;
}


WStacking *stacking_unweave(WStacking **stacking, 
                            WStackingFilter *filt, void *filt_data)
{
    WStacking *np=NULL;
    WStacking *st, *next;

    /* TODO: above-list? */
    
    for(st=*stacking; st!=NULL; st=st->next){
        st->to_unweave=2;
        if(st->above==NULL && cf(filt, filt_data, st))
            st->to_unweave=1;
    }
    
    for(st=*stacking; st!=NULL; st=st->next)
        check_unweave(st);
    
    for(st=*stacking; st!=NULL; st=next){
        next=st->next;
        if(st->to_unweave==1){
            UNLINK_ITEM(*stacking, st, next, prev);
            LINK_ITEM(np, st, next, prev);
        }
    }
    
    return np;
}


static int check_above_lvl(WStacking *st)
{
    if(st->above==NULL)
        return st->level;
    st->level=check_above_lvl(st->above);
    return st->level;
}

    
static void enforce_level_sanity(WStacking **np)
{
    WStacking *st;
    
    /* Make sure that the levels of stuff stacked 'above' match
     * the level of the thing stacked above.
     */
    for(st=*np; st!=NULL; st=st->next)
        check_above_lvl(st);

    /* And now make sure things are ordered by levels. */
    st=*np; 
    while(st->next!=NULL){
        if(st->next->level < st->level){
            WStacking *st2=st->next;
            UNLINK_ITEM(*np, st2, next, prev);
            LINK_ITEM_BEFORE(*np, st2, st, next, prev);
            if(st2->prev!=NULL)
                st=st2->prev;
        }else{
            st=st->next;
        }
    }
}


static void get_bottom(WStacking *st, Window fb_win,
                       Window *other, int *mode)
{
    Window bottom=None, top=None;
    
    while(st!=NULL){
        if(st->reg!=NULL){
            region_stacking(st->reg, &bottom, &top);
            if(bottom!=None){
                *other=bottom;
                *mode=Below;
                return;
            }
        }
        st=st->next;
    }
    
    *other=fb_win;
    *mode=Above;
}


static void stacking_do_weave(WStacking **stacking, WStacking **np, 
                              bool below, Window fb_win)
{
    WStacking *st, *ab;
    uint lvl;
    Window other;
    int mode;

    if(*np==NULL)
        return;
    
    /* Should do nothing.. */
    enforce_level_sanity(np);
    
    ab=*stacking;
    
    while(*np!=NULL){
        lvl=(*np)->level;
        
        while(ab!=NULL){
            if(ab->level>lvl || (below && ab->level==lvl))
                break;
            ab=ab->next;
        }
        get_bottom(ab, fb_win, &other, &mode);
        
        st=*np;

        UNLINK_ITEM(*np, st, next, prev);
        
        region_restack(st->reg, other, mode);

        if(ab!=NULL){
            LINK_ITEM_BEFORE(*stacking, ab, st, next, prev);
        }else{
            LINK_ITEM_LAST(*stacking, st, next, prev);
        }
    }
}


void stacking_weave(WStacking **stacking, WStacking **np, bool below)
{
    stacking_do_weave(stacking, np, below, None);
}


/*}}}*/


/*{{{ Restack */


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
        
        if(cf(filt, filt_data, st)){
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
        if(cf(filt, filt_data, st)){
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
        if(cf(filt, filt_data, st)){
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


static bool is_above(WStacking *st, WStacking *p)
{
    if(st->above==NULL)
        return FALSE;
    else if(st->above==p)
        return TRUE;
    else
        return is_above(st->above, p);
}


static void collect_above(WStacking **dst, WStacking **src, WStacking *regst)
{
    WStacking *stabove, *stnext;
    
    for(stabove=*src; stabove!=NULL; stabove=stnext){
        stnext=stabove->next;
        
        if(is_above(stabove, regst)){
            UNLINK_ITEM(*src, stabove, next, prev);
            LINK_ITEM_LAST(*dst, stabove, next, prev);
        }
    }
}


static WStacking *unweave_subtree(WStacking **stacking, WStacking *regst)
{
    WStacking *tmp=NULL;
    
    UNLINK_ITEM(*stacking, regst, next, prev);
    LINK_ITEM(tmp, regst, next, prev);
    
    collect_above(&tmp, stacking, regst);
    
    return tmp;
}


void stacking_do_raise(WStacking **stacking, WRegion *reg, Window fb_win,
                       WStackingFilter *filt, void *filt_data)
{
    WStacking *regst, *tmp;
    
    regst=stacking_find(*stacking, reg);
    
    if(regst==NULL)
        return;
    
    tmp=unweave_subtree(stacking, regst);
    
    stacking_do_weave(stacking, &tmp, FALSE, fb_win);
    
    assert(tmp==NULL);
}


void stacking_do_lower(WStacking **stacking, WRegion *reg, Window fb_win,
                       WStackingFilter *filt, void *filt_data)
{
    WStacking *regst, *tmp;
    
    regst=stacking_find(*stacking, reg);
    
    if(regst==NULL)
        return;

#warning "TODO: special handling of regst->above!=NULL?"

    tmp=unweave_subtree(stacking, regst);
    
    stacking_do_weave(stacking, &tmp, TRUE, fb_win);
    
    assert(tmp==NULL);
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
        if(cf(tmp->filt, tmp->filt_data, next))
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


void stacking_iter_mgr_init(WStackingIterTmp *tmp, 
                            WStacking *st,
                            WStackingFilter *filt,
                            void *filt_data)
{
    tmp->st=st;
    tmp->filt=filt;
    tmp->filt_data=filt_data;
}


WStacking *stacking_iter_mgr_nodes(WStackingIterTmp *tmp)
{
    WStacking *next=NULL;
    
    while(tmp->st!=NULL){
        next=tmp->st;
        tmp->st=tmp->st->mgr_next;
        if(cf(tmp->filt, tmp->filt_data, next))
            break;
        next=NULL;
    }
    
    return next;
}


WRegion *stacking_iter_mgr(WStackingIterTmp *tmp)
{
    WStacking *st=stacking_iter_mgr_nodes(tmp);
    return (st!=NULL ? st->reg : NULL);
}

    
/*}}}*/


/*{{{ Focus */


WStacking *stacking_find_to_focus(WStacking *stacking, WStacking *to_try,
                                  WStackingFilter *include_filt, 
                                  WStackingFilter *approve_filt, 
                                  void *filt_data)
{
    WStacking *st=NULL;
    uint min_level=0;
    
    if(stacking==NULL)
        return NULL;
    
    st=stacking;
    do{
        st=st->prev;
        
        if(st->reg!=NULL 
           && !(st->reg->flags&REGION_SKIP_FOCUS)
           && cf(include_filt, filt_data, st)){
            
            if(st->level<min_level){
                return NULL;
            }else if(st->level>=STACKING_LEVEL_MODAL1){
                if(to_try!=NULL && to_try->level>=st->level)
                    return to_try;
                min_level=st->level;
            }else{
                if(to_try!=NULL)
                    return to_try;
            }
            
            if(cf(approve_filt, filt_data, st))
                return st;
        }
    }while(st!=stacking);
    
    return NULL;
}


/*}}}*/
