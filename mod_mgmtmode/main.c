/*
 * ion/mod_mgmtmode/main.c
 *
 * Copyright (c) Tuomo Valkonen 2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libextl/readconfig.h>
#include <ioncore/saveload.h>
#include <ioncore/bindmaps.h>


/*{{{ Module information */


#include "../version.h"

char mod_mgmtmode_ion_api_version[]=ION_API_VERSION;


/*}}}*/


/*{{{ Bindmaps */


WBindmap *mod_mgmtmode_bindmap=NULL;


/*}}}*/


/*{{{ Init & deinit */


extern bool mod_mgmtmode_register_exports();
extern void mod_mgmtmode_unregister_exports();


void mod_mgmtmode_deinit()
{
    if(mod_mgmtmode_bindmap!=NULL){
        ioncore_free_bindmap("WMgmtMode", mod_mgmtmode_bindmap);
        mod_mgmtmode_bindmap=NULL;
    }

    mod_mgmtmode_unregister_exports();
}


bool mod_mgmtmode_init()
{
    mod_mgmtmode_bindmap=ioncore_alloc_bindmap("WMgmtMode", NULL);
    
    if(mod_mgmtmode_bindmap==NULL)
        return FALSE;

    if(!mod_mgmtmode_register_exports()){
        mod_mgmtmode_deinit();
        return FALSE;
    }

    extl_read_config("cfg_mgmtmode", NULL, TRUE);
    
    return TRUE;
}


/*}}}*/

