/*
 * ion/mod_sp/main.c
 *
 * Copyright (c) Tuomo Valkonen 2004-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <libtu/map.h>
#include <libtu/minmax.h>
#include <libextl/readconfig.h>
#include <libmainloop/hooks.h>

#include <ioncore/conf-bindings.h>
#include <ioncore/saveload.h>
#include <ioncore/screen.h>
#include <ioncore/mplex.h>
#include <ioncore/ioncore.h>
#include <ioncore/global.h>
#include <ioncore/framep.h>
#include <ioncore/frame.h>
#include <ioncore/regbind.h>
#include <ioncore/bindmaps.h>
#include <ioncore/names.h>

#include "main.h"
#include "exports.h"


/*{{{ Module information */

#include "../version.h"

char mod_sp_ion_api_version[]=ION_API_VERSION;


/*}}}*/


/*{{{ Bindmaps, config, etc. */


#define SP_NAME  "*scratchpad*"


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


static WRegion *create_frame_scratchpad(WWindow *parent, const WFitParams *fp,
                                        void *unused)
{
    return (WRegion*)create_frame(parent, fp, FRAME_MODE_UNKNOWN);
}


static WFrame *create(WMPlex *mplex, int flags)
{
    WFrame *sp;
    WMPlexAttachParams par;
    int sw=REGION_GEOM(mplex).w, sh=REGION_GEOM(mplex).h;

    par.flags=(flags
               |MPLEX_ATTACH_UNNUMBERED
               |MPLEX_ATTACH_MODAL
               |MPLEX_ATTACH_SIZEPOLICY
               |MPLEX_ATTACH_GEOM);
    par.szplcy=SIZEPOLICY_FREE_GLUE;
    
    par.geom.w=minof(sw, CF_SCRATCHPAD_DEFAULT_W);
    par.geom.h=minof(sh, CF_SCRATCHPAD_DEFAULT_H);
    par.geom.x=(sw-par.geom.w)/2;
    par.geom.y=(sh-par.geom.h)/2;

    sp=(WFrame*)mplex_do_attach_new((WMPlex*)mplex, &par,
                                    create_frame_scratchpad,
                                    NULL);
    

    if(sp==NULL){
        warn(TR("Unable to create scratchpad."));
    }
    
    region_add_bindmap((WRegion*)sp, mod_sp_scratchpad_bindmap);
    region_set_name((WRegion*)sp, SP_NAME);
    
    return sp;
}


static bool is_scratchpad(WRegion *reg)
{
    char *nm=reg->ni.name;
    int inst_off=reg->ni.inst_off;
    
    if(nm==NULL)
        return FALSE;
    
    if(inst_off<0)
        return (strcmp(nm, SP_NAME)==0);
    else
        return (strncmp(nm, SP_NAME, inst_off)==0);
}


/*EXTL_DOC
 * Change displayed status of some scratchpad on \var{mplex} if one is 
 * found. The parameter \var{how} is one of (set/unset/toggle).
 */
EXTL_EXPORT
bool mod_sp_set_shown_on(WMPlex *mplex, const char *how)
{
    int setpar=libtu_setparam_invert(libtu_string_to_setparam(how));
    WMPlexIterTmp tmp;
    WRegion *reg;
    bool found=FALSE;
    
    FOR_ALL_MANAGED_BY_MPLEX(mplex, reg, tmp){
        if(is_scratchpad(reg)){
            mplex_set_hidden(mplex, reg, setpar);
            found=TRUE;
        }
    }

    if(!found){
        int sp=libtu_string_to_setparam(how);
        if(sp==SETPARAM_SET || sp==SETPARAM_TOGGLE)
            found=(create(mplex, 0)!=NULL);
    }
    
    return found;
}


/*EXTL_DOC
 * Toggle displayed status of \var{sp}.
 * The parameter \var{how} is one of (set/unset/toggle).
 */
EXTL_EXPORT
bool mod_sp_set_shown(WFrame *sp, const char *how)
{
    if(sp!=NULL){
        int setpar=libtu_setparam_invert(libtu_string_to_setparam(how));
        WMPlex *mplex=OBJ_CAST(REGION_MANAGER(sp), WMPlex);
        if(mplex!=NULL)
            return mplex_set_hidden(mplex, (WRegion*)sp, setpar);
    }
    
    return FALSE;
}


/*}}}*/


/*{{{ Init & deinit */


void mod_sp_deinit()
{
    if(mod_sp_scratchpad_bindmap!=NULL){
        ioncore_free_bindmap("WFrame-as-scratchpad", mod_sp_scratchpad_bindmap);
        mod_sp_scratchpad_bindmap=NULL;
    }
    mod_sp_unregister_exports();
}


static void check_and_create()
{
    WMPlexIterTmp tmp;
    WScreen *scr;
    WRegion *reg;
    
    /* No longer needed, free the memory the list uses. */
    hook_remove(ioncore_post_layout_setup_hook, check_and_create);
    
    FOR_ALL_SCREENS(scr){
        FOR_ALL_MANAGED_BY_MPLEX((WMPlex*)scr, reg, tmp){
            if(is_scratchpad(reg))
                return;
        }
        
        create(&scr->mplex, MPLEX_ATTACH_HIDDEN);
    }
}
    

bool mod_sp_init()
{
    if(!mod_sp_register_exports())
        return FALSE;

    mod_sp_scratchpad_bindmap=ioncore_alloc_bindmap("WFrame-as-scratchpad", NULL);
    
    if(mod_sp_scratchpad_bindmap==NULL){
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

