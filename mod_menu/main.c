/*
 * ion/mod_menu/main.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009.
 *
 * See the included file LICENSE for details.
 */

#include <libextl/readconfig.h>
#include <ioncore/saveload.h>
#include <ioncore/bindmaps.h>

#include "menu.h"
#include "exports.h"


/*{{{ Module information */


#include "../version.h"

char mod_menu_ion_api_version[]=NOTION_API_VERSION;


/*}}}*/


/*{{{ Bindmaps */


WBindmap *mod_menu_menu_bindmap=NULL;


/*}}}*/


/*{{{ Init & deinit */


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

