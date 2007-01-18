/*
 * ion/ioncore/colormap.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/rb.h>
#include "common.h"
#include "global.h"
#include "property.h"
#include "clientwin.h"
#include "colormap.h"
#include "region.h"
#include "names.h"
#include "xwindow.h"


/*{{{ Installing colormaps */


void rootwin_install_colormap(WRootWin *rootwin, Colormap cmap)
{
    if(cmap==None)
        cmap=rootwin->default_cmap;
    XInstallColormap(ioncore_g.dpy, cmap);
}


void clientwin_install_colormap(WClientWin *cwin)
{
    WRootWin *rw=region_rootwin_of((WRegion*)cwin);
    bool found=FALSE;
    int i;

    for(i=cwin->n_cmapwins-1; i>=0; i--){
        rootwin_install_colormap(rw, cwin->cmaps[i]);
        if(cwin->cmapwins[i]==cwin->win)
            found=TRUE;
    }
    
    if(found)
        return;
    
    rootwin_install_colormap(rw, cwin->cmap);
}


/*}}}*/


/*{{{ Management */


static XContext ctx=None;


void xwindow_unmanaged_selectinput(Window win, long mask)
{
    int *p=NULL;
    
    /* We may be monitoring for colourmap changes */
    if(ctx!=None){
        if(XFindContext(ioncore_g.dpy, win, ctx, (XPointer*)&p)==0){
            if(*p>0)
                mask|=ColormapChangeMask;
        }
    }
    
    XSelectInput(ioncore_g.dpy, win, mask);
}

                
        
static void xwindow_selcmap(Window win)
{
    int *p=NULL;
    XWindowAttributes attr;
    
    if(ctx==None)
        ctx=XUniqueContext();
    
    if(XFindContext(ioncore_g.dpy, win, ctx, (XPointer*)&p)==0){
        (*p)++;
    }else{
        p=ALLOC(int);
        if(p==NULL)
            return;
        
        *p=1;
        if(XSaveContext(ioncore_g.dpy, win, ctx, (XPointer)p)!=0){
            warn(TR("Unable to store colourmap watch info."));
            return;
        }

        if(XWINDOW_REGION_OF(win)==NULL){
            XGetWindowAttributes(ioncore_g.dpy, win, &attr);
            XSelectInput(ioncore_g.dpy, win, 
                         attr.your_event_mask|ColormapChangeMask);
        }
    }
}


static void xwindow_unselcmap(Window win)
{
    int *p=NULL;
    XWindowAttributes attr;
    
    if(ctx==None)
        return;

    if(XFindContext(ioncore_g.dpy, win, ctx, (XPointer*)&p)==0){
        (*p)--;
        if(*p==0){
            XDeleteContext(ioncore_g.dpy, win, ctx);
            free(p);
            if(XWINDOW_REGION_OF(win)==NULL){
                XGetWindowAttributes(ioncore_g.dpy, win, &attr);
                XSelectInput(ioncore_g.dpy, win, 
                             attr.your_event_mask&~ColormapChangeMask);
            }
        }
    }
}


void clientwin_get_colormaps(WClientWin *cwin)
{
    Window *wins;
    XWindowAttributes attr;
    int i, n;

    clientwin_clear_colormaps(cwin);
    
    n=xwindow_get_property(cwin->win, ioncore_g.atom_wm_colormaps,
                           XA_WINDOW, 100L, TRUE, (uchar**)&wins);
    
    if(n<=0)
        return;
    
    cwin->cmaps=ALLOC_N(Colormap, n);
    
    if(cwin->cmaps==NULL)
        return;
    
    cwin->cmapwins=wins;
    cwin->n_cmapwins=n;
    
    for(i=0; i<n; i++){
        if(wins[i]==cwin->win){
            cwin->cmaps[i]=cwin->cmap;
        }else{
            xwindow_selcmap(wins[i]);
            XGetWindowAttributes(ioncore_g.dpy, wins[i], &attr);
            cwin->cmaps[i]=attr.colormap;
        }
    }
}


void clientwin_clear_colormaps(WClientWin *cwin)
{
    int i;
    XWindowAttributes attr;

    if(cwin->n_cmapwins==0)
        return;

    for(i=0; i<cwin->n_cmapwins; i++){
        if(cwin->cmapwins[i]!=cwin->win)
            xwindow_unselcmap(cwin->cmapwins[i]);
    }
    
    free(cwin->cmapwins);
    free(cwin->cmaps);
    cwin->n_cmapwins=0;
    cwin->cmapwins=NULL;
    cwin->cmaps=NULL;
}


/*}}}*/


/*{{{ Event handling */


static void handle_cwin_cmap(WClientWin *cwin, const XColormapEvent *ev)
{
    int i;
    
    if(ev->window==cwin->win){
        cwin->cmap=ev->colormap;
        if(REGION_IS_ACTIVE(cwin))
            clientwin_install_colormap(cwin);
    }else{
        for(i=0; i<cwin->n_cmapwins; i++){
            if(cwin->cmapwins[i]!=ev->window)
                continue;
            cwin->cmaps[i]=ev->colormap;
            if(REGION_IS_ACTIVE(cwin))
                clientwin_install_colormap(cwin);
            break;
        }
    }
}


static void handle_all_cmaps(const XColormapEvent *ev)
{
    Rb_node node;
    
    if(!ioncore_clientwin_ns.initialised)
        return;
    
    rb_traverse(node, ioncore_clientwin_ns.rb){
        WClientWin *cwin=(WClientWin*)rb_val(node);
        if(cwin!=NULL)
            handle_cwin_cmap(cwin, ev);
    }
}



void ioncore_handle_colormap_notify(const XColormapEvent *ev)
{
    WClientWin *cwin;
    
    if(!ev->new)
        return;

    cwin=XWINDOW_REGION_OF_T(ev->window, WClientWin);

    if(cwin!=NULL){
        handle_cwin_cmap(cwin, ev);
        /*set_cmap(cwin, ev->colormap);*/
    }else{
        handle_all_cmaps(ev);
    }
}


/*}}}*/

