/*
 * ion/ioncore/wsreg.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
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
#include "names.h"
#include "winprops.h"
#include "extl.h"


/* This file contains the add_clientwin_default handler for managing
 * clientwins. It should be called whenever no other handler knows
 * a special way to handle the window. This will then check if there
 * are any properties set indicating where to place the window and if
 * not, pass on the trouble to a suitable workspace (the current).
 */


/*{{{ Static helper functions */


static bool add_transient(WClientWin *tfor, WClientWin *cwin,
						  const XWindowAttributes *attr,
						  int init_state)
{
	
	WRegion *p=(WRegion*)tfor;
	
	while(p!=NULL){
		if(HAS_DYN(p, region_ws_add_transient))
			return region_ws_add_transient(p, tfor, cwin, attr, init_state);
		p=REGION_MANAGER(p);
	}
	
	/* No parent workspace found that would want to handle transients.
	 * tfor is probably a full screen client window so let's just handle
	 * the transient like Ion normally does.
	 */
	return region_add_managed((WRegion*)tfor, (WRegion*)cwin, 0);
}


static WRegion *find_suitable_workspace(WViewport *vp)
{
	WRegion *r=vp->current_ws;
	
	if(r!=NULL && HAS_DYN(r, region_ws_add_clientwin))
		return r;

	FOR_ALL_MANAGED_ON_LIST(vp->ws_list, r){
		if(HAS_DYN(r, region_ws_add_clientwin))
			return r;
	}
	
	return NULL;
}


static void find_prop_target(WClientWin *cwin, WRegion **target,
							 WRegion **ws)
{
	WRegion *r;
	char *target_name;
	
	if(!extl_table_gets_s(cwin->proptab, "target", &target_name))
		return;
	
	/* Potential problem: numbering */
	r=lookup_region(target_name);
	
	free(target_name);
	
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

#ifdef CF_PLACEMENT_GEOM
	if(x>CF_STUBBORN_TRESH || y>CF_STUBBORN_TRESH ||
	   cwin->size_hints.win_gravity!=ForgetGravity){
		FOR_ALL_TYPED_CHILDREN(scr, vp, WViewport){
			WRectangle geom=REGION_GEOM(vp);
			if(x>=geom.x && x<geom.x+geom.w &&
			   y>=geom.y && y<geom.y+geom.h)
				return vp;
		}
	}
#endif
	
	if(scr->current_viewport!=NULL)
		return scr->current_viewport;
	
	return scr->default_viewport;
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
	WViewport *vp;
	int tm;
	
	vp=find_suitable_viewport(cwin, attr->x, attr->y);
	
	/* check full screen mode */
	do{
		WMwmHints *mwm;
		mwm=get_mwm_hints(cwin->win);
		if(mwm==NULL)
			break;
		if(mwm->flags&MWM_HINTS_DECORATIONS && mwm->decorations==0 &&
		   REGION_GEOM(SCREEN_OF(cwin)).w==attr->width &&
		   REGION_GEOM(SCREEN_OF(cwin)).h==attr->height){
			if(clientwin_fullscreen_vp(cwin, vp, clientwin_get_switchto(cwin)))
				return TRUE;
			warn("Failed to enter full screen mode.");
		}
	}while(0);
	
	tm=clientwin_get_transient_mode(cwin);
	
	if(tm!=TRANSIENT_MODE_CURRENT){
		if(tm==TRANSIENT_MODE_NORMAL){
			/* Is it a transient to some other window? */
			if(XGetTransientForHint(wglobal.dpy, win, &(cwin->transient_for))){
				tfor=find_clientwin(cwin->transient_for);
				if(tfor!=NULL && tfor!=cwin &&
				   add_transient(tfor, cwin, attr, init_state))
					return TRUE;
				cwin->transient_for=None;
			}
		}
		
		get_integer_property(win, wglobal.atom_frame_id, &target_id);
		
		if(target_id!=0)
			target=find_target_by_id(target_id);
		
		find_prop_target(cwin, target==NULL ? &target : NULL, &ws);
		
		if(target!=NULL){
			if(!region_supports_add_managed(target)){
				warn("Target region of window %#x does not support primitive "
					 "add_managed method", cwin->win);
				target=NULL;
			}else if(SCREEN_OF(target)!=SCREEN_OF(cwin)){
				warn("The target id property of window %#x is set to "
					 "a frame on different screen", cwin->win);
				target=NULL;
			}
		}
		
		/* Found a specific target frame or such? */
		if(target!=NULL)
			return finish_add_clientwin(target, cwin, init_state);
	}
	
	/* No, need to find a workspace and let it handle this. */
	
	if(ws==NULL)
		ws=find_suitable_workspace(vp);

	if(ws==NULL)
		return clientwin_fullscreen_vp(cwin, vp, FALSE);
	
	return region_ws_add_clientwin(ws, cwin, attr, init_state);
}


bool region_ws_add_clientwin(WRegion *reg, WClientWin *cwin,
							 const XWindowAttributes *attr, int init_state)
{
	bool ret=TRUE;
	CALL_DYN_RET(ret, bool, region_ws_add_clientwin,
				 reg, (reg, cwin, attr, init_state));
	return ret;
	
}


bool region_ws_add_transient(WRegion *reg, WClientWin *tfor, WClientWin *cwin,
							 const XWindowAttributes *attr, int init_state)
{
	bool ret=TRUE;
	CALL_DYN_RET(ret, bool, region_ws_add_transient,
				 reg, (reg, tfor, cwin, attr, init_state));
	return ret;
}


bool finish_add_clientwin(WRegion *reg, WClientWin *cwin,
						  bool init_state)
{
	int flags=0;

	assert(reg!=NULL);
	
	if(clientwin_get_switchto(cwin))
		flags|=REGION_ATTACH_SWITCHTO;
	
	if(init_state==IconicState)
		flags=0;
	
	return region_add_managed(reg, (WRegion*)cwin, flags);
}


/*}}}*/

