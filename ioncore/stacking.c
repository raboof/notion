/*
 * ion/ioncore/stacking.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2008.
 *
 * See the included file LICENSE for details.
 */

#include <libtu/rb.h>
#include <libtu/minmax.h>

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
        st->hidden=FALSE;
        st->lnode=NULL;
        st->pseudomodal=FALSE;
    }
    
    return st;
}


void stacking_free(WStacking *st)
{
    assert(st->mgr_next==NULL && st->mgr_prev==NULL &&
           st->next==NULL && st->prev==NULL &&
           /*st->above==NULL &&*/
           st->lnode==NULL &&
           st->reg==NULL);
    
    free(st);
}


/*}}}*/


/*{{{ Lookup */


static Rb_node stacking_of_reg=NULL;


WStacking *ioncore_find_stacking(WRegion *reg)
{
    Rb_node node=NULL;
    int found=0;
    
    if(stacking_of_reg!=NULL)
        node=rb_find_pkey_n(stacking_of_reg, reg, &found);
    
    return (found ? (WStacking*)node->v.val : NULL);
}


void stacking_unassoc(WStacking *st)
{
    Rb_node node=NULL;
    int found=0;
    
    if(st->reg==NULL)
        return;
    
    if(stacking_of_reg!=NULL)
        node=rb_find_pkey_n(stacking_of_reg, st->reg, &found);
    
    if(node!=NULL)
        rb_delete_node(node);
    
    st->reg=NULL;
}


bool stacking_assoc(WStacking *st, WRegion *reg)
{
    assert(st->reg==NULL);
    
    if(stacking_of_reg==NULL){
        stacking_of_reg=make_rb();
        if(stacking_of_reg==NULL)
            return FALSE;
    }

    if(rb_insertp(stacking_of_reg, reg, st)==NULL)
        return FALSE;
    
    st->reg=reg;
    return TRUE;
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
    
    if(nxt==NULL)
        nxt=regst->above;
    
    if(regst->above==NULL)
        regst->above=NULL;
    
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


static void collect_first(WStacking **dst, WStacking **src, WStacking *st)
{
    UNLINK_ITEM(*src, st, next, prev);
    LINK_ITEM_FIRST(*dst, st, next, prev);
}


static void collect_last(WStacking **dst, WStacking **src, WStacking *st)
{
    UNLINK_ITEM(*src, st, next, prev);
    LINK_ITEM_LAST(*dst, st, next, prev);
}


static void collect_above(WStacking **dst, WStacking **src, WStacking *regst)
{
    WStacking *stabove, *stnext;
    
    for(stabove=*src; stabove!=NULL; stabove=stnext){
        stnext=stabove->next;
        
        if(is_above(stabove, regst))
            collect_last(dst, src, stabove);
    }
}


static WStacking *unweave_subtree(WStacking **stacking, WStacking *regst,
                                  bool parents)
{
    WStacking *tmp=NULL;
    
    if(parents){
        WStacking *st=regst;
        while(st!=NULL){
            collect_first(&tmp, stacking, st);
            st=st->above;
        }
    }else{
        collect_first(&tmp, stacking, regst);
    }
    
    collect_above(&tmp, stacking, regst);
    
    return tmp;
}


void stacking_restack(WStacking **stacking, WStacking *st, Window fb_win,
                      WStackingFilter *filt, void *filt_data, bool lower)
{
    WStacking *tmp=unweave_subtree(stacking, st, lower);

    stacking_do_weave(stacking, &tmp, lower, fb_win);

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


uint stacking_min_level(WStacking *stacking, 
                        WStackingFilter *include_filt, 
                        void *filt_data)
{
    uint min_level=STACKING_LEVEL_BOTTOM;
    WStacking *st=NULL;
    
    if(stacking==NULL)
        return STACKING_LEVEL_BOTTOM;
    
    st=stacking;
    do{
        st=st->prev;
        
        if(st->reg!=NULL 
           && !(st->reg->flags&REGION_SKIP_FOCUS)
           && cf(include_filt, filt_data, st)){
            
            if(st->level>=STACKING_LEVEL_MODAL1)
                min_level=st->level;
            
            break;
        }
    }while(st!=stacking);
    
    return min_level;
}


WStacking *stacking_find_to_focus(WStacking *stacking, 
                                  WStacking *to_try,
                                  WStackingFilter *include_filt, 
                                  WStackingFilter *approve_filt, 
                                  void *filt_data)
{
    uint min_level=STACKING_LEVEL_BOTTOM;
    WStacking *st=NULL, *found=NULL;
    
    if(stacking==NULL)
        return NULL;
    
    st=stacking;
    do{
        st=st->prev;
        
        if(st->reg==NULL)
            continue;
        
        if(st!=to_try && (st->reg->flags&REGION_SKIP_FOCUS ||
                          !cf(include_filt, filt_data, st))){
            /* skip */
            continue;
        }
        
        if(st->level<min_level)
            break; /* no luck */
        
        if(st==to_try)
            return st;
            
        if(found==NULL && cf(approve_filt, filt_data, st)){
            found=st;
            if(to_try==NULL)
                break;
        }
            
        if(st->level>=STACKING_LEVEL_MODAL1)
            min_level=maxof(min_level, st->level);
    }while(st!=stacking);
    
    return found;
}


static bool mapped_filt(WStacking *st, void *unused)
{
    return (st->reg!=NULL && REGION_IS_MAPPED(st->reg));
}


static bool mapped_filt_neq(WStacking *st, void *st_neq)
{
    return (st!=(WStacking*)st_neq && mapped_filt(st, NULL));
}


static bool mgr_filt(WStacking *st, void *mgr_)
{
    return (st->reg!=NULL && REGION_MANAGER(st->reg)==(WRegion*)mgr_);
}


WStacking *stacking_find_to_focus_mapped(WStacking *stacking, 
                                         WStacking *to_try,
                                         WRegion *mgr)
{
    if(mgr==NULL){
        return stacking_find_to_focus(stacking, to_try, mapped_filt, 
                                      NULL, NULL);
    }else{
        return stacking_find_to_focus(stacking, to_try, mapped_filt, 
                                      mgr_filt, mgr);
    }
}


uint stacking_min_level_mapped(WStacking *stacking)
{
    return stacking_min_level(stacking, mapped_filt, NULL);
}


bool stacking_must_focus(WStacking *stacking, WStacking *st)
{
    WStacking *stf=stacking_find_to_focus(stacking, NULL, 
                                          mapped_filt_neq, NULL, st);
    
    return (stf==NULL || 
            (st->level>stf->level &&
             st->level>=STACKING_LEVEL_MODAL1));
}


/*}}}*/

