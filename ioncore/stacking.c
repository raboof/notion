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


static void get_bottom(WStacking *st, Window *other, int *mode)
{
    Window bottom, top;
    
    while(st!=NULL){
        if(st->reg!=NULL){
            region_stacking(st->reg, &bottom, &top);
            if(bottom!=None){
                *other=bottom;
                *mode=Below;
            }
        }
    }
    
    *other=None;
    *mode=Above;
}


void stacking_weave(WStacking **stacking, WStacking **np, bool below)
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
        get_bottom(ab, &other, &mode);
        
        while(1){
            st=*np;
            if(st==NULL || st->level>lvl)
                break;
            UNLINK_ITEM(*np, st, next, prev);
            if(ab!=NULL){
                LINK_ITEM_BEFORE(*stacking, ab, st, next, prev);
            }else{
                LINK_ITEM_LAST(*stacking, st, next, prev);
            }
            region_restack(st->reg, other, mode);
        }
    }
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


static void restack_above(WStacking **stacking, WStacking *regst,
                          Window fb_other, int fb_mode)
{
    Window bottom, top, other;
    WStacking *stabove, *stnext;
    WStacking *sttop;
    int mode;

    region_stacking(regst->reg, &bottom, &top);
    
    if(top==None){
        other=fb_other;
        mode=fb_mode;
    }else{
        other=top;
        mode=Above;
    }
    
    if(other==None)
        return;
    
    sttop=regst;
    
    for(stabove=*stacking; stabove!=NULL; stabove=stnext){
        stnext=stabove->next;
        
        if(stabove==regst)
            continue;
        
        if(stabove->above==regst){
            UNLINK_ITEM(*stacking, stabove, next, prev);
            region_restack(stabove->reg, other, mode);
            LINK_ITEM_AFTER(*stacking, sttop, stabove, next, prev);
            region_stacking(stabove->reg, &bottom, &top);
            if(top!=None){
                other=top;
                mode=Above;
            }
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
    int mode=Above;
    
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
        if(st->above!=regst && cf(filt, filt_data, st)){
            region_stacking(st->reg, &bottom, &top);
            if(st->level>regst->level){
                if(bottom!=None){
                    other=bottom;
                    mode=Below;
                    sttop=st;
                }
            }else{
                if(top!=None){
                    other=top;
                    mode=Above;
                    sttop=st;
                    break;
                }
            }
        }
    }

    /* Restack reg */
    
    if(sttop!=NULL){
        UNLINK_ITEM(*stacking, regst, next, prev);
        region_restack(reg, other, mode);
        if(mode==Above){
            LINK_ITEM_AFTER(*stacking, sttop, regst, next, prev);
        }else{
            LINK_ITEM_BEFORE(*stacking, sttop, regst, next, prev);
        }
    }else if(initial){
        other=fb_win;
        mode=Above;
        region_restack(reg, other, mode);
    }
    
    if(!initial){
        /* Restack stuff above reg */
        restack_above(stacking, regst, other, mode);
    }
}


void stacking_do_lower(WStacking **stacking, WRegion *reg, Window fb_win,
                       WStackingFilter *filt, void *filt_data)
{
    WStacking *regst=NULL, *st, *stbottom=NULL;
    Window bottom=None, top=None, other=None;
    int mode=Below;
    
    if(*stacking==NULL)
        return;
    
    regst=stacking_find(*stacking, reg);
    
    if(regst==NULL)
        return;

    for(st=*stacking; st!=NULL; st=st->next){
        if(st==regst)
            break;
        
        if(st->above!=regst && cf(filt, filt_data, st)){
            region_stacking(st->reg, &bottom, &top);
            if(st->level<regst->level){
                if(top!=None){
                    other=top;
                    mode=Above;
                    stbottom=st;
                }
            }else{
                if(bottom!=None){
                    other=bottom;
                    mode=Below;
                    stbottom=st;
                    break;
                }
            }
        }
    }
    
    if(stbottom!=NULL){
        UNLINK_ITEM(*stacking, regst, next, prev);
        region_restack(reg, other, mode);
        if(mode==Above){
            LINK_ITEM_AFTER(*stacking, stbottom, regst, next, prev);
        }else{
            LINK_ITEM_BEFORE(*stacking, stbottom, regst, next, prev);
        }
    }else{
        other=fb_win;
        mode=Above;
        region_restack(reg, other, mode);
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

