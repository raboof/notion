/*
 * ion/mod_statusbar/main.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libextl/readconfig.h>
#include <ioncore/saveload.h>
#include <ioncore/bindmaps.h>

#include "statusbar.h"


/*{{{ Module information */


#include "../version.h"

char mod_statusbar_ion_api_version[]=ION_API_VERSION;


/*}}}*/


/*{{{ Bindmaps */


WBindmap *mod_statusbar_statusbar_bindmap=NULL;


/*}}}*/


/*{{{ Init & deinit */


extern bool mod_statusbar_register_exports();
extern void mod_statusbar_unregister_exports();


void mod_statusbar_deinit()
{
    if(mod_statusbar_statusbar_bindmap!=NULL){
        ioncore_free_bindmap("WStatusBar", mod_statusbar_statusbar_bindmap);
        mod_statusbar_statusbar_bindmap=NULL;
    }

    ioncore_unregister_regclass(&CLASSDESCR(WStatusBar));

    mod_statusbar_unregister_exports();
}


bool mod_statusbar_init()
{
    mod_statusbar_statusbar_bindmap=ioncore_alloc_bindmap("WStatusBar", NULL);
    
    if(mod_statusbar_statusbar_bindmap==NULL)
        return FALSE;

    if(!ioncore_register_regclass(&CLASSDESCR(WStatusBar),
                                  (WRegionLoadCreateFn*) statusbar_load)){
        mod_statusbar_deinit();
        return FALSE;
    }

    if(!mod_statusbar_register_exports()){
        mod_statusbar_deinit();
        return FALSE;
    }
    
    /*ioncore_read_config("cfg_statusbar", NULL, TRUE);*/
    
    return TRUE;
}


/*}}}*/

