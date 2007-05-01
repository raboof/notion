/*
 * ion/ioncore/tags.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
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
 * (one of \codestr{set}, \codestr{unset}, or \codestr{toggle}).
 * The resulting state is returned.
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
void ioncore_tagged_clear()
{
    while(ioncore_tagged_first(TRUE)!=NULL)
        /* nothing */;
}


/*}}}*/


/*{{{ Iteration */


/*EXTL_DOC
 * Returns first tagged object, untagging it as well if \var{untag}is set.
 */
EXTL_SAFE
EXTL_EXPORT
WRegion *ioncore_tagged_first(bool untag)
{
    WRegion *reg;
    
    if(!untag){
        reg=(WRegion*)OBJLIST_FIRST(WRegion*, taglist);
    }else{
        reg=(WRegion*)objlist_take_first(&taglist);
    
        if(reg!=NULL){
            reg->flags&=~REGION_TAGGED;
            region_notify_change(reg, ioncore_g.notifies.tag);
        }
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

