/*
 * ion/ioncore/colormap.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "common.h"
#include "global.h"
#include "property.h"
#include "clientwin.h"
#include "colormap.h"
#include "region.h"


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


void clientwin_get_colormaps(WClientWin *cwin)
{
    Window *wins;
    XWindowAttributes attr;
    int i, n;
    
    n=xwindow_get_property(cwin->win, ioncore_g.atom_wm_colormaps,
                   XA_WINDOW, 100L, TRUE, (uchar**)&wins);
    
    if(cwin->n_cmapwins!=0){
        free(cwin->cmapwins);
        free(cwin->cmaps);
    }
    
    if(n>0){
        cwin->cmaps=ALLOC_N(Colormap, n);
        
        if(cwin->cmaps==NULL){
            n=0;
            free(wins);
        }
    }
        
    if(n<=0){
        cwin->cmapwins=NULL;
        cwin->cmaps=NULL;
        cwin->n_cmapwins=0;
        return;
    }
    
    cwin->cmapwins=wins;
    cwin->n_cmapwins=n;
    
    for(i=0; i<n; i++){
        if(wins[i]==cwin->win){
            cwin->cmaps[i]=cwin->cmap;
        }else{
            XSelectInput(ioncore_g.dpy, wins[i], ColormapChangeMask);
            XGetWindowAttributes(ioncore_g.dpy, wins[i], &attr);
            cwin->cmaps[i]=attr.colormap;
        }
    }
}


void clientwin_clear_colormaps(WClientWin *cwin)
{
    int i;
    
    if(cwin->n_cmapwins==0)
        return;
    
    for(i=0; i<cwin->n_cmapwins; i++)
        XSelectInput(ioncore_g.dpy, cwin->cmapwins[i], 0);
    
    free(cwin->cmapwins);
    free(cwin->cmaps);
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
    WClientWin *cwin;

    for(cwin=ioncore_clientwin_list(); 
        cwin!=NULL; 
        cwin=(WClientWin*)((WRegion*)cwin)->ni.ns_next){
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

