/*
 * ion/ioncore/colormap.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
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


/*{{{ Installing colormaps */


void install_cmap(WRootWin *rootwin, Colormap cmap)
{
	if(cmap==None)
		cmap=rootwin->default_cmap;
	XInstallColormap(wglobal.dpy, cmap);
}


void install_cwin_cmap(WClientWin *cwin)
{
	int i;
	bool found=FALSE;

	for(i=cwin->n_cmapwins-1; i>=0; i--){
		install_cmap(ROOTWIN_OF(cwin), cwin->cmaps[i]);
		if(cwin->cmapwins[i]==cwin->win)
			found=TRUE;
	}
	
	if(found)
		return;
	
	install_cmap(ROOTWIN_OF(cwin), cwin->cmap);
}


void handle_cwin_cmap(WClientWin *cwin, const XColormapEvent *ev)
{
	int i;
	
	if(ev->window==cwin->win){
		cwin->cmap=ev->colormap;
		if(REGION_IS_ACTIVE(cwin))
			install_cwin_cmap(cwin);
	}else{
		for(i=0; i<cwin->n_cmapwins; i++){
			if(cwin->cmapwins[i]!=ev->window)
				continue;
			cwin->cmaps[i]=ev->colormap;
			if(REGION_IS_ACTIVE(cwin))
				install_cwin_cmap(cwin);
			break;
		}
	}
}


void handle_all_cmaps(const XColormapEvent *ev)
{
	WClientWin *cwin;

	FOR_ALL_CLIENTWINS(cwin){
		handle_cwin_cmap(cwin, ev);
	}
}


/*}}}*/


/*{{{ Management */


void get_colormaps(WClientWin *cwin)
{
	Window *wins;
	XWindowAttributes attr;
	int i, n;
	
	n=get_property(wglobal.dpy, cwin->win, wglobal.atom_wm_colormaps,
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
			XSelectInput(wglobal.dpy, wins[i], ColormapChangeMask);
			XGetWindowAttributes(wglobal.dpy, wins[i], &attr);
			cwin->cmaps[i]=attr.colormap;
		}
	}
}


void clear_colormaps(WClientWin *cwin)
{
	int i;
	
	if(cwin->n_cmapwins==0)
		return;
	
	for(i=0; i<cwin->n_cmapwins; i++)
		XSelectInput(wglobal.dpy, cwin->cmapwins[i], 0);
	
	free(cwin->cmapwins);
	free(cwin->cmaps);
}


/*}}}*/

