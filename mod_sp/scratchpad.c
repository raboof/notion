/*
 * ion/mod_sp/scratchpad.c
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
#include <ioncore/activity.h>
#include <ioncore/defer.h>
#include <ioncore/frame.h>
#include <ioncore/frame-draw.h>
#include <ioncore/framep.h>
#include <ioncore/saveload.h>
#include <ioncore/extl.h>
#include <ioncore/regbind.h>
#include <ioncore/region-iter.h>
#include <ioncore/infowin.h>
#include <ioncore/names.h>

#include "scratchpad.h"
#include "main.h"


/*{{{ Destroy/create frame */


static bool scratchpad_init(WScratchpad *sp, WWindow *parent, 
                            const WFitParams *fp)
{
    WFitParams lazyfp;
    
    sp->last_fp=*fp;
    watch_init(&(sp->notifywin_watch));
    
    if(fp->mode==REGION_FIT_EXACT){
        lazyfp=*fp;
    }else{
        lazyfp.mode=REGION_FIT_EXACT;
        lazyfp.g.w=minof(fp->g.w, CF_SCRATCHPAD_DEFAULT_W);
        lazyfp.g.h=minof(fp->g.h, CF_SCRATCHPAD_DEFAULT_H);
        lazyfp.g.x=fp->g.x+(fp->g.w-lazyfp.g.w)/2;
        lazyfp.g.y=fp->g.y+(fp->g.h-lazyfp.g.h)/2;
    }
    
    if(!frame_init((WFrame*)sp, parent, &lazyfp))
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


/*{{{ Notifications */


void scratchpad_notify(WScratchpad *sp)
{
    WInfoWin *iw;
    WWindow *parent;
    WFitParams fp;
    GrBrush *brush;
    GrBorderWidths bdw;
    GrFontExtents fnte;

    if(sp->notifywin_watch.obj!=NULL)
        return;
    
    parent=REGION_PARENT_CHK(sp, WWindow);
    
    if(!OBJ_IS(parent, WMPlex))
        return;
    
    fp.mode=REGION_FIT_EXACT;
    fp.g.x=0;
    fp.g.y=0;
    fp.g.w=1;
    fp.g.h=1;
    
    iw=create_infowin(parent, &fp, "spnotify");
    
    if(iw==NULL)
        return;
    
    snprintf(INFOWIN_BUFFER(iw), INFOWIN_BUFFER_LEN, "act: %s",
             region_name((WRegion*)sp));

    grbrush_get_border_widths(INFOWIN_BRUSH(iw), &bdw);
    grbrush_get_font_extents(INFOWIN_BRUSH(iw), &fnte);
    
    fp.g.w=bdw.left+bdw.right;
    fp.g.w+=grbrush_get_text_width(INFOWIN_BRUSH(iw), INFOWIN_BUFFER(iw),
                                   strlen(INFOWIN_BUFFER(iw)));
    fp.g.h=fnte.max_height+bdw.top+bdw.bottom;;
    fp.g.x=sp->last_fp.g.x;
    fp.g.y=sp->last_fp.g.y;
    
    region_fitrep((WRegion*)iw, NULL, &fp);
    region_map((WRegion*)iw);
    
    watch_setup(&(sp->notifywin_watch), (Obj*)iw, NULL);
}


void scratchpad_unnotify(WScratchpad *sp)
{
    Obj *iw=sp->notifywin_watch.obj;
    if(iw!=NULL){
        ioncore_defer_destroy(iw);
        watch_reset(&(sp->notifywin_watch));
    }
}


static bool scratchpad_managed_activity(WScratchpad *sp)
{
    WRegion *reg;
    
    FOR_ALL_MANAGED_ON_LIST(sp->frame.mplex.l1_list, reg){
        if(region_activity(reg))
            return TRUE;
    }
    
    FOR_ALL_MANAGED_ON_LIST(sp->frame.mplex.l2_list, reg){
        if(region_activity(reg))
            return TRUE;
    }
    
    return FALSE;
}


static void scratchpad_managed_notify(WScratchpad *sp, WRegion *sub)
{
    frame_managed_notify(&(sp->frame), sub);
    if(!REGION_IS_MAPPED(sp) && scratchpad_managed_activity(sp))
        scratchpad_notify(sp);
}


static void scratchpad_map(WScratchpad *sp)
{
    mplex_map((WMPlex*)sp);
    scratchpad_unnotify(sp);
}


/*}}}*/


/*{{{ Fit */


bool scratchpad_fitrep(WScratchpad *sp, WWindow *parent, const WFitParams *fp)
{
    WFitParams lazyfp;
    
    if(parent!=NULL)
        scratchpad_unnotify(sp);
    
    if(fp->mode==REGION_FIT_EXACT){
        lazyfp=*fp;
    }else{
        lazyfp.mode=REGION_FIT_EXACT;
        lazyfp.g.w=minof(fp->g.w, REGION_GEOM(sp).w);
        lazyfp.g.h=minof(fp->g.h, REGION_GEOM(sp).h);
        
        if(parent!=NULL){
            lazyfp.g.x=fp->g.x+(fp->g.w-lazyfp.g.w)/2;
            lazyfp.g.y=fp->g.y+(fp->g.h-lazyfp.g.h)/2;
        }else{
            lazyfp.g.x=minof(maxof(fp->g.x, REGION_GEOM(sp).x),
                              fp->g.x+fp->g.w-lazyfp.g.w);
            lazyfp.g.y=minof(maxof(fp->g.y, REGION_GEOM(sp).y),
                              fp->g.y+fp->g.h-lazyfp.g.h);
        }
    }
    
    return frame_fitrep(&sp->frame, parent, &lazyfp);
}


/*}}}*/


/*{{{ Style */


static const char *scratchpad_style(WScratchpad *frame)
{
    return "frame-scratchpad";
}


static const char *scratchpad_tab_style(WScratchpad *frame)
{
    return "tab-frame-scratchpad";
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


/*{{{ Dynamic function table and class implementation */


static DynFunTab scratchpad_dynfuntab[]={
    {(DynFun*)frame_style, 
     (DynFun*)scratchpad_style},
    {(DynFun*)frame_tab_style, 
     (DynFun*)scratchpad_tab_style},

    {(DynFun*)region_fitrep,
     (DynFun*)scratchpad_fitrep},
    
    {(DynFun*)region_managed_notify,
     (DynFun*)scratchpad_managed_notify},
    
    {region_map,
     scratchpad_map},
    
    END_DYNFUNTAB
};
                                       

IMPLCLASS(WScratchpad, WFrame, scratchpad_deinit, scratchpad_dynfuntab);

    
/*}}}*/
