/*
 * ion/ioncore/stacking.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/objp.h>
#include "common.h"
#include "region.h"
#include "window.h"
#include "stacking.h"
#include "global.h"
#include "region-iter.h"
#include "xwindow.h"


/*
 * Some notes on the stack lists:
 * 
 *  * "Normally" stacked regions are not on any list.
 *  * Regions that are stacked above some other region stacking.above are on
 *    stacking.above's stacking.stack_below_list.
 *  * Regions that are 'kept on top' inside their parent window or on the
 *    window's keep_on_top_list and have the flag REGION_KEEPONTOP set.
 */


/*{{{ XQueryTree code */


static Window find_lowest_keep_on_top(WWindow *par)
{
    Window root=None, parent=None, *children=NULL;
    int nchildren=0, i=0;
    Window lowest=None;
    
    if(par->keep_on_top_list==NULL)
        return None;
    
    if(par->keep_on_top_list->stacking.next==NULL)
        return region_xwindow(par->keep_on_top_list);
    
    XQueryTree(ioncore_g.dpy, region_xwindow((WRegion*)par),
               &root, &parent, &children, (uint*)&nchildren);
    
    for(i=0; i<nchildren; i++){
        WRegion *other=XWINDOW_REGION_OF(children[i]);

        if(other!=NULL && other->flags&REGION_KEEPONTOP){
            lowest=children[i];
            break;
        }
    }
    
    XFree(children);
    
    return lowest;
}


static Window find_highest_normal(WWindow *par)
{
    Window root=None, parent=None, *children=NULL;
    int nchildren=0, i=0;
    Window highest=None;
    
    XQueryTree(ioncore_g.dpy, region_xwindow((WRegion*)par),
               &root, &parent, &children, (uint*)&nchildren);
    
    for(i=nchildren-1; i>=0; i--){
        WRegion *other=XWINDOW_REGION_OF(children[i]);
        
        if(other!=NULL && !(other->flags&REGION_KEEPONTOP)){
            highest=children[i];
            break;
        }
    }
    
    XFree(children);
    
    return highest;
}


static void do_raise_tree(WWindow *par, WRegion *reg, Window sibling, int mode)
{
    Window root=None, parent=None, *children=NULL;
    int nchildren=0, i=0;
    
    XQueryTree(ioncore_g.dpy, region_xwindow((WRegion*)par),
               &root, &parent, &children, (uint*)&nchildren);
    
    for(i=nchildren-1; i>=0; i--){
        WRegion *other=XWINDOW_REGION_OF(children[i]);
        WRegion *other2=other;
        
        while(other2!=NULL && other2!=reg)
            other2=other2->stacking.above;
        
        if(other2==reg){
            region_restack(other, sibling, mode);
            sibling=children[i];
            mode=Below;
        }
    }
    
    XFree(children);
}


static void do_lower_tree(WWindow *par, WRegion *reg, Window sibling, int mode)
{
    Window root=None, parent=None, *children=NULL;
    int nchildren=0, i=0;
    
    XQueryTree(ioncore_g.dpy, region_xwindow((WRegion*)par),
               &root, &parent, &children, (uint*)&nchildren);
    
    for(i=0; i<nchildren; i++){
        WRegion *other=XWINDOW_REGION_OF(children[i]);
        WRegion *other2=other;
        
        while(other2!=NULL && other2!=reg)
            other2=other2->stacking.above;
        
        if(other2==reg){
            region_restack(other, sibling, mode);
            sibling=children[i];
            mode=Above;
        }
    }
    
    XFree(children);
}


/*}}}*/


/*{{{ Stack above */


/*EXTL_DOC
 * Inform that \var{reg} should be stacked above the region \var{above}.
 */
EXTL_EXPORT_MEMBER
bool region_stack_above(WRegion *reg, WRegion *above)
{
    WRegion *r2;
    WWindow *par=REGION_PARENT_CHK(reg, WWindow);
    Window abovewin=region_xwindow(above);

    if(above==NULL || reg==above || par==NULL)
        return FALSE;

    if((WRegion*)par!=REGION_PARENT(above))
        return FALSE;
    
    if(region_xwindow(reg)==None || abovewin==None)
        return FALSE;
    
    region_reset_stacking(reg);
    
    do_raise_tree(par, reg, region_xwindow(above), Above);
    
    LINK_ITEM(above->stacking.below_list, reg, stacking.next, stacking.prev);
    reg->stacking.above=above;
    
    return TRUE;
}


