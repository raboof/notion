/*
 * ion/ionws/ionframe.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/objp.h>
#include <ioncore/common.h>
#include <ioncore/frame.h>
#include <ioncore/frame-draw.h>
#include <ioncore/framep.h>
#include <ioncore/saveload.h>
#include <ioncore/extl.h>

#include "ionframe.h"
#include "ionws.h"
#include "main.h"


/*{{{ Destroy/create frame */


static bool ionframe_init(WIonFrame *frame, WWindow *par,
                          const WFitParams *fp)
{
    if(!frame_init((WFrame*)frame, par, fp))
        return FALSE;
    
    region_add_bindmap((WRegion*)frame, mod_ionws_ionframe_bindmap);
    
    return TRUE;
}


WIonFrame *create_ionframe(WWindow *par, const WFitParams *fp)
{
    CREATEOBJ_IMPL(WIonFrame, ionframe, (p, par, fp));
}


static void ionframe_deinit(WIonFrame *frame)
{
    frame_deinit((WFrame*)frame);
}


/*}}}*/


/*{{{ Style */


static const char *ionframe_style(WIonFrame *frame)
{
    return "frame-ionframe";
}


static const char *ionframe_tab_style(WIonFrame *frame)
{
    return "tab-frame-ionframe";
}


/*}}}*/


/*{{{ Load */


WRegion *ionframe_load(WWindow *par, const WFitParams *fp, ExtlTab tab)
{
    WIonFrame *frame=create_ionframe(par, fp);
    if(frame!=NULL)
        frame_do_load((WFrame*)frame, tab);
    return (WRegion*)frame;
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab ionframe_dynfuntab[]={
    {(DynFun*)frame_style, (DynFun*)ionframe_style},
    {(DynFun*)frame_tab_style, (DynFun*)ionframe_tab_style},
    
    END_DYNFUNTAB
};
                                       

IMPLCLASS(WIonFrame, WFrame, ionframe_deinit, ionframe_dynfuntab);

    
/*}}}*/
