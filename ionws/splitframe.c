/*
 * ion/ionws/splitframe.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/objp.h>
#include <ioncore/focus.h>
#include <ioncore/clientwin.h>
#include <ioncore/resize.h>
#include <ioncore/framep.h>
#include <ioncore/defer.h>
#include "splitframe.h"
#include "split.h"


/*{{{ Find */


static bool coord_not_in_rect(int x, int y, const WRectangle *geom)
{
	return (x<geom->x || x>=(geom->x+geom->w) ||
			y<geom->y || y>=(geom->y+geom->h));
}


static WObj *do_find_at(WObj *obj, int x, int y)
{
	WWsSplit *split;
	
	if(!WOBJ_IS(obj, WWsSplit)){
		if(!WOBJ_IS(obj, WRegion))
			return NULL;
		if(coord_not_in_rect(x, y, &REGION_GEOM((WRegion*)obj)))
			return NULL;
		return obj;
	}
	
	split=(WWsSplit*)obj;
	
	if(coord_not_in_rect(x, y, &(split->geom)))
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


static WIonFrame *do_split(WIonFrame *oframe, const char *str, bool attach)
{
	WRegion *reg;
	int dir, primn, mins;
	
	if(!get_split_dir_primn(str, &dir, &primn)){
		warn("Unknown parameter to do_split");
		return NULL;
	}
	
	mins=(dir==VERTICAL
		  ? region_min_h((WRegion*)oframe)
		  : region_min_w((WRegion*)oframe));
	
	reg=split_reg((WRegion*)oframe, dir, primn, mins,
				  (WRegionSimpleCreateFn*)create_ionframe);
	
	if(reg==NULL){
		warn("Unable to split");
		return NULL;
	}

	assert(WOBJ_IS(reg, WIonFrame));
	
	if(attach && WGENFRAME_CURRENT(oframe)!=NULL)
		mplex_attach_simple((WMPlex*)reg, WGENFRAME_CURRENT(oframe), TRUE);
	
	if(region_may_control_focus((WRegion*)oframe))
		region_goto(reg);

	return (WIonFrame*)reg;
}


/*EXTL_DOC
 * Split \var{frame} creating a new WIonFrame to direction \var{dir}
 * (one of ''left'', ''right'', ''top'' or ''bottom'') of \var{frame}.
 * The active manages region in \var{frame}, if any, is moved to the
 * new frame.
 */
EXTL_EXPORT_MEMBER
WIonFrame *ionframe_split(WIonFrame *frame, const char *dirstr)
{
	return do_split(frame, dirstr, TRUE);
}

/*EXTL_DOC
 * Similar to \fnref{WIonFrame.split} except nothing is moved to the newly
 * created frame.
 */
EXTL_EXPORT_MEMBER
WIonFrame *ionframe_split_empty(WIonFrame *frame, const char *dirstr)
{
	return do_split(frame, dirstr, FALSE);
}


/*EXTL_DOC
 * Create new WIonFrame on \var{ws} above/below/left of/right of
 * all other objects depending on \var{dirstr}
 * (one of ''left'', ''right'', ''top'' or ''bottom'').
 */
EXTL_EXPORT_MEMBER
WIonFrame *ionws_newframe(WIonWS *ws, const char *dirstr)
{
	WRegion *reg;
	int dir, primn, mins;
	
	if(!get_split_dir_primn(dirstr, &dir, &primn))
		return NULL;
	
	/*mins=(dir==VERTICAL ? GRDATA_OF(ws)->h_unit : GRDATA_OF(ws)->w_unit);*/
	mins=1;
	
	reg=split_toplevel(ws, dir, primn, mins,
					   (WRegionSimpleCreateFn*)create_ionframe);
	if(reg!=NULL)
		warp(reg);
	
	return (WIonFrame*)reg;
}


/*}}}*/


/*{{{ Close */


/*EXTL_DOC
 * Try to relocate regions managed by \var{frame} somewhere else
 * and if possible, destroy the frame.
 */
EXTL_EXPORT_MEMBER
void ionframe_relocate_and_close(WIonFrame *frame)
{
	if(!region_may_destroy((WRegion*)frame)){
		warn("Frame may not be destroyed");
		return;
	}

	if(!region_rescue_clientwins((WRegion*)frame)){
		warn("Failed to rescue managed objects.");
		return;
	}

	defer_destroy((WObj*)frame);
}


void ionframe_close(WIonFrame *frame)
{
	if(WGENFRAME_MCOUNT(frame)!=0 || WGENFRAME_CURRENT(frame)!=NULL){
		warn("Frame not empty.");
		return;
	}
	
	ionframe_relocate_and_close(frame);
}


/*}}}*/

