/*
 * ion/ioncore/xwindow.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <libtu/minmax.h>
#include "common.h"
#include "global.h"
#include "xwindow.h"
#include "cursor.h"
#include "sizehint.h"


/*{{{ X window->region mapping */


WRegion *xwindow_region_of(Window win)
{
    WRegion *reg;
    
    if(XFindContext(ioncore_g.dpy, win, ioncore_g.win_context,
                    (XPointer*)&reg)!=0)
        return NULL;
    
    return reg;
}


WRegion *xwindow_region_of_t(Window win, const ClassDescr *descr)
{
    WRegion *reg=xwindow_region_of(win);
    
    if(reg==NULL)
        return NULL;
    
    if(!obj_is((Obj*)reg, descr))
        return NULL;
    
    return reg;
}


/*}}}*/


/*{{{ Create */


Window create_xwindow(WRootWin *rw, Window par, const WRectangle *geom)
{
    int w=maxof(1, geom->w);
    int h=maxof(1, geom->h);
    
    return XCreateSimpleWindow(ioncore_g.dpy, par, geom->x, geom->y, w, h,
                               0, 0, BlackPixel(ioncore_g.dpy, rw->xscr));
}


/*}}}*/


/*{{{ Restack */


void xwindow_restack(Window win, Window other, int stack_mode)
{
    XWindowChanges wc;
    int wcmask;
    
    wcmask=CWStackMode;
    wc.stack_mode=stack_mode;
    if((wc.sibling=other)!=None)
        wcmask|=CWSibling;

    XConfigureWindow(ioncore_g.dpy, win, wcmask, &wc);
}


/*}}}*/


/*{{{ Focus */


void xwindow_do_set_focus(Window win)
{
    XSetInputFocus(ioncore_g.dpy, win, RevertToParent, CurrentTime);
}


/*}}}*/


/*{{{ Cursors */

void xwindow_set_cursor(Window win, int cursor)
{
    XDefineCursor(ioncore_g.dpy, win, ioncore_xcursor(cursor));
}


/*}}}*/


/*{{{ Size hints */


void xwindow_get_sizehints(Window win, XSizeHints *hints)
{
    int minh, minw;
    long supplied=0;
    
    memset(hints, 0, sizeof(*hints));
    XGetWMNormalHints(ioncore_g.dpy, win, hints, &supplied);
    
    xsizehints_sanity_adjust(hints);
}


/*}}}*/