/*}}}*/


/*{{{ Keep on top */


/*EXTL_DOC
 * Inform that \var{reg} should be kept above normally stacked regions
 * within its parent.
 */
EXTL_EXPORT_MEMBER
void region_keep_on_top(WRegion *reg)
{
    WWindow *par;

    if(region_xwindow(reg)==None){
        warn("Stack-managed regions must have a window associated to them.");
        return;
    }
    
    par=REGION_PARENT_CHK(reg, WWindow);
    
    if(par==NULL)
        return;
    
    region_reset_stacking(reg);

    region_restack(reg, None, Above);
    
    LINK_ITEM(par->keep_on_top_list, reg, stacking.next, stacking.prev);
    reg->flags|=REGION_KEEPONTOP;
}


/*}}}*/


/*{{{ Reset */


static void do_reset_stacking(WRegion *reg)
{
    if(reg->stacking.above){
        UNLINK_ITEM(reg->stacking.above->stacking.below_list,
                    reg, stacking.next, stacking.prev);
        reg->stacking.above=NULL;
    }else if(reg->flags&REGION_KEEPONTOP){
        WWindow *par=REGION_PARENT_CHK(reg, WWindow);
        assert(par!=NULL);
        UNLINK_ITEM(par->keep_on_top_list, reg, stacking.next, stacking.prev);
        reg->flags&=~REGION_KEEPONTOP;
    }
}


/*EXTL_DOC
 * Inform that \var{reg} should be stacked normally.
 */
EXTL_EXPORT_MEMBER
void region_reset_stacking(WRegion *reg)
{
    WRegion *r2, *next;
    
    do_reset_stacking(reg);
    
    for(r2=reg->stacking.below_list; r2!=NULL; r2=next){
        next=r2->stacking.next;
        do_reset_stacking(r2);
    }
}
        

/*}}}*/


/*{{{ Raise/lower/init */


/*EXTL_DOC
 * Raise \var{reg} in the stack. The regions marked to be stacked above
 * \var{reg} will also be raised and normally stacked regions will not
 * be raised above region marked to be kept on top.
 */
EXTL_EXPORT_MEMBER
void region_raise(WRegion *reg)
{
    WRegion *r2=NULL;
    WWindow *par=REGION_PARENT_CHK(reg, WWindow);
    Window sibling=None;
    int mode=Above;

    if(par==NULL)
        return;
    
    r2=reg;
    while(r2->stacking.above!=NULL){
        r2=r2->stacking.above;
        assert(r2!=reg);
    }

    if(!(r2->flags&REGION_KEEPONTOP)){
        sibling=find_lowest_keep_on_top(par);
        if(sibling!=None)
            mode=Below;
    }

    do_raise_tree(par, reg, sibling, mode);
}


void window_init_sibling_stacking(WWindow *par, Window win)
{
    Window below=find_lowest_keep_on_top(par);
    if(below!=None)
        xwindow_restack(win, below, Below);
}


/*EXTL_DOC
 * Lower \var{reg} in the stack. The regions marked to be stacked above
 * \var{reg} will also be lowerd and regions marked to be kept on top
 * will not be lowered below normally stacked regions.
 */
EXTL_EXPORT_MEMBER
void region_lower(WRegion *reg)
{
    WRegion *r2=NULL;
    WWindow *par=REGION_PARENT_CHK(reg, WWindow);
    Window sibling=None;
    int mode=Below;

    if(par==NULL)
        return;

    r2=reg;
    while(r2->stacking.above!=NULL){
        r2=r2->stacking.above;
        assert(r2!=reg);
    }
    
    if(r2->flags&REGION_KEEPONTOP){
        sibling=find_highest_normal(par);
        if(sibling!=None)
            mode=Above;
    }
    
    do_lower_tree(par, r2, sibling, mode);
}


/* region_restack should return the topmost window */
Window region_restack(WRegion *reg, Window other, int mode)
{
    Window ret=None;
    CALL_DYN_RET(ret, Window, region_restack, reg, (reg, other, mode));
    return ret;
}


/*}}}*/

