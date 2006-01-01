/*
 * ion/mod_sp/scratchpad.c
 *
 * Copyright (c) Tuomo Valkonen 2004-2006. 
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
#include <libextl/extl.h>
#include <ioncore/regbind.h>

#include "scratchpad.h"
#include "main.h"


/*{{{ Destroy/create frame */


static bool scratchpad_init(WScratchpad *sp, WWindow *parent, 
                            const WFitParams *fp)
{
    WFitParams lazyfp;
    
    sp->last_fp=*fp;
    
    if(!(fp->mode&REGION_FIT_BOUNDS)){
        lazyfp=*fp;
    }else{
        lazyfp.mode=REGION_FIT_EXACT;
        lazyfp.g.w=minof(fp->g.w, CF_SCRATCHPAD_DEFAULT_W);
        lazyfp.g.h=minof(fp->g.h, CF_SCRATCHPAD_DEFAULT_H);
        lazyfp.g.x=fp->g.x+(fp->g.w-lazyfp.g.w)/2;
        lazyfp.g.y=fp->g.y+(fp->g.h-lazyfp.g.h)/2;
    }
    
    if(!frame_init((WFrame*)sp, parent, &lazyfp, "frame-scratchpad"))
        return FALSE;
    
    region_add_bindmap((WRegion*)sp, mod_sp_scratchpad_bindmap);
    
    return TRUE;
}


WScratchpad *create_scratchpad(WWindow *parent, const WFitParams *fp)
{
    CREATEOBJ_IMPL(WScratchpad, scratchpad, (p, parent, fp));
}


static void scratchpad_deinit(WScratchpad *sp)
{
    frame_deinit((WFrame*)sp);
}


/*}}}*/


/*{{{ Fit */


bool scratchpad_fitrep(WScratchpad *sp, WWindow *parent, const WFitParams *fp)
{
    WFitParams lazyfp;
    
    if(!(fp->mode&REGION_FIT_BOUNDS)){
        lazyfp=*fp;
    }else{
        lazyfp.mode=REGION_FIT_EXACT;
        
        if(parent!=NULL){
            lazyfp.g.w=minof(fp->g.w, REGION_GEOM(sp).w);
            lazyfp.g.h=minof(fp->g.h, REGION_GEOM(sp).h);
            lazyfp.g.x=fp->g.x+(fp->g.w-lazyfp.g.w)/2;
            lazyfp.g.y=fp->g.y+(fp->g.h-lazyfp.g.h)/2;
        }else{
            lazyfp.g=REGION_GEOM(sp);
            rectangle_constrain(&(lazyfp.g), &(fp->g));
        }
    }
    
    return frame_fitrep(&sp->frame, parent, &lazyfp);
}


/*}}}*/


/*{{{ Load */


WRegion *scratchpad_load(WWindow *par, const WFitParams *fp, ExtlTab tab)
{
    WScratchpad *frame=create_scratchpad(par, fp);
    if(frame!=NULL)
        frame_do_load((WFrame*)frame, tab);
    return (WRegion*)frame;
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab scratchpad_dynfuntab[]={
    {(DynFun*)region_fitrep,
     (DynFun*)scratchpad_fitrep},
    
    END_DYNFUNTAB
};
                                       

EXTL_EXPORT
IMPLCLASS(WScratchpad, WFrame, scratchpad_deinit, scratchpad_dynfuntab);

    
/*}}}*/
