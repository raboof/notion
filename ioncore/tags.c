/*
 * ion/ioncore/tags.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */


#include "region.h"
#include "tags.h"
#include "objlist.h"


static WObjList *taglist=NULL;


/*{{{ Adding/removing tags */

/*EXTL_DOC
 * Tag \var{reg}.
 */
EXTL_EXPORT_MEMBER
void region_tag(WRegion *reg)
{
	if(reg->flags&REGION_TAGGED)
		return;
	
	/*clear_sub_tags(reg);*/
	
	objlist_insert(&taglist, (WObj*)reg);
	
	reg->flags|=REGION_TAGGED;
	region_notify_change(reg);
}


/*EXTL_DOC
 * Untag \var{reg}.
 */
EXTL_EXPORT_MEMBER
void region_untag(WRegion *reg)
{
	if(!(reg->flags&REGION_TAGGED))
		return;

	objlist_remove(&taglist, (WObj*)reg);
	
	reg->flags&=~REGION_TAGGED;
	region_notify_change(reg);
}


/*EXTL_DOC
 * Toggle region \var{reg} tag.
 */
EXTL_EXPORT_MEMBER
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
EXTL_EXPORT_MEMBER
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
	WRegion *reg;
	
	ITERATE_OBJLIST(WRegion*, reg, taglist){
		region_untag(reg);
	}
}


/*}}}*/


/*{{{ Iteration */


WRegion *tag_first()
{
	return (WRegion*)objlist_init_iter(taglist);
}


WRegion *tag_take_first()
{
	WRegion *reg=(WRegion*)objlist_init_iter(taglist);
	
	if(reg!=NULL)
		region_untag(reg);
	
	return reg;
}


WRegion *tag_next(WRegion *reg)
{
	return (WRegion*)objlist_iter(taglist);
}


/*}}}*/

