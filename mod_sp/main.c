/*
 * ion/mod_sp/main.c
 *
 * Copyright (c) Tuomo Valkonen 2004-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/map.h>
#include <ioncore/conf-bindings.h>
#include <libextl/readconfig.h>
#include <ioncore/saveload.h>
#include <ioncore/screen.h>
#include <ioncore/hooks.h>
#include <ioncore/mplex.h>
#include <ioncore/ioncore.h>
#include <ioncore/global.h>
#include <ioncore/framep.h>
#include <ioncore/bindmaps.h>

#include "main.h"
#include "scratchpad.h"


/*{{{ Module information */


#include "../version.h"

char mod_sp_ion_api_version[]=ION_API_VERSION;


/*}}}*/


/*{{{ Bindmaps w/ config */


WBindmap *mod_sp_scratchpad_bindmap=NULL;


static StringIntMap frame_areas[]={
    {"border",     FRAME_AREA_BORDER},
    {"tab",        FRAME_AREA_TAB},
    {"empty_tab",  FRAME_AREA_TAB},
    {"client",     FRAME_AREA_CLIENT},
    END_STRINGINTMAP
};


/*}}}*/


/*{{{ Exports */


static bool do_toggle(WMPlex *mplex, WScratchpad *sp)
{
    if(mplex_l2_hidden(mplex, (WRegion*)sp))
        return mplex_l2_show(mplex, (WRegion*)sp);
    else
        return mplex_l2_hide(mplex, (WRegion*)sp);
}


/*EXTL_DOC
 * Toggle displayed status of some scratchpad on \var{mplex} if one is 
 * found.
 */
EXTL_EXPORT
bool mod_sp_toggle_on(WMPlex *mplex)
{
    int i;
    
    for(i=mplex_lcount(mplex, 2)-1; i>=0; i--){
        WScratchpad *sp=OBJ_CAST(mplex_lnth(mplex, 2, i), WScratchpad);
        if(sp!=NULL)
            return do_toggle(mplex, sp);
    }
   
    return FALSE;
}


/*EXTL_DOC
 * Toggle displayed status of \var{sp}.
 */
EXTL_EXPORT
bool mod_sp_toggle(WScratchpad *sp)
{
    if(sp!=NULL){
        WMPlex *mplex=OBJ_CAST(REGION_MANAGER(sp), WMPlex);
        if(mplex!=NULL /*&& mplex_layer(mplex, (WRegion*)sp)==2*/)
            return do_toggle(mplex, sp);
    }
    
    return FALSE;
}


/*}}}*/


/*{{{ Init & deinit */


extern bool mod_sp_register_exports();
extern void mod_sp_unregister_exports();


void mod_sp_deinit()
{
    if(mod_sp_scratchpad_bindmap!=NULL){
        ioncore_free_bindmap("WScratchpad", mod_sp_scratchpad_bindmap);
        mod_sp_scratchpad_bindmap=NULL;
    }
    mod_sp_unregister_exports();
    ioncore_unregister_regclass(&CLASSDESCR(WScratchpad));
}


static WScratchpad *create(WScreen *scr)
{
    return (WScratchpad*)mplex_attach_hnd((WMPlex*)scr,
                                          (WRegionAttachHandler*)
                                          create_scratchpad, NULL,
                                          MPLEX_ATTACH_L2);
}


static void check_and_create()
{
    WScreen *scr;
    int i;
    
    /* No longer needed, free the memory the list uses. */
    hook_remove(ioncore_post_layout_setup_hook, check_and_create);
    
    FOR_ALL_SCREENS(scr){
        WScratchpad *sp=NULL;
        /* This is really inefficient, but most likely there's just
         * the scratchpad on the list... 
         */
        for(i=0; i<mplex_lcount((WMPlex*)scr, 2); i++){
            sp=OBJ_CAST(mplex_lnth((WMPlex*)scr, 2, i), WScratchpad);
            if(sp!=NULL)
                break;
        }
        
        if(sp==NULL){
            sp=create(scr);
            if(sp==NULL){
                warn(TR("Unable to create scratchpad for screen %d."),
                     screen_id(scr));
            }
        }
    }
}
    

bool mod_sp_init()
{
    if(!mod_sp_register_exports())
        return FALSE;

    mod_sp_scratchpad_bindmap=ioncore_alloc_bindmap("WScratchpad", NULL);
    
    if(mod_sp_scratchpad_bindmap==NULL){
        mod_sp_deinit();
        return FALSE;
    }
    
    if(!ioncore_register_regclass(&CLASSDESCR(WScratchpad),
                                  (WRegionLoadCreateFn*)scratchpad_load)){
        mod_sp_deinit();
        return FALSE;
    }
    
    extl_read_config("cfg_sp", NULL, FALSE);
    
    if(ioncore_g.opmode==IONCORE_OPMODE_INIT){
        hook_add(ioncore_post_layout_setup_hook, check_and_create);
    }else{
        check_and_create();
    }
    
    return TRUE;
}


/*}}}*/

