/*
 * ion/ionws/splitframe.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <string.h>

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/objp.h>
#include <ioncore/focus.h>
#include <ioncore/clientwin.h>
#include <ioncore/resize.h>
#include <ioncore/genframep.h>
#include "splitframe.h"
#include "split.h"


/*{{{ Find */


static bool coord_not_in_rect(int x, int y, WRectangle geom)
{
	return (x<geom.x || x>=(geom.x+geom.w) ||
			y<geom.y || y>=(geom.y+geom.h));
}


static WObj *do_find_at(WObj *obj, int x, int y)
{
	WWsSplit *split;
	
	if(!WOBJ_IS(obj, WWsSplit)){
		if(!WOBJ_IS(obj, WRegion))
			return NULL;
		if(coord_not_in_rect(x, y, REGION_GEOM((WRegion*)obj)))
			return NULL;
		return obj;
	}
	
	split=(WWsSplit*)obj;
	
	if(coord_not_in_rect(x, y, split->geom))
		return NULL;
	
	obj=do_find_at(split->tl, x, y);
	if(obj==NULL)
		obj=do_find_at(split->br, x, y);
	return obj;
}


WIonFrame *find_frame_at(WIonWS *ws, int x, int y)
{
	WObj *obj=ws->split_tree;
	
	obj=do_find_at(obj, x, y);
	
	return WOBJ_IS(obj, WIonFrame) ? (WIonFrame*)obj : NULL;
}


/*}}}*/


/*{{{ Split */


bool get_splitparams(int *dir, int *primn, const char *str)
{
	if(str==NULL)
		return FALSE;
	
	if(!strcmp(str, "left")){
		*primn=TOP_OR_LEFT;
		*dir=HORIZONTAL;
	}else if(!strcmp(str, "right")){
		*primn=BOTTOM_OR_RIGHT;
		*dir=HORIZONTAL;
	}else if(!strcmp(str, "top")){
		*primn=TOP_OR_LEFT;
		*dir=VERTICAL;
	}else if(!strcmp(str, "bottom")){
		*primn=BOTTOM_OR_RIGHT;
		*dir=VERTICAL;
	}else{
		return FALSE;
	}
	
	return TRUE;
}


static void do_split(WRegion *oreg, const char *str, bool attach)
{
	WRegion *reg;
	int dir, primn, mins;
	
	if(!get_splitparams(&dir, &primn, str)){
		warn("Unknown parameter to do_split");
		return;
	}
	
	mins=(dir==VERTICAL ? region_min_h(oreg) : region_min_w(oreg));
	
	reg=split_reg(oreg, dir, primn, mins,
				  (WRegionSimpleCreateFn*)create_ionframe_simple);
	
	if(reg!=NULL){
		if(attach && WOBJ_IS(oreg, WIonFrame) &&
		   ((WIonFrame*)oreg)->genframe.current_sub!=NULL){
			region_add_managed(reg, ((WIonFrame*)oreg)->genframe.current_sub,
							   REGION_ATTACH_SWITCHTO);
		}
		region_goto(reg);
	}else{
		warn("Unable to split");
	}
}


EXTL_EXPORT
void ionws_split(WRegion *reg, const char *str)
{
	do_split(reg, str, TRUE);
}


EXTL_EXPORT
void ionws_split_empty(WRegion *reg, const char *str)
{
	do_split(reg, str, FALSE);
}


EXTL_EXPORT
void ionws_split_top(WIonWS *ws, const char *str)
{
	WRegion *reg;
	int dir, primn, mins;
	
	if(!get_splitparams(&dir, &primn, str))
		return;
	
	mins=(dir==VERTICAL
		  ? WGENFRAME_MIN_H(SCREEN_OF(ws)) 
		  : WGENFRAME_MIN_W(SCREEN_OF(ws)));
	
	reg=split_toplevel(ws, dir, primn, mins,
					   (WRegionSimpleCreateFn*)create_ionframe_simple);
	if(reg!=NULL)
		warp(reg);
}


/*}}}*/


/*{{{ Close */


EXTL_EXPORT
void ionframe_relocate_and_close(WIonFrame *frame)
{
	WIonWS *ws;
	WViewport *vp;
	WRegion *other;
	bool was_active=REGION_IS_ACTIVE(frame);
	
	ws=(WIonWS*)REGION_MANAGER(frame);
	
	if(ws==NULL || !WOBJ_IS(ws, WIonWS)){
		region_rescue_managed_on_list((WRegion*)frame,
									  frame->genframe.managed_list);
		destroy_obj((WObj*)frame);
		return;
	}
	
	vp=(WViewport*)REGION_MANAGER(ws);
	
	if(vp!=NULL && WOBJ_IS(vp, WViewport)){
		if(vp->ws_count<=1 && ws->split_tree==(WObj*)frame){
			warn("Cannot destroy only frame on only ionws.");
			return;
		}
	}
	
	other=ionws_find_new_manager((WRegion*)frame);
	
	if(frame->genframe.managed_count!=0){
		if(other==NULL || WOBJ_IS(other, WScreen)){
			warn("Last frame on workspace and not empty: refusing to "
				 "destroy.");
			return;
		}
		region_move_managed_on_list(other, (WRegion*)frame,
									frame->genframe.managed_list);
		if(frame->genframe.managed_count==0)
			warn("Could not move all managed objects.");
	}

	if(other!=NULL && was_active)
		set_focus(other);
				 
	destroy_obj((WObj*)frame);
}


EXTL_EXPORT
void ionframe_close(WIonFrame *frame)
{
	if(frame->genframe.managed_count!=0 ||
	   frame->genframe.current_input!=NULL)
		return;
	
	ionframe_relocate_and_close(frame);
}


/*}}}*/

