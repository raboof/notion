/*
 * wmcore/wsreg.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include "global.h"
#include "common.h"
#include "region.h"
#include "attach.h"
#include "property.h"
#include "targetid.h"
#include "wsreg.h"
#include "mwmhints.h"
#include "objp.h"


/* This file contains the add_clientwin_default handler for managing
 * clientwins. It should be called whenever no other handler knows
 * a special way to handle the window. This will then check if there
 * are any properties set indicating where to place the window and if
 * not, pass on the trouble to a suitable workspace (the current).
 */


/*{{{ Static helper functions */


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


static WWinProp *setup_get_winprops(WClientWin *cwin)
{
	WWinProp *props;
	
	props=find_winprop_win(cwin->win);
	
	if(props!=NULL)
		set_winprops(cwin, props);
	
	return props;
}


static bool add_transient(WClientWin *tfor, WClientWin *cwin,
						  const XWindowAttributes *attr,
						  int init_state, WWinProp *props)
{
	
	WRegion *p=(WRegion*)tfor;
	
	while(p!=NULL){
		if(HAS_DYN(p, region_ws_add_transient))
			return region_ws_add_transient(p, tfor, cwin, attr,
										   init_state, props);
		p=FIND_PARENT1(p, WRegion);
	}
	
	/* No parent workspace found that would want to handle transients.
	 * tfor is probably a full screen client window so let's just attach
	 * the transient to it like Ion normally does.
	 */
	return clientwin_attach_sub(tfor, (WRegion*)cwin, 0);
}


static WRegion *find_suitable_workspace(WViewport *vp)
{
	WRegion *r=REGION_ACTIVE_SUB(vp);
	
	if(r!=NULL && HAS_DYN(r, region_ws_add_clientwin))
		return r;

	FOR_ALL_TYPED(vp, r, WRegion){
		if(HAS_DYN(r, region_ws_add_clientwin))
			return r;
	}
	
	return NULL;
}


static void find_prop_target(WClientWin *cwin, WWinProp *props,
							 WRegion **target, WRegion **ws)
{
	WRegion *r;
	
	if(props==NULL || props->target_name==NULL)
		return;
	
	/* Potential problem: numbering */
	r=lookup_region(props->target_name);
	
	if(r!=NULL && SCREEN_OF(r)==SCREEN_OF(cwin)){
		if(ws!=NULL && HAS_DYN(r, region_ws_add_clientwin))
			*ws=r;
		else if(target!=NULL)
			*target=r;
	}
}


static WViewport *find_suitable_viewport(WClientWin *cwin, int x, int y)
{
	WScreen *scr=SCREEN_OF(cwin);
	WViewport *vp;
	
	if(x>CF_STUBBORN_TRESH && y>CF_STUBBORN_TRESH &&
	   cwin->size_hints.win_gravity!=ForgetGravity){
		FOR_ALL_TYPED(scr, vp, WViewport){
			WRectangle geom=REGION_GEOM(vp);
			if(x>=geom.x && x<geom.x+geom.w &&
			   y>=geom.y && y<geom.y+geom.h)
				return vp;
		}
	}
	
	if(REGION_ACTIVE_SUB(scr)!=NULL &&
	   WTHING_IS(REGION_ACTIVE_SUB(scr), WViewport)){
		return (WViewport*)REGION_ACTIVE_SUB(scr);
	}
	
	return (WViewport*)scr->default_viewport;
}


/*}}}*/


/*{{{ Add */


bool add_clientwin_default(WClientWin *cwin, const XWindowAttributes *attr,
						   int init_state, bool dock)
{
	int target_id=0;
	Window win=cwin->win;
	WClientWin *tfor;
	WRegion *target=NULL, *ws=NULL;
	WWinProp *props;
	WViewport *vp;
	
	vp=find_suitable_viewport(cwin, attr->x, attr->y);
	
	/* Get and set winprops */
	props=setup_get_winprops(cwin);

	/* check full screen mode */
	do{
		WMwmHints *mwm;
		bool switchto=FALSE;
		mwm=get_mwm_hints(cwin->win);
		if(mwm==NULL)
			break;
		if(REGION_GEOM(SCREEN_OF(cwin)).w==attr->width &&
		   REGION_GEOM(SCREEN_OF(cwin)).h==attr->height &&
		   mwm->flags&MWM_HINTS_DECORATIONS && mwm->decorations==0){
			if(wglobal.opmode!=OPMODE_INIT){
#ifdef CF_SWITCH_NEW_CLIENTS
				switchto=TRUE;
#endif
				if(props!=NULL)
					switchto=(props->manage_flags&REGION_ATTACH_SWITCHTO);
			}
			if(clientwin_fullscreen_vp(cwin, vp, switchto))
				return TRUE;
			warn("Failed to enter full screen mode.");
		}
	}while(0);
		
	if(props==NULL || props->transient_mode!=TRANSIENT_MODE_CURRENT){
		if(props==NULL || props->transient_mode==TRANSIENT_MODE_NORMAL){
			/* Is it a transient to some other window? */
			if(XGetTransientForHint(wglobal.dpy, win, &(cwin->transient_for))){
				tfor=find_clientwin(cwin->transient_for);
				if(tfor!=NULL && add_transient(tfor, cwin, attr,
											   init_state, props))
					return TRUE;
				cwin->transient_for=None;
			}
		}
		
		get_integer_property(win, wglobal.atom_frame_id, &target_id);
		
		if(target_id!=0)
			target=find_target_by_id(target_id);
		
		find_prop_target(cwin, props, target==NULL ? &target : NULL, &ws);
		
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
		
		/* Found a specific target frame or such? */
		if(target!=NULL)
			return finish_add_clientwin(target, cwin, init_state, props);
	}
	
	/* No, need to find a workspace and let it handle this. */
	
	if(ws==NULL)
		ws=find_suitable_workspace(vp);

	if(ws==NULL)
		return clientwin_fullscreen_vp(cwin, vp, FALSE);
	
	return region_ws_add_clientwin(ws, cwin, attr, init_state, props);
}



bool region_ws_add_clientwin(WRegion *reg, WClientWin *cwin,
							 const XWindowAttributes *attr,
							 int init_state, WWinProp *props)
{
	bool ret=TRUE;
	CALL_DYN_RET(ret, bool, region_ws_add_clientwin,
				 reg, (reg, cwin, attr, init_state, props));
	return ret;
	
}


bool region_ws_add_transient(WRegion *reg, WClientWin *tfor,
							 WClientWin *cwin,
							 const XWindowAttributes *attr,
							 int init_state, WWinProp *props)
{
	bool ret=TRUE;
	CALL_DYN_RET(ret, bool, region_ws_add_transient,
				 reg, (reg, tfor, cwin, attr, init_state, props));
	return ret;
}


bool finish_add_clientwin(WRegion *reg, WClientWin *cwin,
						  bool init_state, const WWinProp *props)
{
	int flags=REGION_ATTACH_SWITCHTO;
	
	assert(reg!=NULL);
	
	if(init_state==IconicState)
		flags=0;
	if(wglobal.opmode!=OPMODE_INIT && props!=NULL)
		flags=props->manage_flags;
		
	return region_attach_sub(reg, (WRegion*)cwin, flags);
}


/*}}}*/

