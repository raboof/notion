/*
 * ion/ioncore/tags.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */


#include "region.h"
#include "tags.h"


static WRegion *taglist;


/*{{{ Adding/removing tags */

/*EXTL_DOC
 * Tag \var{reg}.
 */
EXTL_EXPORT
void region_tag(WRegion *reg)
{
	if(reg->flags&REGION_TAGGED)
		return;
	
	/*clear_sub_tags(reg);*/
	
	LINK_ITEM(taglist, reg, tag_next, tag_prev);
	
	reg->flags|=REGION_TAGGED;
	region_notify_change(reg);
}


/*EXTL_DOC
 * Untag \var{reg}.
 */
EXTL_EXPORT
void region_untag(WRegion *reg)
{
	if(!(reg->flags&REGION_TAGGED))
		return;

	UNLINK_ITEM(taglist, reg, tag_next, tag_prev);
	
	reg->flags&=~REGION_TAGGED;
	region_notify_change(reg);
}


/*EXTL_DOC
 * Toggle region \var{reg} tag.
 */
EXTL_EXPORT
void region_toggle_tag(WRegion *reg)
{
	if(reg->flags&REGION_TAGGED)
		region_untag(reg);
	else
		region_tag(reg);
}


/*EXTL_DOC
 * Is \var{reg} tagged?
 */
EXTL_EXPORT
bool region_is_tagged(WRegion *reg)
{
	return ((reg->flags&REGION_TAGGED)!=0);
}


/*EXTL_DOC
 * Untag all regions.
 */
EXTL_EXPORT
void clear_tags()
{
	while(taglist!=NULL)
		region_untag(taglist);
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
		region_untag(reg);
	
	return reg;
}


WRegion *tag_next(WRegion *reg)
{
	return reg->tag_next;
}


/*}}}*/

