/*
 * ion/splitframe.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <wmcore/focus.h>
#include <wmcore/clientwin.h>
#include <wmcore/resize.h>
#include <query/fwarn.h>
#include "splitframe.h"
#include "split.h"
#include "framep.h"
#include <wmcore/global.h>
#include <wmcore/close.h>
#include <wmcore/objp.h>

typedef WRegion *WSplitCreate(WScreen *scr, WWinGeomParams params);
extern WRegion *split_reg(WRegion *reg, int dir, int primn,
						  int minsize, WSplitCreate *fn);
extern WRegion *split_toplevel(WWorkspace *ws, int dir, int primn,
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


WFrame *find_frame_at(WWorkspace *ws, int x, int y)
{
	WObj *obj=ws->splitree;
	
	obj=do_find_at(obj, x, y);
	
	return WTHING_IS(obj, WFrame) ? (WFrame*)obj : NULL;
}


/*}}}*/


/*{{{ Split */


static WRegion* split_create_frame(WScreen *scr, WWinGeomParams params)
{
	return (WRegion*)create_frame(scr, params, 0, 0);
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

	if(!get_splitparams(&dir, &primn, str))
		return;

	mins=(dir==VERTICAL ? region_min_h(oreg) : region_min_w(oreg));
	
	reg=split_reg(oreg, dir, primn, mins, split_create_frame);

	if(reg!=NULL){
		if(attach && WTHING_IS(oreg, WFrame) &&
		   ((WFrame*)oreg)->current_sub!=NULL){
			frame_attach_sub((WFrame*)reg, ((WFrame*)oreg)->current_sub, TRUE);
		}
		goto_region(reg);
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


void split_top(WWorkspace *ws, const char *str)
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


void destroy_frame(WFrame *frame)
{
	WWorkspace *ws;
	WScreen *scr;
	WRegion *other;
	bool was_active=REGION_IS_ACTIVE(frame);

	ws=FIND_PARENT1(frame, WWorkspace);

	if(ws!=NULL){
		scr=FIND_PARENT(ws, WScreen);
		assert(scr!=NULL);
		
		if(scr->sub_count<=1 && ws->splitree==(WObj*)frame){
			fwarn(frame, "Cannot destroy only frame on only workspace.");
			return;
		}
	}
	
	other=workspace_find_new_home((WRegion*)frame);
	
	if(frame->sub_count!=0){
		if(other==NULL || WTHING_IS(other, WScreen)){
			fwarn(frame, "Last frame on workspace and not empty: "
				  "refusing to destroy.");
			return;
		}
		region_move_subregions(other, (WRegion*)frame);
		/*SET_FOCUS(((WWindow*)frame)->win);*/

		assert(frame->sub_count==0);
	}
	
	if(other!=NULL && was_active)
		set_focus(other);
				 
	destroy_thing((WThing*)frame);
}


/*}}}*/

