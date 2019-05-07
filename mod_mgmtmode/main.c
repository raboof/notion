/*
 * ion/mod_mgmtmode/main.c
 *
 * Copyright (c) Tuomo Valkonen 2004-2007.
 *
 * See the included file LICENSE for details.
 */

#include <libextl/readconfig.h>
#include <ioncore/saveload.h>
#include <ioncore/bindmaps.h>

#include "exports.h"

/*{{{ Module information */


#include "../version.h"

char mod_mgmtmode_ion_api_version[]=ION_API_VERSION;


/*}}}*/


/*{{{ Bindmaps */


WBindmap *mod_mgmtmode_bindmap=NULL;


/*}}}*/


/*{{{ Init & deinit */


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

