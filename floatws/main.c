/*
 * ion/floatws/main.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <ioncore/binding.h>
#include <ioncore/conf-bindings.h>
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

char floatwsmod_ion_api_version[]=ION_API_VERSION;


/*}}}*/


/*{{{ Bindmaps w/ config */


WBindmap floatws_bindmap=BINDMAP_INIT;
WBindmap floatframe_bindmap=BINDMAP_INIT;


static StringIntMap frame_areas[]={
    {"border",         FRAME_AREA_BORDER},
    {"tab",         FRAME_AREA_TAB},
    {"empty_tab",     FRAME_AREA_TAB},
    {"client",         FRAME_AREA_CLIENT},
    END_STRINGINTMAP
};


EXTL_EXPORT_AS(global, __defbindings_WFloatFrame)
bool floatwsmod_defbindings_WFloatFrame(ExtlTab tab)
{
    return bindmap_do_table(&floatframe_bindmap, frame_areas, tab);
}


EXTL_EXPORT_AS(global, __defbindings_WFloatWS)
void floatwsmod_defbindings_WFloatWS(ExtlTab tab)
{
    bindmap_do_table(&floatws_bindmap, NULL, tab);
}


/*}}}*/


/*{{{ Init & deinit */


extern bool floatwsmod_register_exports();
extern bool floatwsmod_unregister_exports();


void floatwsmod_deinit()
{
    REMOVE_HOOK(clientwin_do_manage_alt, 
                floatwsmod_clientwin_do_manage);

    floatwsmod_unregister_exports();
    bindmap_deinit(&floatws_bindmap);
    bindmap_deinit(&floatframe_bindmap);
    ioncore_unregister_regclass(&CLASSDESCR(WFloatWS));
    ioncore_unregister_regclass(&CLASSDESCR(WFloatFrame));
}



bool floatwsmod_init()
{
    if(!floatwsmod_register_exports()){
        warn_obj("floatws module", "failed to register functions.");
        goto err;
    }
    
    if(!ioncore_register_regclass(&CLASSDESCR(WFloatWS),
                              (WRegionSimpleCreateFn*) create_floatws,
                              (WRegionLoadCreateFn*) floatws_load) ||
       !ioncore_register_regclass(&CLASSDESCR(WFloatFrame), NULL,
                              (WRegionLoadCreateFn*) floatframe_load)){
        warn_obj("floatws module", "failed to register classes.");
        goto err;
    }

    ioncore_read_config("floatws", NULL, TRUE);
    
    ADD_HOOK(clientwin_do_manage_alt, floatwsmod_clientwin_do_manage);

    return TRUE;
    
err:
    floatwsmod_deinit();
    return FALSE;
}


/*}}}*/

