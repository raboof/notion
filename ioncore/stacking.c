/*
 * ion/ioncore/stacking.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "common.h"
#include "region.h"
#include "window.h"
#include "stacking.h"


/*
 * Some notes on the stack lists:
 * 
 *  * "Normally" stacked regions are not on any list.
 *  * Regions that are stacked above some other region stacking.above are on
 *    stacking.above's stacking.stack_below_list.
 *  * Regions that are 'kept on top' inside their parent window or on the
 *    window's keep_on_top_list and have the flag REGION_KEEPONTOP set.
 */


/*{{{ Functions to set up stacking management */


/*EXTL_DOC
 * Inform that \var{reg} should be stacked above the region \var{above}.
 */
EXTL_EXPORT
void region_stack_above(WRegion *reg, WRegion *above)
{
	WRegion *r2;
	
	if(region_x_window(reg)==None || region_x_window(above)==None){
		warn("Stack-managed regions must have a window associated to them.");
		return;
	}
	
	region_reset_stacking(reg);
	
	r2=above->stacking.below_list;
	if(r2==NULL)
		r2=above;
	else
		r2=r2->stacking.prev;
	
	region_restack(reg, region_x_window(r2), Above);
	
	LINK_ITEM(above->stacking.below_list, reg, stacking.next, stacking.prev);
	reg->stacking.above=above;
}


/*EXTL_DOC
 * Inform that \var{reg} should be kept above normally stacked regions
 * within its parent.
 */
EXTL_EXPORT
void region_keep_on_top(WRegion *reg)
{
	WWindow *par;

	if(region_x_window(reg)==None){
		warn("Stack-managed regions must have a window associated to them.");
		return;
	}
	
	par=REGION_PARENT_CHK(reg, WWindow);
	
	if(par==NULL)
		return;
	
	region_reset_stacking(reg);

	region_restack(reg, None, Above);
	
	LINK_ITEM(par->keep_on_top_list, reg, stacking.next, stacking.prev);
	reg->flags|=REGION_KEEPONTOP;
}


static void do_reset_stacking(WRegion *reg)
{
	if(reg->stacking.above){
		UNLINK_ITEM(reg->stacking.above->stacking.below_list,
					reg, stacking.next, stacking.prev);
		reg->stacking.above=NULL;
	}else if(reg->flags&REGION_KEEPONTOP){
		WWindow *par=REGION_PARENT_CHK(reg, WWindow);
		assert(par!=NULL);
		UNLINK_ITEM(par->keep_on_top_list, reg, stacking.next, stacking.prev);
		reg->flags&=~REGION_KEEPONTOP;
	}
}


/*EXTL_DOC
 * Inform that \var{reg} should be stacked normally.
 */
EXTL_EXPORT
void region_reset_stacking(WRegion *reg)
{
	WRegion *r2, *next;
	
	do_reset_stacking(reg);
	
	for(r2=reg->stacking.below_list; r2!=NULL; r2=next){
		next=r2->stacking.next;
		do_reset_stacking(r2);
	}
}
		

static void movetotop(WRegion *reg)
{
	if(reg->stacking.above){
		UNLINK_ITEM(reg->stacking.above->stacking.below_list,
					reg, stacking.next, stacking.prev);
		LINK_ITEM(reg->stacking.above->stacking.below_list,
				  reg, stacking.next, stacking.prev);
	}else if(reg->flags&REGION_KEEPONTOP){
		WWindow *par=REGION_PARENT_CHK(reg, WWindow);
		assert(par!=NULL);
		UNLINK_ITEM(par->keep_on_top_list, reg, stacking.next, stacking.prev);
		LINK_ITEM(par->keep_on_top_list, reg, stacking.next, stacking.prev);
	}
}


static void movetobottom(WRegion *reg)
{
	if(reg->stacking.above){
		UNLINK_ITEM(reg->stacking.above->stacking.below_list,
					reg, stacking.next, stacking.prev);
		LINK_ITEM_FIRST(reg->stacking.above->stacking.below_list,
						reg, stacking.next, stacking.prev);
	}else if(reg->flags&REGION_KEEPONTOP){
		WWindow *par=REGION_PARENT_CHK(reg, WWindow);
		assert(par!=NULL);
		UNLINK_ITEM(par->keep_on_top_list, reg, stacking.next, stacking.prev);
		LINK_ITEM_FIRST(par->keep_on_top_list, reg, stacking.next,
						stacking.prev);
	}
}

	
/*}}}*/


/*{{{ Functions to restack */


/*EXTL_DOC
 * Raise \var{reg} in the stack. The regions marked to be stacked above
 * \var{reg} will also be raised and normally stacked regions will not
 * be raised above region marked to be kept on top.
 */
EXTL_EXPORT
void region_raise(WRegion *reg)
{
	WRegion *r2;
	bool st=FALSE;
	Window w=None;
	
	r2=reg;
	while(r2->stacking.above!=NULL)
		r2=r2->stacking.above;
	   
	if(!(r2->flags&REGION_KEEPONTOP)){
		/* Stack below lowest keepontop window */
		WWindow *par=REGION_PARENT_CHK(reg, WWindow);
		if(par!=NULL && par->keep_on_top_list!=NULL){
			w=region_x_window(par->keep_on_top_list);
			w=region_restack(reg, w, Below);
			st=TRUE;
		}
	}
	
	if(!st)
		w=region_restack(reg, None, Above);
	
	/* Restack window that are stacked above this one */
	for(r2=reg->stacking.below_list; r2!=NULL; r2=r2->stacking.next)
		w=region_restack(r2, w, Above);
	
	movetotop(reg);
}


/*EXTL_DOC
 * Lower \var{reg} in the stack. The regions marked to be stacked above
 * \var{reg} will also be lowerd and regions marked to be kept on top
 * will not be lowered below normally stacked regions.
 */
EXTL_EXPORT
void region_lower(WRegion *reg)
{
	WRegion *r2;
	Window w;
	
	r2=reg;
	while(reg->stacking.above){
		reg=reg->stacking.above;
		/* Check for loops in stack hierarchy */
		assert(reg!=r2);
	}
	
	if(reg->flags&REGION_KEEPONTOP){
		/* Stack below lowest keepontop window */
		WWindow *par=REGION_PARENT_CHK(reg, WWindow);
		if(par==NULL || par->keep_on_top_list==reg)
			return;
		w=region_x_window(par->keep_on_top_list);
		w=region_restack(reg, w, Below);
	}else{
		w=region_restack(reg, None, Below);
	}
	
	/* Restack window that are stacked above this one */
	for(r2=reg->stacking.below_list; r2!=NULL; r2=r2->stacking.next)
		w=region_restack(r2, w, Above);

	movetobottom(reg);
}


/* region_restack should return the topmost window */
Window region_restack(WRegion *reg, Window other, int mode)
{
	Window ret=None;
	CALL_DYN_RET(ret, Window, region_restack, reg, (reg, other, mode));
	return ret;
}


/*}}}*/


/*{{{ Misc */


WRegion *region_topmost_stacked_above(WRegion *reg)
{
	if(reg->stacking.below_list==NULL)
		return NULL;
	return reg->stacking.below_list->stacking.prev;
}


/*}}}*/

