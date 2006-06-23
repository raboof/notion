/*
 * ion/ioncore/genws.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
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
#include "names.h"
#include "manage.h"


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
    region_register(&(ws->reg));

    XSelectInput(ioncore_g.dpy, ws->dummywin,
                 FocusChangeMask|KeyPressMask|KeyReleaseMask|
                 ButtonPressMask|ButtonReleaseMask);
    XSaveContext(ioncore_g.dpy, ws->dummywin, ioncore_g.win_context,
                 (XPointer)ws);
    
    ((WRegion*)ws)->flags|=REGION_PLEASE_WARP;

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
    if(warp)
        region_do_warp((WRegion*)ws);
    
    region_set_await_focus((WRegion*)ws);
    xwindow_do_set_focus(ws->dummywin);
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


WRegion *genws_rqclose_propagate(WGenWS *ws, WRegion *maybe_sub)
{
    return (region_rqclose((WRegion*)ws, FALSE) ? (WRegion*)ws : NULL);
}


WPHolder *genws_prepare_manage_transient(WGenWS *ws,
                                         const WClientWin *transient,
                                         const WManageParams *param,
                                         int unused)
{
    /* Transient manager searches should not cross workspaces. */
    return NULL;
}
    

/*}}}*/


/*{{{ Dynfuns */


void genws_manage_stdisp(WGenWS *ws, WRegion *stdisp, 
                         const WMPlexSTDispInfo *info)
{
    CALL_DYN(genws_manage_stdisp, ws, (ws, stdisp, info));
}


void genws_unmanage_stdisp(WGenWS *ws, bool permanent, bool nofocus)
{
    CALL_DYN(genws_unmanage_stdisp, ws, (ws, permanent, nofocus));
}


/*}}}*/


/*{{{ Class implementation */


static DynFunTab genws_dynfuntab[]={
    {(DynFun*)region_xwindow,
     (DynFun*)genws_xwindow},
    /*{(DynFun*)region_rqclose_propagate,
     (DynFun*)genws_rqclose_propagate},*/
    {(DynFun*)region_prepare_manage_transient,
     (DynFun*)genws_prepare_manage_transient},
    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WGenWS, WRegion, genws_deinit, genws_dynfuntab);


/*}}}*/

