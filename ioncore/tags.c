/*
 * wmcore/tags.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */


#include "region.h"
#include "tags.h"


static WRegion *taglist;


/*{{{ Adding/removing tags */


/*
void clear_sub_tags(WRegion *reg)
{
	WRegion *sub;
	
	if(taglist==NULL)
		return;
	
	untag_region(reg);
	
	FOR_ALL_TYPED(reg, sub, WRegion){
		clear_sub_tags(sub);
	}
}
*/

void tag_region(WRegion *reg)
{
	if(reg->flags&REGION_TAGGED)
		return;
	
	/*clear_sub_tags(reg);*/
	
	LINK_ITEM(taglist, reg, tag_next, tag_prev);
	
	reg->flags|=REGION_TAGGED;
	region_notify_change(reg);
}


void untag_region(WRegion *reg)
{
	if(!(reg->flags&REGION_TAGGED))
		return;

	UNLINK_ITEM(taglist, reg, tag_next, tag_prev);
	
	reg->flags&=~REGION_TAGGED;
	region_notify_change(reg);
}


void toggle_region_tag(WRegion *reg)
{
	if(reg->flags&REGION_TAGGED)
		untag_region(reg);
	else
		tag_region(reg);
}


void clear_tags(WRegion *reg)
{
	while(taglist!=NULL)
		untag_region(taglist);
}


/*}}}*/



/*{{{ Iteration */


WRegion *tag_first()
{
	return taglist;
}


WRegion *tag_take_first()
{
	WRegion *reg=taglist;
	
	if(reg!=NULL)
		untag_region(reg);
	
	return reg;
}


WRegion *tag_next(WRegion *reg)
{
	return reg->tag_next;
}


/*}}}*/

