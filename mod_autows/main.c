/*
 * ion/autows/main.c
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
#include "autows.h"
#include "autoframe.h"
#include "placement.h"


/*{{{ Module information */


#include "../version.h"

char mod_autows_ion_api_version[]=ION_API_VERSION;


/*}}}*/


/*{{{ Bindmaps */


WBindmap *mod_autows_autows_bindmap=NULL;
WBindmap *mod_autows_autoframe_bindmap=NULL;


static StringIntMap frame_areas[]={
    {"border",      FRAME_AREA_BORDER},
    {"tab",         FRAME_AREA_TAB},
    {"empty_tab",   FRAME_AREA_TAB},
    {"client",      FRAME_AREA_CLIENT},
    END_STRINGINTMAP
};


/*}}}*/


/*{{{ Module init & deinit */


extern bool mod_autows_register_exports();
extern void mod_autows_unregister_exports();


void mod_autows_deinit()
{
    mod_autows_unregister_exports();
    ioncore_unregister_regclass(&CLASSDESCR(WAutoWS));
    ioncore_unregister_regclass(&CLASSDESCR(WAutoFrame));
    
    if(mod_autows_autows_bindmap!=NULL){
        ioncore_free_bindmap("WAutoWS", mod_autows_autows_bindmap);
        mod_autows_autows_bindmap=NULL;
    }
    
    if(mod_autows_autoframe_bindmap!=NULL){
        ioncore_free_bindmap("WAutoFrame", mod_autows_autoframe_bindmap);
        mod_autows_autoframe_bindmap=NULL;
    }
    
    if(autows_init_layout_alt!=NULL){
        destroy_obj((Obj*)autows_init_layout_alt);
        autows_init_layout_alt=NULL;
    }

}


static bool register_regions()
{
    if(!ioncore_register_regclass(&CLASSDESCR(WAutoWS),
                                  (WRegionSimpleCreateFn*)create_autows,
                                  (WRegionLoadCreateFn*)autows_load)){
        return FALSE;
    }
    if(!ioncore_register_regclass(&CLASSDESCR(WAutoFrame),
                                  (WRegionSimpleCreateFn*)create_autoframe,
                                  (WRegionLoadCreateFn*)autoframe_load)){
        return FALSE;
    }
    
    return TRUE;
}


#define INIT_HOOK_(NM)                            \
    NM=ioncore_register_hook(#NM, create_hook()); \
    if(NM==NULL) return FALSE;


static bool init_hooks()
{
    INIT_HOOK_(autows_init_layout_alt);
    return TRUE;
}



bool mod_autows_init()
{
    if(!init_hooks()){
        WARN_FUNC("failed to initialise hooks");
        goto err;
    }

    mod_autows_autows_bindmap=ioncore_alloc_bindmap("WAutoWS", NULL);
    
    mod_autows_autoframe_bindmap=ioncore_alloc_bindmap("WAutoFrame", 
                                                       frame_areas);

    if(mod_autows_autows_bindmap==NULL ||
       mod_autows_autoframe_bindmap==NULL){
        warn_obj("mod_autows", "failed to allocate bindmaps.");
        goto err;
    }

    if(!mod_autows_register_exports()){
        warn_obj("mod_autows", "failed to register functions.");
        goto err;
    }

    if(!mod_autows_register_exports()){
        warn_obj("mod_autows", "Unable to register exports");
        goto err;
    }
    
    if(!register_regions()){
        warn_obj("mod_autows", "Unable to register classes");
        goto err;
    }
    
    ioncore_read_config("templates", NULL, TRUE);
    ioncore_read_config("autows", NULL, FALSE);

    return TRUE;
    
err:
    mod_autows_deinit();
    return FALSE;
}


/*}}}*/

