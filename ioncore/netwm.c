/*
 * ion/ioncore/netwm.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <X11/Xatom.h>

#include "common.h"
#include "mwmhints.h"
#include "global.h"
#include "fullscreen.h"
#include "clientwin.h"
#include "netwm.h"
#include "property.h"


static Atom atom_net_wm_state_fullscreen=0;


void netwm_init()
{
	wglobal.atom_net_wm_name=XInternAtom(wglobal.dpy, "_NET_WM_NAME", False);
	wglobal.atom_net_wm_state=XInternAtom(wglobal.dpy, "_NET_WM_STATE", False);
	atom_net_wm_state_fullscreen=XInternAtom(wglobal.dpy, "_NET_WM_STATE_FULLSCREEN", False);
	
	/* Should set _NET_SUPPROTED. */
}

	
void netwm_state_change_rq(WClientWin *cwin, const XClientMessageEvent *ev)
{
	if((ev->data.l[1]==0 ||
		ev->data.l[1]!=(long)atom_net_wm_state_fullscreen) &&
	   (ev->data.l[2]==0 ||
		ev->data.l[2]!=(long)atom_net_wm_state_fullscreen)){
		return;
	}
	
	/* Ok, full screen add/remove/toggle */
	if(!CLIENTWIN_IS_FULLSCREEN(cwin)){
		if(ev->data.l[0]==_NET_WM_STATE_ADD || 
		   ev->data.l[0]==_NET_WM_STATE_TOGGLE){
			bool sw=region_may_control_focus((WRegion*)cwin);
			clientwin_enter_fullscreen(cwin, sw);
		}
	}else{
		if(ev->data.l[0]==_NET_WM_STATE_REMOVE || 
		   ev->data.l[0]==_NET_WM_STATE_TOGGLE){
			bool sw=region_may_control_focus((WRegion*)cwin);
			clientwin_leave_fullscreen(cwin, sw);
		}
	}
}


int netwm_check_initial_fullscreen(WClientWin *cwin, bool sw)
{

	int i, n;
	int ret=0;
	long *data;
	
	n=get_property(wglobal.dpy, cwin->win, wglobal.atom_net_wm_state, XA_ATOM,
				   1, TRUE, (uchar**)&data);
	
	if(n<0)
		return -1;
	
	for(i=0; i<n; i++){
		if(data[i]==(long)atom_net_wm_state_fullscreen){
			ret=clientwin_enter_fullscreen(cwin, sw);
			break;
		}
	}
	
	XFree((void*)data);

	return ret;
}


void netwm_update_state(WClientWin *cwin)
{
	CARD32 data[1];
	int n=0;
	
	if(CLIENTWIN_IS_FULLSCREEN(cwin))
		data[n++]=atom_net_wm_state_fullscreen;

	XChangeProperty(wglobal.dpy, cwin->win, wglobal.atom_net_wm_state, 
					XA_ATOM, 32, PropModeReplace, (uchar*)data, n);
}


