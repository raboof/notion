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


/* {{{{ Storage tree and sorting */

static Rb_node retrb=NULL;


int watch_cmp(Watch *w1, Watch *w2)
{
    if(w1->obj < w2->obj)
        return -1;
    else if(w1->obj == w2->obj)
        return 0;
    else
        return 1;
}


static void do_remove_node(Rb_node node);


static void retrb_watch_handler(Watch *watch, Obj *obj)
{
    Rb_node node;
    int found=0;
    
    node=rb_find_gkey_n(retrb, &watch, (Rb_compfn*)watch_cmp, &found);
    
    if(found)
        do_remove_node(node);
}


/*}}}*/



/*{{{ Set */


bool region_do_set_return(WRegion *reg, WPHolder *ph)
{
    Rb_node node;
    Watch *watch;
    int found=0;
    
    region_unset_return(reg);
    
    if(retrb==NULL){
        retrb=make_rb();
        if(retrb==NULL)
            return FALSE;
    }
    
    watch=ALLOC(Watch);
    
    if(watch==NULL)
        return FALSE;
        
    watch_init(watch);
    watch_setup(watch, (Obj*)reg, &retrb_watch_handler);
    
    node=rb_insertg(retrb, watch, ph, (Rb_compfn*)watch_cmp);
    
    if(node==NULL){
        watch_reset(watch);
        free(watch);
    }
    
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
    Watch watch;
    int found=0;
    Rb_node node;
    
    if(retrb==NULL)
        return NULL;
    
    watch_init(&watch);
    
    watch.obj=(Obj*)reg;
    
    node=rb_find_gkey_n(retrb, &watch, (Rb_compfn*)watch_cmp, &found);
    
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


static WPHolder *do_remove_node_step1(Rb_node node)
{
   WPHolder *ph=(WPHolder*)node->v.val;
   Watch *watch=(Watch*)node->k.key;
            
   rb_delete_node(node);
            
   watch_reset(watch);
   free(watch);
   
   return ph;
}


static void do_remove_node(Rb_node node)
{
    WPHolder *ph=do_remove_node_step1(node);
    if(ph!=NULL)
        mainloop_defer_destroy((Obj*)ph);
        /*destroy_obj((Obj*)ph);*/
}


void region_unset_return(WRegion *reg)
{
    Rb_node node; 
    
    node=do_find(reg);
    
    if(node!=NULL)
        do_remove_node(node);
}

WPHolder *region_unset_get_return(WRegion *reg)
{
    Rb_node node; 
    
    node=do_find(reg);
    
    if(node!=NULL)
        return do_remove_node_step1(node);
    else
        return NULL;
}


/*}}}*/

