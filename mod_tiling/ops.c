/*
 * ion/mod_tiling/ops.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#include <libtu/objp.h>
#include <ioncore/common.h>
#include <ioncore/mplex.h>
#include <ioncore/focus.h>
#include <ioncore/return.h>
#include <ioncore/group.h>
#include "tiling.h"


/*{{{ mkbottom */


static WRegion *mkbottom_fn(WWindow *parent, const WFitParams *fp, 
                            void *param)
{
    WRegion *reg=(WRegion*)param, *res;
    WRegionAttachData data;
    WTiling *tiling;
    WFitParams fp2;
    
    fp2.mode=REGION_FIT_EXACT;
    fp2.g=fp->g;
    
    tiling=create_tiling(parent, &fp2, NULL, FALSE);
    
    if(tiling==NULL)
        return NULL;
        
    data.type=REGION_ATTACH_REPARENT;
    data.u.reg=reg;
    
    /* Warning! Potentially dangerous call to remove a `reg` from the same
     * group we're being attached to, and from the attach routine of which
     * this function is called from!
     */
    res=region_attach_helper((WRegion*)tiling, parent, &fp2,
                             (WRegionDoAttachFn*)tiling_do_attach_initial, 
                             NULL, &data);
    
    if(res==NULL){
        destroy_obj((Obj*)tiling);
        return NULL;
    }
        
    return (WRegion*)tiling;
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
    
    if(grp==NULL){
        warn(TR("Not member of a group"));
        return FALSE;
    }
    
    if(group_bottom(grp)!=NULL){
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
    
    /* See the "Warning!" above. */
    return (group_do_attach(grp, &ap, &data)!=NULL);
}


/*}}}*/


/*{{{ untile */


/*EXTL_DOC
 * If \var{tiling} is managed by some group, float the frames in
 * the tiling in that group, and dispose of \var{tiling}.
 */
EXTL_EXPORT
bool mod_tiling_untile(WTiling *tiling)
{
    WGroup *grp=REGION_MANAGER_CHK(tiling, WGroup);
    WGroupAttachParams param=GROUPATTACHPARAMS_INIT;
    WTilingIterTmp tmp;
    WRegion *reg, *reg2;
    
    if(grp==NULL){
        warn(TR("Not member of a group"));
        return FALSE;
    }
    
    if(group_bottom(grp)==(WRegion*)tiling)
        group_set_bottom(grp, NULL);
    
    /* Setting `batchop` will stop `tiling_managed_remove` from 
     * resizing remaining frames into freed space. It will also 
     * stop the tiling from being destroyed by actions of
     * `tiling_managed_disposeroot`.
     */
    tiling->batchop=TRUE;
    
    FOR_ALL_MANAGED_BY_TILING(reg, tiling, tmp){
        WRegionAttachData data;
        
        /* Don't bother with the status display */
        if(reg==TILING_STDISP_OF(tiling))
            continue;
        
        /* Don't bother with regions containing no client windows. */
        if(!region_rescue_needed(reg))
            continue;
        
        data.type=REGION_ATTACH_REPARENT;
        data.u.reg=reg;
        
        param.geom_set=TRUE;
        param.geom=REGION_GEOM(reg);
        
        reg2=group_do_attach(grp, &param, &data);
        
        if(reg2==NULL)
            warn(TR("Unable to move a region from tiling to group."));
    }
    
    tiling->batchop=FALSE;
    
    region_dispose((WRegion*)tiling);
    
    return TRUE;
}


/*}}}*/

