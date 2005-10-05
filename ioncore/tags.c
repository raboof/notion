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
#include <libtu/setparam.h>
#include "region.h"
#include "tags.h"


static ObjList *taglist=NULL;


/*{{{ Adding/removing tags */


bool region_set_tagged(WRegion *reg, int sp)
{
    bool set=(reg->flags&REGION_TAGGED);
    bool nset=libtu_do_setparam(sp, set);
    
    if(XOR(nset, set)){
        if(reg->flags&REGION_TAGGED){
            reg->flags&=~REGION_TAGGED;
            objlist_remove(&taglist, (Obj*)reg);
        }else{
            reg->flags|=REGION_TAGGED;
            objlist_insert_last(&taglist, (Obj*)reg);
        }
        region_notify_change(reg);
    }

    return nset;
}


/*EXTL_DOC
 * Change tagging state of \var{reg} as defined by \var{how}
 * (set/unset/toggle). Resulting state is returned.
 */
EXTL_EXPORT_AS(WRegion, set_tagged)
bool region_set_tagged_extl(WRegion *reg, const char *how)
{
    return region_set_tagged(reg, libtu_string_to_setparam(how));
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
    while(ioncore_tags_take_first()!=NULL)
        /* nothing */;
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
    
    if(reg!=NULL){
        reg->flags&=~REGION_TAGGED;
        region_notify_change(reg);
    }
    
    return reg;
}

/*EXTL_DOC
 * Returns a list of tagged regions.
 */
EXTL_SAFE
EXTL_EXPORT
ExtlTab ioncore_tagged_list()
{
    int n=0;
    ExtlTab tab;
    WRegion *region;
    ObjListIterTmp tmp;

    region=ioncore_tags_first();
    if(!region)
        return extl_table_none();

    tab=extl_create_table();

    FOR_ALL_ON_OBJLIST(WRegion*, region, taglist, tmp){
        if(extl_table_seti_o(tab, n+1, (Obj*)region))
            n++;
    }

    return tab;
}


/*}}}*/

