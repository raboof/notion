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

#include "autoframe.h"
#include "main.h"


/*{{{ Destroy/create frame */


static bool autoframe_init(WAutoFrame *frame, WWindow *parent, 
                           const WFitParams *fp)
{
    WFitParams lazyfp;
    
    frame->last_fp=*fp;
    
    /*if(fp->mode==REGION_FIT_EXACT){*/
        lazyfp=*fp;
    /*}else{
        lazyfp.mode=REGION_FIT_EXACT;
        lazyfp.g.w=minof(fp->g.w, CF_SCRATCHPAD_DEFAULT_W);
        lazyfp.g.h=minof(fp->g.h, CF_SCRATCHPAD_DEFAULT_H);
        lazyfp.g.x=fp->g.x+(fp->g.w-lazyfp.g.w)/2;
        lazyfp.g.y=fp->g.y+(fp->g.h-lazyfp.g.h)/2;
    }*/
    
    if(!frame_init((WFrame*)frame, parent, &lazyfp))
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


/*{{{ Fit */


bool autoframe_fitrep(WAutoFrame *frame, WWindow *parent, const WFitParams *fp)
{
    WFitParams lazyfp;
    
    if(fp->mode==REGION_FIT_EXACT){
        lazyfp=*fp;
    }else{
        lazyfp.mode=REGION_FIT_EXACT;
        
        if(parent!=NULL){
            lazyfp.g.w=minof(fp->g.w, REGION_GEOM(frame).w);
            lazyfp.g.h=minof(fp->g.h, REGION_GEOM(frame).h);
            lazyfp.g.x=fp->g.x+(fp->g.w-lazyfp.g.w)/2;
            lazyfp.g.y=fp->g.y+(fp->g.h-lazyfp.g.h)/2;
        }else{
            lazyfp.g=REGION_GEOM(frame);
            rectangle_constrain(&(lazyfp.g), &(fp->g));
        }
    }
    
    return frame_fitrep(&frame->frame, parent, &lazyfp);
}


/*}}}*/


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

    {(DynFun*)region_fitrep,
     (DynFun*)autoframe_fitrep},
    
    END_DYNFUNTAB
};
                                       

IMPLCLASS(WAutoFrame, WFrame, autoframe_deinit, autoframe_dynfuntab);

    
/*}}}*/
