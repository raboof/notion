/*
 * ion/ioncore/return.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#include <libtu/rb.h>
#include <libmainloop/defer.h>

#include "common.h"
#include "global.h"
#include "region.h"
#include "pholder.h"
#include "return.h"


/*{{{ Storage tree */


static Rb_node retrb=NULL;


/*}}}*/



/*{{{ Set */


bool region_do_set_return(WRegion *reg, WPHolder *ph)
{
    Rb_node node;
    int found=0;
    
    assert(!OBJ_IS_BEING_DESTROYED(reg));
    
    region_unset_return(reg);
    
    if(retrb==NULL){
        retrb=make_rb();
        if(retrb==NULL)
            return FALSE;
    }
    
    node=rb_insertp(retrb, reg, ph);
    
    region_notify_change(reg, ioncore_g.notifies.set_return);
    
    return (node!=NULL);
}


WPHolder *region_make_return_pholder(WRegion *reg)
{
    WRegion *mgr=region_manager(reg);
    
    if(mgr==NULL)
        return NULL;
    
    return region_managed_get_pholder(mgr, reg);
}


/*
extern WPHolder *region_set_return(WRegion *reg)
{
    WPHolder *ph=region_make_return_pholder(reg);
    
    if(ph!=NULL){
        if(region_do_set_return(reg, ph))
            return ph;
        destroy_obj((Obj*)ph);
    }
    
    return NULL;
}
*/


/*}}}*/


/*{{{ Get */

Rb_node do_find(WRegion *reg)
{
    int found=0;
    Rb_node node;
    
    if(retrb==NULL)
        return NULL;
    
    node=rb_find_pkey_n(retrb, reg, &found);
    
    return (found ? node : NULL);
}


WPHolder *region_do_get_return(WRegion *reg)
{
    Rb_node node=do_find(reg);
    
    return (node!=NULL ? (WPHolder*)node->v.val : NULL);
}


WPHolder *region_get_return(WRegion *reg)
{
    /* Should managers be scanned? */
    return region_do_get_return(reg);
}


/*}}}*/


/*{{{ Unset */


static WPHolder *do_remove_node(Rb_node node)
{
    WPHolder *ph=(WPHolder*)node->v.val;
   
    rb_delete_node(node);
   
    return ph;
}


WPHolder *region_unset_get_return(WRegion *reg)
{
    Rb_node node; 
    
    node=do_find(reg);
    
    if(node!=NULL){
        region_notify_change(reg, ioncore_g.notifies.unset_return);
        return do_remove_node(node);
    }else{
        return NULL;
    }
}


void region_unset_return(WRegion *reg)
{
    WPHolder *ph=region_unset_get_return(reg);
    
    if(ph!=NULL)
        mainloop_defer_destroy((Obj*)ph);
}


/*}}}*/

