/*
 * ion/ioncore/tags.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/objlist.h>
#include "region.h"
#include "tags.h"


static ObjList *taglist=NULL;


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
    
    objlist_insert_last(&taglist, (Obj*)reg);
    
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

    objlist_remove(&taglist, (Obj*)reg);
    
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
EXTL_SAFE
EXTL_EXPORT_MEMBER
bool region_is_tagged(WRegion *reg)
{
    return ((reg->flags&REGION_TAGGED)!=0);
}


/*EXTL_DOC
 * Untag all regions.
 */
EXTL_EXPORT
void ioncore_clear_tags()
{
    WRegion *reg;
    ObjListIterTmp tmp;
    
    FOR_ALL_ON_OBJLIST(WRegion*, reg, taglist, tmp){
        region_untag(reg);
    }
}


/*}}}*/


/*{{{ Iteration */


WRegion *ioncore_tags_first()
{
    return (WRegion*)OBJLIST_FIRST(WRegion*, taglist);
}


WRegion *ioncore_tags_take_first()
{
    WRegion *reg=(WRegion*)objlist_take_first(&taglist);
    
    if(reg!=NULL)
        reg->flags&=~REGION_TAGGED;
    
    return reg;
}


/*}}}*/

