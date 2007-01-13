/*
 * ion/ioncore/tags.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/objlist.h>
#include <libtu/setparam.h>

#include "global.h"
#include "region.h"
#include "tags.h"
#include "extlconv.h"


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
        region_notify_change(reg, ioncore_g.notifies.tag);
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
    while(ioncore_tagged_take_first()!=NULL)
        /* nothing */;
}


/*}}}*/


/*{{{ Iteration */


/*EXTL_DOC
 * Returns first tagged object.
 */
EXTL_SAFE
EXTL_EXPORT
WRegion *ioncore_tagged_first()
{
    return (WRegion*)OBJLIST_FIRST(WRegion*, taglist);
}


WRegion *ioncore_tagged_take_first()
{
    WRegion *reg=(WRegion*)objlist_take_first(&taglist);
    
    if(reg!=NULL){
        reg->flags&=~REGION_TAGGED;
        region_notify_change(reg, ioncore_g.notifies.tag);
    }
    
    return reg;
}

/*EXTL_DOC
 * Iterate over tagged regions until \var{iterfn} returns \code{false}.
 * The function itself returns \code{true} if it reaches the end of list
 * without this happening.
 */
EXTL_SAFE
EXTL_EXPORT
bool ioncore_tagged_i(ExtlFn iterfn)
{
    return extl_iter_objlist(iterfn, taglist);
}


/*}}}*/

