/*
 * ion/mod_ionws/main.c
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
#include <ioncore/readconfig.h>
#include <ioncore/framep.h>
#include <ioncore/bindmaps.h>
#include <ioncore/bindmaps.h>

#include "main.h"
#include "ionws.h"
#include "placement.h"


/*{{{ Module information */


#include "../version.h"

char mod_ionws_ion_api_version[]=ION_API_VERSION;


/*}}}*/


/*{{{ Bindmaps */


WBindmap *mod_ionws_ionws_bindmap=NULL;
WBindmap *mod_ionws_frame_bindmap=NULL;


/*}}}*/


/*{{{ Module init & deinit */


extern bool mod_ionws_register_exports();
extern void mod_ionws_unregister_exports();


void mod_ionws_deinit()
{
    mod_ionws_unregister_exports();
    ioncore_unregister_regclass(&CLASSDESCR(WIonWS));
    
    if(mod_ionws_ionws_bindmap!=NULL){
        ioncore_free_bindmap("WIonWS", mod_ionws_ionws_bindmap);
        mod_ionws_ionws_bindmap=NULL;
    }
    
    if(mod_ionws_frame_bindmap!=NULL){
        ioncore_free_bindmap("WFrame@WIonWS", mod_ionws_frame_bindmap);
        mod_ionws_frame_bindmap=NULL;
    }
    
    if(ionws_placement_alt!=NULL){
        destroy_obj((Obj*)ionws_placement_alt);
        ionws_placement_alt=NULL;
    }
}


static bool register_regions()
{
    if(!ioncore_register_regclass(&CLASSDESCR(WIonWS),
                                  (WRegionLoadCreateFn*)ionws_load)){
        return FALSE;
    }
    
    return TRUE;
}


#define INIT_HOOK_(NM)                            \
    NM=ioncore_register_hook(#NM, create_hook()); \
    if(NM==NULL) return FALSE;


static bool init_hooks()
{
    INIT_HOOK_(ionws_placement_alt);
    return TRUE;
}


bool mod_ionws_init()
{
    if(!init_hooks()){
        WARN_FUNC("failed to initialise hooks");
        goto err;
    }
            
    mod_ionws_ionws_bindmap=ioncore_alloc_bindmap("WIonWS", NULL);
    
    mod_ionws_frame_bindmap=ioncore_alloc_bindmap_frame("WFrame-on-WIonWS");

    if(mod_ionws_ionws_bindmap==NULL ||
       mod_ionws_frame_bindmap==NULL){
        WARN_FUNC("failed to allocate bindmaps.");
        goto err;
    }

    if(!mod_ionws_register_exports()){
        WARN_FUNC("failed to register functions.");
        goto err;
    }

    if(!mod_ionws_register_exports()){
        WARN_FUNC("Unable to register exports");
        goto err;
    }
    
    if(!register_regions()){
        WARN_FUNC("Unable to register classes");
        goto err;
    }
    
    ioncore_read_config("ionws", NULL, TRUE);

    return TRUE;
    
err:
    mod_ionws_deinit();
    return FALSE;
}


/*}}}*/

