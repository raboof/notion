/*
 * ion/ioncore/return.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/rb.h>

#include "region.h"
#include "pholder.h"
#include "return.h"

static Rb_node retrb=NULL;


/*{{{ Set */


bool region_do_set_return(WRegion *reg, WPHolder *ph)
{
    Rb_node node;
    int found=0;
    
    region_unset_return(reg);
    
    if(retrb==NULL){
        retrb=make_rb();
        if(retrb==NULL)
            return FALSE;
    }
    
    node=rb_insertp(retrb, reg, ph);
    
    return (node!=NULL);
}


static WPHolder *make_return_pholder(WRegion *reg)
{
    WRegion *mgr=region_manager(reg);
    
    if(mgr==NULL)
        return NULL;
    
    return region_managed_get_pholder(mgr, reg);
}


extern WPHolder *region_set_return(WRegion *reg)
{
    WPHolder *ph=make_return_pholder(reg);
    
    if(ph!=NULL){
        if(region_do_set_return(reg, ph))
            return ph;
        destroy_obj((Obj*)ph);
    }
    
    return NULL;
}


/*}}}*/


/*{{{ Get */


WPHolder *region_do_get_return(WRegion *reg)
{
    Rb_node node; 
    int found=0;
    
    if(retrb==NULL)
        return NULL;
    
    node=rb_find_pkey_n(retrb, reg, &found);
    
    return (found ? (WPHolder*)node->v.val : NULL);
}


WPHolder *region_get_return(WRegion *reg)
{
    /* Should managers be scanned? */
    return region_do_get_return(reg);
}


/*}}}*/


/*{{{ Unset */


void region_unset_return(WRegion *reg)
{
    Rb_node node; 
    int found=0;
    
    if(retrb!=NULL){
        node=rb_find_pkey_n(retrb, reg, &found);
    
        if(found){
            Obj *ph=(Obj*)node->v.val;
            rb_delete_node(node);
            if(ph!=NULL)
                destroy_obj(ph);
        }
    }
}


/*}}}*/

