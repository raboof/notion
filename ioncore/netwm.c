/*
 * ion/ioncore/netwm.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <X11/Xatom.h>
#include <X11/Xmd.h>

#include <libtu/util.h>
#include "common.h"
#include "global.h"
#include "fullscreen.h"
#include "clientwin.h"
#include "netwm.h"
#include "property.h"
#include "focus.h"
#include "region-iter.h"


static Atom atom_net_wm_state_fullscreen=0;
static Atom atom_net_supported=0;
static Atom atom_net_supporting_wm_check=0;
static Atom atom_net_virtual_roots=0;

#define N_NETWM 5


void netwm_init()
{
	ioncore_g.atom_net_wm_name=XInternAtom(ioncore_g.dpy, "_NET_WM_NAME", False);
	ioncore_g.atom_net_wm_state=XInternAtom(ioncore_g.dpy, "_NET_WM_STATE", False);
	atom_net_supported=XInternAtom(ioncore_g.dpy, "_NET_SUPPORTED", False);
	atom_net_supporting_wm_check=XInternAtom(ioncore_g.dpy, "_NET_SUPPORTING_WM_CHECK", False);
	atom_net_wm_state_fullscreen=XInternAtom(ioncore_g.dpy, "_NET_WM_STATE_FULLSCREEN", False);
	atom_net_virtual_roots=XInternAtom(ioncore_g.dpy, "_NET_VIRTUAL_ROOTS", False);
}


void netwm_init_rootwin(WRootWin *rw)
{
	Atom atoms[N_NETWM];

	atoms[0]=ioncore_g.atom_net_wm_name;
	atoms[1]=ioncore_g.atom_net_wm_state;
	atoms[2]=atom_net_wm_state_fullscreen;
	atoms[3]=atom_net_supporting_wm_check;
	atoms[4]=atom_net_virtual_roots;
	
	FOR_ALL_ROOTWINS(rw){
		XChangeProperty(ioncore_g.dpy, WROOTWIN_ROOT(rw),
						atom_net_supporting_wm_check, XA_WINDOW,
						32, PropModeReplace, (uchar*)&(rw->dummy_win), 1);
		XChangeProperty(ioncore_g.dpy, WROOTWIN_ROOT(rw),
						atom_net_supported, XA_ATOM,
						32, PropModeReplace, (uchar*)atoms, N_NETWM);
		/* Something else should probably be used as WM name here. */
		xwindow_set_text_property(rw->dummy_win, ioncore_g.atom_net_wm_name,
						  prog_execname());
	}
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
	
	n=xwindow_get_property(cwin->win, ioncore_g.atom_net_wm_state, XA_ATOM,
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

	XChangeProperty(ioncore_g.dpy, cwin->win, ioncore_g.atom_net_wm_state, 
					XA_ATOM, 32, PropModeReplace, (uchar*)data, n);
}


