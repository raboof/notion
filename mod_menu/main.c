/*
 * ion/mod_menu/main.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libextl/readconfig.h>
#include <ioncore/saveload.h>
#include <ioncore/bindmaps.h>

#include "menu.h"


/*{{{ Module information */


#include "../version.h"

char mod_menu_ion_api_version[]=ION_API_VERSION;


/*}}}*/


/*{{{ Bindmaps */


WBindmap *mod_menu_menu_bindmap=NULL;


/*}}}*/


/*{{{ Init & deinit */


extern bool mod_menu_register_exports();
extern void mod_menu_unregister_exports();


void mod_menu_deinit()
{
    if(mod_menu_menu_bindmap!=NULL){
        ioncore_free_bindmap("WMenu", mod_menu_menu_bindmap);
        mod_menu_menu_bindmap=NULL;
    }

    mod_menu_unregister_exports();
}


bool mod_menu_init()
{
    mod_menu_menu_bindmap=ioncore_alloc_bindmap("WMenu", NULL);
    
    if(mod_menu_menu_bindmap==NULL)
        return FALSE;

    if(!mod_menu_register_exports()){
        mod_menu_deinit();
        return FALSE;
    }
    
    /*ioncore_read_config("cfg_menu", NULL, TRUE);*/
    
    return TRUE;
}


/*}}}*/

