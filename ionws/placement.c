/*
 * ion/placement.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <wmcore/common.h>
#include <wmcore/global.h>
#include <wmcore/screen.h>
#include <wmcore/property.h>
#include <wmcore/clientwin.h>
#include <wmcore/targetid.h>
#include <wmcore/attach.h>
#include "placement.h"
#include "frame.h"
#include "winprops.h"
#include "splitframe.h"

#undef FULLSCREEN_CLIENTS_POSSIBLE

static void set_winprops(WClientWin *cwin, const WWinProp *winprop)
{
	cwin->flags|=winprop->flags;
	
	if(cwin->flags&CWIN_PROP_MAXSIZE){
		cwin->size_hints.max_width=winprop->max_w;
		cwin->size_hints.max_height=winprop->max_h;
		cwin->size_hints.flags|=PMaxSize;
	}

	if(cwin->flags&CWIN_PROP_ASPECT){
		cwin->size_hints.min_aspect.x=winprop->aspect_w;
		cwin->size_hints.max_aspect.x=winprop->aspect_w;
		cwin->size_hints.min_aspect.y=winprop->aspect_h;
		cwin->size_hints.max_aspect.y=winprop->aspect_h;
		cwin->size_hints.flags|=PAspect;
	}
}


WWinProp *setup_get_winprops(WClientWin *cwin)
{
	WWinProp *props;
	
	props=find_winprop_win(cwin->win);
	
	if(props!=NULL)
		set_winprops(cwin, props);
	
	return props;
}


static WWorkspace *find_suitable_workspace(WScreen *scr)
{
	WRegion *r=REGION_ACTIVE_SUB(scr);
	
	if(r!=NULL && WTHING_IS(r, WWorkspace))
		return (WWorkspace*)r;

#ifdef FULLSCREEN_CLIENTS_POSSIBLE
	return NULL;
#else
	return FIRST_THING(scr, WWorkspace);
#endif
}


static WRegion *find_suitable_frame(WWorkspace *ws)
{
	WRegion *r=REGION_ACTIVE_SUB(ws);
	
	if(r!=NULL && region_supports_attach(r)) /* WTHING_IS(r, WFrame)) */
		return r;
	
	return (WRegion*)FIRST_THING(ws, WFrame);
}


bool add_clientwin(WClientWin *cwin, const XWindowAttributes *attr,
				   int init_state, bool dock)
{
	int target_id=0;
	Window win=cwin->win;
	WClientWin *tfor;
	WRegion *target=NULL;
	WWorkspace *ws=NULL;
	WWinProp *props=NULL;

	/* Get and set winprops */
	props=setup_get_winprops(cwin);
	
	/* Is it a transient to some other window? */
	if(XGetTransientForHint(wglobal.dpy, win, &(cwin->transient_for))){
		tfor=find_clientwin(cwin->transient_for);
		if(tfor!=NULL){
			if(clientwin_attach_sub(tfor, &cwin->region, 0))
				return TRUE;
		}
		cwin->transient_for=None;
	}
	
	get_integer_property(win, wglobal.atom_frame_id, &target_id);
	
	if(target_id!=0)
		target=find_target_by_id(target_id);
	
	if(target!=NULL){
		if(!region_supports_attach(target)){
			warn("Target region of window %#x does not support primitive "
				 "attach method", cwin->win);
			target=NULL;
		}else if(SCREEN_OF(target)!=SCREEN_OF(cwin)){
			warn("The target id property of window %#x is set to "
				 "a frame on different screen", cwin->win);
			target=NULL;
		}
	}
		
	if(target==NULL){
		bool geomset;
		ws=find_suitable_workspace(SCREEN_OF(cwin));
		
#ifdef CF_PLACEMENT_GEOM
		geomset=(cwin->size_hints.win_gravity!=ForgetGravity &&
				 (attr->x>CF_STUBBORN_TRESH &&
				  attr->y>CF_STUBBORN_TRESH));
		if(ws!=NULL && geomset && (!props || !props->stubborn))
			target=(WRegion*)find_frame_at(ws, attr->x, attr->y);
	}
	if(target==NULL){
#endif
		if(ws!=NULL)
			target=find_suitable_frame(ws);

		if(target==NULL){
#ifdef FULLSCREEN_CLIENTS_POSSIBLE
			target=SCREEN_OF(cwin);
#else
			warn("No client-supplied frame for window %d and no"
				 "current frame. TODO: fullscreen", win);
			return FALSE;
#endif
		}
	}
	
	return finish_add_clientwin(target, cwin, init_state, props);
}


bool finish_add_clientwin(WRegion *reg, WClientWin *cwin,
						  bool init_state, const WWinProp *props)
{
#ifdef CF_SWITCH_NEW_CLIENTS
	bool switchto=TRUE;
#else
	bool switchto=FALSE;
#endif
	
	assert(reg!=NULL);
	
	/* frame!=NULL => should be a new client */
	if(wglobal.opmode==OPMODE_INIT)
		switchto=(init_state!=IconicState);
	else if(props!=NULL && props->switchto>=0)
		switchto=props->switchto;
		
	return region_attach_sub(reg, (WRegion*)cwin,
							 switchto ? REGION_ATTACH_SWITCHTO : 0);
}

