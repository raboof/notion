/*
 * ion/panews/main.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/map.h>

#include <ioncore/common.h>
#include <ioncore/reginfo.h>
#include <libextl/readconfig.h>
#include <ioncore/framep.h>
#include <ioncore/bindmaps.h>
#include <ioncore/bindmaps.h>

#include "main.h"
#include "panews.h"
#include "placement.h"


/*{{{ Module information */


#include "../version.h"

char mod_panews_ion_api_version[]=ION_API_VERSION;


/*}}}*/


/*{{{ Bindmaps */


WBindmap *mod_panews_panews_bindmap=NULL;
WBindmap *mod_panews_frame_bindmap=NULL;
WBindmap *mod_panews_unusedwin_bindmap=NULL;


/*}}}*/


/*{{{ Module init & deinit */


extern bool mod_panews_register_exports();
extern void mod_panews_unregister_exports();


void mod_panews_deinit()
{
    mod_panews_unregister_exports();
    ioncore_unregister_regclass(&CLASSDESCR(WPaneWS));
    
    if(mod_panews_panews_bindmap!=NULL){
        ioncore_free_bindmap("WPaneWS", mod_panews_panews_bindmap);
        mod_panews_panews_bindmap=NULL;
    }

    if(mod_panews_unusedwin_bindmap!=NULL){
        ioncore_free_bindmap("WUnusedWin", mod_panews_unusedwin_bindmap);
        mod_panews_unusedwin_bindmap=NULL;
    }
    
    if(mod_panews_frame_bindmap!=NULL){
        ioncore_free_bindmap("WFrame@WPaneWS", mod_panews_frame_bindmap);
        mod_panews_frame_bindmap=NULL;
    }
    
    if(panews_init_layout_alt!=NULL){
        destroy_obj((Obj*)panews_init_layout_alt);
        panews_init_layout_alt=NULL;
    }

    if(panews_make_placement_alt!=NULL){
        destroy_obj((Obj*)panews_make_placement_alt);
        panews_make_placement_alt=NULL;
    }
}


static bool register_regions()
{
    if(!ioncore_register_regclass(&CLASSDESCR(WPaneWS),
                                  (WRegionLoadCreateFn*)panews_load)){
        return FALSE;
    }
    
    return TRUE;
}


#define INIT_HOOK_(NM)                            \
    NM=ioncore_register_hook(#NM, create_hook()); \
    if(NM==NULL) return FALSE;


static bool init_hooks()
{
    INIT_HOOK_(panews_init_layout_alt);
    INIT_HOOK_(panews_make_placement_alt);
    return TRUE;
}



bool mod_panews_init()
{
    if(!init_hooks())
        goto err;

    mod_panews_panews_bindmap=ioncore_alloc_bindmap("WPaneWS", NULL);
    mod_panews_unusedwin_bindmap=ioncore_alloc_bindmap_frame("WUnusedWin");
    mod_panews_frame_bindmap=ioncore_alloc_bindmap_frame("WFrame@WPaneWS");

    if(mod_panews_panews_bindmap==NULL ||
       mod_panews_unusedwin_bindmap==NULL ||
       mod_panews_frame_bindmap==NULL){
        goto err;
    }

    if(!mod_panews_register_exports())
        goto err;
    
    if(!register_regions())
        goto err;
    
    /*ioncore_read_config("cfg_panews", NULL, FALSE);*/

    return TRUE;
    
err:
    mod_panews_deinit();
    return FALSE;
}


/*}}}*/

