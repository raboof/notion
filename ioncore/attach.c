/*
 * ion/ioncore/attach.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <limits.h>

#include "common.h"
#include "global.h"
#include "region.h"
#include "attach.h"
#include "clientwin.h"
#include "saveload.h"
#include "manage.h"
#include "extlconv.h"
#include "names.h"
#include "focus.h"


/*{{{ Helper */


static WRegion *doit_new(WRegion *mgr,
                         WWindow *par, const WFitParams *fp,
                         WRegionDoAttachFn *cont, void *cont_param,
                         WRegionCreateFn *fn, void *fn_param)
{
    WRegion *reg=fn(par, fp, fn_param);
    
    if(reg==NULL)
        return NULL;
        
    if(!cont(mgr, reg, cont_param)){
        destroy_obj((Obj*)reg);
        return NULL;
    }
    
    return reg;
}


static WRegion *doit_reparent(WRegion *mgr,
                              WWindow *par, const WFitParams *fp,
                              WRegionDoAttachFn *cont, void *cont_param,
                              WRegion *reg)
{
    WFitParams fp2;
    WRegion *disposeroot;
    
    if(!region_ancestor_check(mgr, reg)){
        warn(TR("Attempt to make region %s manage its ancestor %s."),
             region_name(mgr), region_name(reg));
        return NULL;
    }
    
    disposeroot=region_disposeroot(reg);
    
    if(disposeroot==NULL){
        /* Region may not be reparented */
        return NULL;
    }
    
    if(fp->mode&REGION_FIT_WHATEVER){
        /* fp->g is not final; substitute size with current to avoid
         * useless resizing. 
         */
        fp2.mode=fp->mode;
        fp2.g.x=fp->g.x;
        fp2.g.y=fp->g.y;
        fp2.g.w=REGION_GEOM(reg).w;
        fp2.g.h=REGION_GEOM(reg).h;
        fp=&fp2;
    }
    
    if(!region_fitrep(reg, par, fp)){
        warn(TR("Unable to reparent."));
        return NULL;
    }
    
    region_detach_manager(reg);
    
    if(!cont(mgr, reg, cont_param)){
        WScreen *scr=region_screen_of(reg);
        
        warn(TR("Unexpected attach error: "
                "trying to recover by attaching to screen."));
        
        if(scr!=NULL){
            /* Try to attach to screen, to have `reg` attached at least
             * somewhere. For better recovery, we could try to get
             * a placeholder for `reg` before we detach it, but this
             * would add unnecessary overhead in the usual succesfull
             * case. (This failure is supposed to be _very_ rare!)
             * We intentionally also do not region_postdetach_dispose 
             * on recovery.
             */
            int flags=(region_may_control_focus(reg) 
                       ? MPLEX_ATTACH_SWITCHTO 
                       : 0);
            if(mplex_attach_simple(&scr->mplex, reg, flags)!=NULL)
                return NULL;
        }
        
        warn(TR("Failed recovery."));
        
        return NULL;
    }
    
    region_postdetach_dispose(reg, disposeroot);
    
    return reg;
}


typedef struct{
    ExtlTab tab;
    WPHolder **sm_ph_p;
} WLP;

static WRegion *wrap_load(WWindow *par, const WFitParams *fp, WLP *p)
{
    return create_region_load(par, fp, p->tab, p->sm_ph_p);
}


WRegion *ioncore_newly_created=NULL;


static WRegion *doit_load(WRegion *mgr,
                          WWindow *par, const WFitParams *fp,
                          WRegionDoAttachFn *cont, void *cont_param,
                          const WRegionAttachData *data)
{
    WRegion *reg=NULL;
    
    if(extl_table_gets_o(data->u.tab, "reg", (Obj**)&reg)){
        if(!OBJ_IS(reg, WRegion))
            return FALSE;
    }/*else if(extl_table_is_bool_set(tab, "reg_use_new")){
        reg=ioncore_newly_created;
        if(reg==NULL)
            return NULL;
    }*/
    
    if(reg!=NULL){
        return doit_reparent(mgr, par, fp, cont, cont_param, reg);
    }else{
        WLP p;
        p.tab=data->u.tab;
        p.sm_ph_p=NULL;
        
        return doit_new(mgr, par, fp, cont, cont_param,
                        (WRegionCreateFn*)wrap_load, &p);
    }
}


WRegion *region_attach_load_helper(WRegion *mgr,
                                   WWindow *par, const WFitParams *fp,
                                   WRegionDoAttachFn *fn, void *fn_param,
                                   ExtlTab tab, WPHolder **sm_ph)
{
    WLP p;
    p.tab=tab;
    p.sm_ph_p=sm_ph;
    
    return doit_new(mgr, par, fp, fn, fn_param,
                    (WRegionCreateFn*)wrap_load, &p);
}                                   


WRegion *region_attach_helper(WRegion *mgr,
                              WWindow *par, const WFitParams *fp,
                              WRegionDoAttachFn *fn, void *fn_param,
                              const WRegionAttachData *data)
{
    if(data->type==REGION_ATTACH_NEW){
        return doit_new(mgr, par, fp, fn, fn_param, 
                        data->u.n.fn, data->u.n.param);
    }else if(data->type==REGION_ATTACH_LOAD){
        return doit_load(mgr, par, fp, fn, fn_param, data);
    }else if(data->type==REGION_ATTACH_REPARENT){
        return doit_reparent(mgr, par, fp, fn, fn_param, data->u.reg);
    }else{
        return NULL;
    }
}


/*}}}*/


/*{{{ Reparent check etc. */


bool region_ancestor_check(WRegion *dst, WRegion *reg)
{
    WRegion *reg2;
    
    /* Check that reg is not a parent or manager of mgr */
    for(reg2=dst; reg2!=NULL; reg2=REGION_MANAGER(reg2)){
        if(reg2==reg)
            return FALSE;
    }
    
    for(reg2=REGION_PARENT_REG(dst); reg2!=NULL; reg2=REGION_PARENT_REG(reg2)){
        if(reg2==reg)
            return FALSE;
    }

    return TRUE;
}


void region_postdetach_dispose(WRegion *reg, WRegion *disposeroot)
{
    /* disposeroot should be destroyed (as empty and useless) unless it 
     * still, in fact, is an ancestor of reg.
     */
    if(disposeroot!=reg && region_ancestor_check(reg, disposeroot))
        region_dispose(disposeroot);
}


/*}}}*/


