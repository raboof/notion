/*
 * ion/ioncore/genws.c
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
#include "global.h"
#include "region.h"
#include "genws.h"
#include "xwindow.h"
#include "focus.h"


/*{{{ Create/destroy */


bool genws_init(WGenWS *ws, WWindow *par, const WFitParams *fp)
{
    ws->dummywin=XCreateWindow(ioncore_g.dpy, par->win,
                                fp->g.x, fp->g.y, 1, 1, 0,
                                CopyFromParent, InputOnly,
                                CopyFromParent, 0, NULL);
    if(ws->dummywin==None)
        return FALSE;

    region_init(&(ws->reg), par, fp);

    XSelectInput(ioncore_g.dpy, ws->dummywin,
                 FocusChangeMask|KeyPressMask|KeyReleaseMask|
                 ButtonPressMask|ButtonReleaseMask);
    XSaveContext(ioncore_g.dpy, ws->dummywin, ioncore_g.win_context,
                 (XPointer)ws);
    
    return TRUE;
}


void genws_deinit(WGenWS *ws)
{
    XDeleteContext(ioncore_g.dpy, ws->dummywin, ioncore_g.win_context);
    XDestroyWindow(ioncore_g.dpy, ws->dummywin);
    ws->dummywin=None;

    region_deinit(&(ws->reg));
}


/*}}}*/


/*{{{ Misc. */


void genws_do_reparent(WGenWS *ws, WWindow *par, const WFitParams *fp)
{
    region_unset_parent((WRegion*)ws);
    XReparentWindow(ioncore_g.dpy, ws->dummywin, par->win, fp->g.x, fp->g.h);
    region_set_parent((WRegion*)ws, par);
}


void genws_fallback_focus(WGenWS *ws, bool warp)
{
    xwindow_do_set_focus(ws->dummywin);
    if(warp)
        region_do_warp((WRegion*)ws);
}


Window genws_xwindow(const WGenWS *ws)
{
    return ws->dummywin;
}


void genws_do_map(WGenWS *ws)
{
    REGION_MARK_MAPPED(ws);
    XMapWindow(ioncore_g.dpy, ws->dummywin);
}


void genws_do_unmap(WGenWS *ws)
{
    REGION_MARK_UNMAPPED(ws);
    XUnmapWindow(ioncore_g.dpy, ws->dummywin);
}


/*}}}*/


/*{{{ Dynfuns */


void genws_manage_stdisp(WGenWS *ws, WRegion *stdisp, int pos)
{
    CALL_DYN(genws_manage_stdisp, ws, (ws, stdisp, pos));
}


void genws_unmanage_stdisp(WGenWS *ws, bool permanent, bool nofocus)
{
    CALL_DYN(genws_unmanage_stdisp, ws, (ws, permanent, nofocus));
}


/*}}}*/


/*{{{ Class implementation */


static DynFunTab genws_dynfuntab[]={
    {(DynFun*)region_xwindow,
     (DynFun*)genws_xwindow},
    
    END_DYNFUNTAB
};


IMPLCLASS(WGenWS, WRegion, genws_deinit, genws_dynfuntab);


/*}}}*/

