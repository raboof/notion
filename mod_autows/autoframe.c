/*
 * ion/mod_autows/autoframe.c
 *
 * Copyright (c) Tuomo Valkonen 2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <libtu/objp.h>
#include <libtu/minmax.h> 
#include <ioncore/common.h>
#include <ioncore/frame.h>
#include <ioncore/frame-draw.h>
#include <ioncore/framep.h>
#include <ioncore/saveload.h>
#include <ioncore/extl.h>
#include <ioncore/regbind.h>
#include <ioncore/defer.h>

#include "autoframe.h"
#include "main.h"


/*{{{ Destroy/create frame */


static bool autoframe_init(WAutoFrame *frame, WWindow *parent, 
                           const WFitParams *fp)
{
    /*WFitParams lazyfp;*/
    
    frame->last_fp=*fp;
    frame->autocreated=FALSE;
    
    /*if(fp->mode==REGION_FIT_EXACT){
        lazyfp=*fp;
    }else{
        lazyfp.mode=REGION_FIT_EXACT;
        lazyfp.g.w=minof(fp->g.w, CF_SCRATCHPAD_DEFAULT_W);
        lazyfp.g.h=minof(fp->g.h, CF_SCRATCHPAD_DEFAULT_H);
        lazyfp.g.x=fp->g.x+(fp->g.w-lazyfp.g.w)/2;
        lazyfp.g.y=fp->g.y+(fp->g.h-lazyfp.g.h)/2;
    }*/
    
    if(!frame_init((WFrame*)frame, parent, fp /*&lazyfp*/))
        return FALSE;
    
    region_add_bindmap((WRegion*)frame, mod_autows_autoframe_bindmap);
    
    return TRUE;
}


WAutoFrame *create_autoframe(WWindow *parent, const WFitParams *fp)
{
    CREATEOBJ_IMPL(WAutoFrame, autoframe, (p, parent, fp));
}


static void autoframe_deinit(WAutoFrame *frame)
{
    frame_deinit((WFrame*)frame);
}


/*}}}*/


#if 0    

/*{{{ Fit */


bool autoframe_fitrep(WAutoFrame *frame, WWindow *parent, const WFitParams *fp)
{
    WFitParams lazyfp;
    
    if(fp->mode==REGION_FIT_EXACT){
        lazyfp=*fp;
    }else{
        WRectangle rg=REGION_GEOM(frame);
        lazyfp.mode=REGION_FIT_EXACT;
        
        /*lazyfp.g.w=minof(fp->g.w, rg.w);
        lazyfp.g.h=minof(fp->g.h, rg.h);
        lazyfp.g.x=minof(maxof(fp->g.x, rg.x), fp->g.x+fp->g.w-lazyfp.g.w);
        lazyfp.g.y=minof(maxof(fp->g.y, rg.y), fp->g.y+fp->g.h-lazyfp.g.h);*/
        lazyfp.g.w=minof(fp->g.w, rg.w);
        lazyfp.g.h=minof(fp->g.h, rg.h);
        lazyfp.g.x=fp->g.x+(fp->g.w-lazyfp.g.w)/2;
        lazyfp.g.y=fp->g.y+(fp->g.h-lazyfp.g.h)/2;
    }

    return frame_fitrep(&frame->frame, parent, &lazyfp);
}


/*}}}*/

#endif


/*{{{ Style */


static const char *autoframe_style(WAutoFrame *frame)
{
    return "frame-autoframe";
}


static const char *autoframe_tab_style(WAutoFrame *frame)
{
    return "tab-frame-autoframe";
}


/*}}}*/


/*{{{ Remove managed */


void autoframe_managed_remove(WAutoFrame *frame, WRegion *reg)
{
    mplex_managed_remove((WMPlex*)frame, reg);
    if(FRAME_MCOUNT(frame)==0 && frame->autocreated &&
       !OBJ_IS_BEING_DESTROYED(frame) && 
       region_may_destroy((WRegion*)frame)){
        ioncore_defer_destroy((Obj*)frame);
    }
}


/*}}}*/


/*{{{ Load */


WRegion *autoframe_load(WWindow *par, const WFitParams *fp, ExtlTab tab)
{
    /* TODO: Get use old geometry */
    WAutoFrame *frame=create_autoframe(par, fp);
    if(frame!=NULL)
        frame_do_load((WFrame*)frame, tab);
    return (WRegion*)frame;
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab autoframe_dynfuntab[]={
    {(DynFun*)frame_style, 
     (DynFun*)autoframe_style},
    {(DynFun*)frame_tab_style, 
     (DynFun*)autoframe_tab_style},

    /*{(DynFun*)region_fitrep,
     (DynFun*)autoframe_fitrep},*/

    {region_managed_remove, 
     autoframe_managed_remove},
    
    END_DYNFUNTAB
};
                                       

IMPLCLASS(WAutoFrame, WFrame, autoframe_deinit, autoframe_dynfuntab);

    
/*}}}*/
