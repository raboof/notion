/*
 * ion/menu/main.c
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
#include <ioncore/saveload.h>

#include "menu.h"


/*{{{ Module information */


#include "../version.h"

char mod_menu_ion_api_version[]=ION_API_VERSION;


/*}}}*/


/*{{{ Bindmaps w/ config */


WBindmap mod_menu_menu_bindmap=BINDMAP_INIT;


EXTL_EXPORT_AS(global, __defbindings_WMenu)
bool mod_menu_defbindings_WMenu(ExtlTab tab)
{
    return bindmap_do_table(&mod_menu_menu_bindmap, NULL, tab);
}


/*}}}*/


/*{{{ Init & deinit */


extern bool mod_menu_register_exports();
extern void mod_menu_unregister_exports();


void mod_menu_deinit()
{
    mod_menu_unregister_exports();
    bindmap_deinit(&mod_menu_menu_bindmap);
}


bool mod_menu_init()
{
    if(!mod_menu_register_exports()){
        mod_menu_deinit();
        return FALSE;
    }
    
    ioncore_read_config("menu", NULL, TRUE);
    
    return TRUE;
}


/*}}}*/

