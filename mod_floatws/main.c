/*
 * ion/mod_floatws/main.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <ioncore/bindmaps.h>
#include <ioncore/readconfig.h>
#include <ioncore/framep.h>
#include <ioncore/frame-pointer.h>
#include <ioncore/reginfo.h>
#include <ioncore/hooks.h>
#include <ioncore/clientwin.h>
#include <ioncore/extl.h>

#include "floatws.h"
#include "floatframe.h"


/*{{{ Module information */


#include "../version.h"

char mod_floatws_ion_api_version[]=ION_API_VERSION;


/*}}}*/


/*{{{ Bindmaps */


WBindmap *mod_floatws_floatws_bindmap=NULL;
WBindmap *mod_floatws_floatframe_bindmap=NULL;


static StringIntMap frame_areas[]={
    {"border",     FRAME_AREA_BORDER},
    {"tab",        FRAME_AREA_TAB},
    {"empty_tab",  FRAME_AREA_TAB},
    {"client",     FRAME_AREA_CLIENT},
    END_STRINGINTMAP
};


/*}}}*/


/*{{{ Init & deinit */


extern bool mod_floatws_register_exports();
extern bool mod_floatws_unregister_exports();


void mod_floatws_deinit()
{
    hook_remove(clientwin_do_manage_alt, 
                (WHookDummy*)mod_floatws_clientwin_do_manage);

    if(mod_floatws_floatws_bindmap!=NULL){
        ioncore_free_bindmap("WFloatWS", mod_floatws_floatws_bindmap);
        mod_floatws_floatws_bindmap=NULL;
    }
    
    if(mod_floatws_floatframe_bindmap!=NULL){
        ioncore_free_bindmap("WFloatFrame", mod_floatws_floatframe_bindmap);
        mod_floatws_floatframe_bindmap=NULL;
    }
    
    ioncore_unregister_regclass(&CLASSDESCR(WFloatWS));
    ioncore_unregister_regclass(&CLASSDESCR(WFloatFrame));
    
    mod_floatws_unregister_exports();
}



bool mod_floatws_init()
{
    mod_floatws_floatws_bindmap=ioncore_alloc_bindmap("WFloatWS", NULL);
       
    mod_floatws_floatframe_bindmap=ioncore_alloc_bindmap("WFloatFrame", 
                                                         frame_areas);

    if(mod_floatws_floatws_bindmap==NULL ||
       mod_floatws_floatframe_bindmap==NULL){
        WARN_FUNC(TR("Failed to allocate bindmaps."));
        goto err;
    }

    if(!mod_floatws_register_exports()){
        WARN_FUNC(TR("Failed to register functions."));
        goto err;
    }
    
    if(!ioncore_register_regclass(&CLASSDESCR(WFloatWS),
                                  (WRegionLoadCreateFn*) floatws_load) ||
       !ioncore_register_regclass(&CLASSDESCR(WFloatFrame), 
                                  (WRegionLoadCreateFn*) floatframe_load)){
        WARN_FUNC(TR("Failed to register classes."));
        goto err;
    }

    ioncore_read_config("cfg_floatws", NULL, TRUE);
    
    hook_add(clientwin_do_manage_alt, 
             (WHookDummy*)mod_floatws_clientwin_do_manage);

    return TRUE;
    
err:
    mod_floatws_deinit();
    return FALSE;
}


/*}}}*/

