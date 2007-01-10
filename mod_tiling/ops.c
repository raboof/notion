/*
 * ion/mod_tiling/ops.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <ioncore/common.h>
#include <ioncore/mplex.h>
#include <ioncore/focus.h>
#include <ioncore/return.h>
#include <ioncore/group.h>
#include "tiling.h"


static WRegion *mkbottom_fn(WWindow *parent, const WFitParams *fp, 
                            void *param)
{
    WRegion *reg=(WRegion*)param;
    WTiling *tiling;
    WSplitRegion *node=NULL;
    
    if(!region_fitrep(reg, parent, fp))
        return NULL;
    
    tiling=create_tiling(parent, fp, NULL, FALSE);
    
    if(tiling==NULL)
        return NULL;
    
    node=create_splitregion(&REGION_GEOM(tiling), reg);
    if(node!=NULL){
        tiling->split_tree=(WSplit*)node;
        tiling->split_tree->ws_if_root=tiling;
        
        region_detach_manager(reg);
        
        if(tiling_managed_add(tiling, reg))
            return (WRegion*)tiling;
        
        #warning "TODO: reattach?"
        
        destroy_obj((Obj*)tiling->split_tree);
        tiling->split_tree=NULL;
    }
    
    destroy_obj((Obj*)tiling);
    return NULL;
}


/*EXTL_DOC
 * Create a new \type{WTiling} 'bottom' for the group of \var{reg},
 * consisting of \var{reg}.
 */
EXTL_EXPORT
bool mod_tiling_mkbottom(WRegion *reg)
{
    WGroup *grp=REGION_MANAGER_CHK(reg, WGroup);
    WGroupAttachParams ap=GROUPATTACHPARAMS_INIT;
    WRegionAttachData data;
    WRegion *tiling;
    
    if(grp==NULL){
        warn(TR("Not member of a group"));
        return FALSE;
    }
    
    if(grp->bottom!=NULL){
        warn(TR("Manager group already has bottom"));
        return FALSE;
    }
    
    ap.level_set=TRUE;
    ap.level=STACKING_LEVEL_BOTTOM;
    
    ap.szplcy_set=TRUE;
    ap.szplcy=SIZEPOLICY_FULL_EXACT;
    
    ap.switchto_set=TRUE;
    ap.switchto=region_may_control_focus(reg);
    
    ap.bottom=TRUE;

    data.type=REGION_ATTACH_NEW;
    data.u.n.fn=mkbottom_fn;
    data.u.n.param=reg;
    
    /* kele... poisto samalla kuin attach */
    return (group_do_attach(grp, &ap, &data)!=NULL);
}
