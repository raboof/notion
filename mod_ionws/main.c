/*
 * ion/mod_ionws/main.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
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
#include "ionws.h"
#include "placement.h"


/*{{{ Module information */


#include "../version.h"

char mod_ionws_ion_api_version[]=ION_API_VERSION;


/*}}}*/


/*{{{ Bindmaps and configuration variables */


WBindmap *mod_ionws_ionws_bindmap=NULL;
WBindmap *mod_ionws_frame_bindmap=NULL;

int mod_ionws_raise_delay=CF_RAISE_DELAY;


/*}}}*/


/*{{{ Configuration */


/*EXTL_DOC
 * Set parameters. Currently only \var{raise_delay} (in milliseconds)
 * is supported.
 */
EXTL_EXPORT
void mod_ionws_set(ExtlTab tab)
{
    int d;
    if(extl_table_gets_i(tab, "raise_delay", &d)){
        if(d>=0)
            mod_ionws_raise_delay=d;
    }
}


/*EXTL_DOC
 * Get parameters. For details see \fnref{mod_ionws.set}.
 */
EXTL_SAFE
EXTL_EXPORT
ExtlTab mod_ionws_get()
{
    ExtlTab tab=extl_create_table();
    
    extl_table_sets_i(tab, "raise_delay", mod_ionws_raise_delay);
    
    return tab;
}



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
        ioncore_free_bindmap("WFrame-on-WIonWS", mod_ionws_frame_bindmap);
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
    if(!init_hooks())
        goto err;
            
    mod_ionws_ionws_bindmap=ioncore_alloc_bindmap("WIonWS", NULL);
    
    mod_ionws_frame_bindmap=ioncore_alloc_bindmap_frame("WFrame-on-WIonWS");

    if(mod_ionws_ionws_bindmap==NULL || mod_ionws_frame_bindmap==NULL)
        goto err;

    if(!mod_ionws_register_exports())
        goto err;

    if(!register_regions())
        goto err;
    
    extl_read_config("cfg_ionws", NULL, TRUE);

    return TRUE;
    
err:
    mod_ionws_deinit();
    return FALSE;
}


/*}}}*/

