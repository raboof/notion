/*
 * ion/splitframe.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <string.h>

#include <wmcore/common.h>
#include <wmcore/global.h>
#include <wmcore/objp.h>
#include <wmcore/focus.h>
#include <wmcore/clientwin.h>
#include <wmcore/resize.h>
#include <query/fwarn.h>
#include "splitframe.h"
#include "split.h"
#include "framep.h"


typedef WRegion *WSplitCreate(WRegion *parent, WRectangle geom);
extern WRegion *split_reg(WRegion *reg, int dir, int primn,
						  int minsize, WSplitCreate *fn);
extern WRegion *split_toplevel(WIonWS *ws, int dir, int primn,
							   int minsize, WSplitCreate *fn);


/*{{{ Find */


static bool coord_not_in_rect(int x, int y, WRectangle geom)
{
	return (x<geom.x || x>=(geom.x+geom.w) ||
			y<geom.y || y>=(geom.y+geom.h));
}


static WObj *do_find_at(WObj *obj, int x, int y)
{
	WWsSplit *split;
	
	if(!WTHING_IS(obj, WWsSplit)){
		if(!WTHING_IS(obj, WRegion))
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


WFrame *find_frame_at(WIonWS *ws, int x, int y)
{
	WObj *obj=ws->split_tree;
	
	obj=do_find_at(obj, x, y);
	
	return WTHING_IS(obj, WFrame) ? (WFrame*)obj : NULL;
}


/*}}}*/


/*{{{ Split */


static WRegion* split_create_frame(WRegion *parent, WRectangle geom)
{
	return (WRegion*)create_frame(parent, geom, 0, 0);
}


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
	
	reg=split_reg(oreg, dir, primn, mins, split_create_frame);
	
	if(reg!=NULL){
		if(attach && WTHING_IS(oreg, WFrame) &&
		   ((WFrame*)oreg)->current_sub!=NULL){
			region_add_managed(reg, ((WFrame*)oreg)->current_sub, TRUE);
		}
		goto_region(reg);
	}else{
		warn("Unable to split");
	}
}


void split(WRegion *reg, const char *str)
{
	do_split(reg, str, TRUE);
}


void split_empty(WRegion *reg, const char *str)
{
	do_split(reg, str, FALSE);
}


void split_top(WIonWS *ws, const char *str)
{
	WRegion *reg;
	int dir, primn, mins;
	
	if(!get_splitparams(&dir, &primn, str))
		return;
	
	mins=(dir==VERTICAL
		  ? FRAME_MIN_H(SCREEN_OF(ws)) 
		  : FRAME_MIN_W(SCREEN_OF(ws)));
	
	reg=split_toplevel(ws, dir, primn, mins, split_create_frame);
	if(reg!=NULL)
		warp(reg);
}


/*}}}*/


/*{{{ Close */


void frame_close(WFrame *frame)
{
	WIonWS *ws;
	WViewport *vp;
	WRegion *other;
	bool was_active=REGION_IS_ACTIVE(frame);
	
	ws=(WIonWS*)REGION_MANAGER(frame);
	
	if(ws==NULL || !WTHING_IS(ws, WIonWS)){
		region_rescue_managed_on_list((WRegion*)frame, frame->managed_list);
		destroy_thing((WThing*)frame);
		return;
	}
	
	vp=(WViewport*)REGION_MANAGER(ws);
	
	if(vp!=NULL && WTHING_IS(vp, WViewport)){
		if(vp->ws_count<=1 && ws->split_tree==(WObj*)frame){
			fwarn(frame, "Cannot destroy only frame on only ionws.");
			return;
		}
	}
	
	other=ionws_find_new_manager((WRegion*)frame);
	
	if(frame->managed_count!=0){
		if(other==NULL || WTHING_IS(other, WScreen)){
			fwarn(frame, "Last frame on workspace and not empty: "
				  "refusing to destroy.");
			return;
		}
		region_move_managed_on_list(other, (WRegion*)frame,
									frame->managed_list);
		if(frame->managed_count==0)
			warn("Could not move all managed objects.");
	}

	if(other!=NULL && was_active)
		set_focus(other);
				 
	destroy_thing((WThing*)frame);
}


void frame_close_if_empty(WFrame *frame)
{
	if(frame->managed_count!=0 || frame->current_input!=NULL)
		return;
	
	frame_close(frame);
}


/*}}}*/

