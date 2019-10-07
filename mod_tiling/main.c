/*
 * ion/mod_tiling/main.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#include <libtu/map.h>

#include <ioncore/common.h>
#include <ioncore/reginfo.h>
#include <libextl/readconfig.h>
#include <ioncore/framep.h>
#include <ioncore/bindmaps.h>
#include <ioncore/bindmaps.h>

#include "main.h"
#include "tiling.h"
#include "placement.h"
#include "exports.h"


/*{{{ Module information */


#include "../version.h"

char mod_tiling_ion_api_version[]=NOTION_API_VERSION;


/*}}}*/


/*{{{ Bindmaps and configuration variables */


WBindmap *mod_tiling_tiling_bindmap=NULL;

int mod_tiling_raise_delay=CF_RAISE_DELAY;


/*}}}*/


/*{{{ Configuration */


/*EXTL_DOC
 * Set parameters. Currently only \var{raise_delay} (in milliseconds)
 * is supported.
 */
EXTL_EXPORT
void mod_tiling_set(ExtlTab tab)
{
    int d;
    if(extl_table_gets_i(tab, "raise_delay", &d)){
        if(d>=0)
            mod_tiling_raise_delay=d;
    }
}


/*EXTL_DOC
 * Get parameters. For details see \fnref{mod_tiling.set}.
 */
EXTL_SAFE
EXTL_EXPORT
ExtlTab mod_tiling_get()
{
    ExtlTab tab=extl_create_table();

    extl_table_sets_i(tab, "raise_delay", mod_tiling_raise_delay);

    return tab;
}



/*}}}*/



/*{{{ Module init & deinit */


void mod_tiling_deinit()
{
    mod_tiling_unregister_exports();
    ioncore_unregister_regclass(&CLASSDESCR(WTiling));

    if(mod_tiling_tiling_bindmap!=NULL){
        ioncore_free_bindmap("WTiling", mod_tiling_tiling_bindmap);
        mod_tiling_tiling_bindmap=NULL;
    }

    if(tiling_placement_alt!=NULL){
        destroy_obj((Obj*)tiling_placement_alt);
        tiling_placement_alt=NULL;
    }
}


static bool register_regions()
{
    if(!ioncore_register_regclass(&CLASSDESCR(WTiling),
                                  (WRegionLoadCreateFn*)tiling_load)){
        return FALSE;
    }

    return TRUE;
}


#define INIT_HOOK_(NM)                             \
    NM=mainloop_register_hook(#NM, create_hook()); \
    if(NM==NULL) return FALSE; \
    ((void)0)


static bool init_hooks()
{
    INIT_HOOK_(tiling_placement_alt);
    return TRUE;
}


bool mod_tiling_init()
{
    if(!init_hooks())
        goto err;

    mod_tiling_tiling_bindmap=ioncore_alloc_bindmap("WTiling", NULL);

    if(mod_tiling_tiling_bindmap==NULL)
        goto err;

    if(!mod_tiling_register_exports())
        goto err;

    if(!register_regions())
        goto err;

    extl_read_config("cfg_tiling", NULL, TRUE);

    return TRUE;

err:
    mod_tiling_deinit();
    return FALSE;
}


/*}}}*/
