/*
 * ion/ioncore/saveload.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>
#include <time.h>
#include <unistd.h>

#include <libtu/objp.h>

#include "common.h"
#include "global.h"
#include "region.h"
#include "readconfig.h"
#include "screen.h"
#include "saveload.h"
#include "names.h"
#include "attach.h"
#include "reginfo.h"
#include "extl.h"
#include "extlconv.h"


static bool loading_layout=FALSE;
static bool layout_load_error=FALSE;


/*{{{ Load support functions */



WRegion *create_region_load(WWindow *par, const WFitParams *fp, 
                            ExtlTab tab)
{
    char *objclass=NULL, *name=NULL;
    WRegionLoadCreateFn* fn=NULL;
    WRegClassInfo *info=NULL;
    WRegion *reg=NULL;
    
    if(!extl_table_gets_s(tab, "type", &objclass))
        return NULL;

    info=ioncore_lookup_regclass(objclass, FALSE);
    if(info!=NULL)
        fn=info->lc_fn;
    
    if(fn==NULL){
        warn("Unknown class \"%s\", cannot create region.", objclass);
        layout_load_error=loading_layout;
        return NULL;
    }

    free(objclass);
    
    reg=fn(par, fp, tab);
    
    if(reg==NULL)
        return NULL;
    
    if(!OBJ_IS(reg, WClientWin)){
        if(extl_table_gets_s(tab, "name", &name)){
            region_set_name(reg, name);
            free(name);
        }
    }
    
    return reg;
}

    
/*}}}*/


/*{{{ Save support functions */


bool region_supports_save(WRegion *reg)
{
    return HAS_DYN(reg, region_get_configuration);
}


ExtlTab region_get_base_configuration(WRegion *reg)
{
    const char *name;
    ExtlTab tab;
    
    tab=extl_create_table();
    
    extl_table_sets_s(tab, "type", OBJ_TYPESTR(reg));
    
    name=region_name(reg);
    
    if(name!=NULL && !OBJ_IS(reg, WClientWin))
        extl_table_sets_s(tab, "name", name);
    
    return tab;
}


ExtlTab region_get_configuration(WRegion *reg)
{
    ExtlTab tab=extl_table_none();
    CALL_DYN_RET(tab, ExtlTab, region_get_configuration, reg, (reg));
    return tab;
}


/*}}}*/


/*{{{ save_workspaces, load_workspaces */


#define BACKUP_MSG                                                            \
"There were errors loading layout. Backing up current layout savefile as\n"   \
"%s.\n"                                                                       \
"If you are _not_ running under a session manager and wish to restore your\n" \
"old layout, copy this backup file over the layout savefile found in the\n"   \
"same directory while Ion is not running and after having fixed your other\n" \
"configuration files that are causing this problem. (Maybe a missing\n"       \
"ioncore.load_module call?)."


bool ioncore_init_layout()
{
    ExtlTab tab;
    WScreen *scr;
    bool ok;
    bool n=0;
    
    ok=ioncore_read_savefile("layout", &tab);
    
    loading_layout=TRUE;
    layout_load_error=FALSE;
    
    FOR_ALL_SCREENS(scr){
        ExtlTab scrtab=extl_table_none();
        bool scrok=FALSE;
        if(ok)
            scrok=extl_table_geti_t(tab, screen_id(scr), &scrtab);
        
        n+=(TRUE==screen_init_layout(scr, scrtab));
        
        if(scrok)
            extl_unref_table(scrtab);
    }

    loading_layout=FALSE;

    if(layout_load_error){
        time_t t=time(NULL);
        char tm[]="layout.backup-YYYYMMDDHHMMSS\0\0\0\0";
        char *backup;
        
        strftime(tm+14, 15, "%Y%m%d%H%M%S", localtime(&t));
        backup=ioncore_get_savefile(tm);
        if(backup==NULL){
            warn("Unable to get file for layout backup.");
            return FALSE;
        }
        if(access(backup, F_OK)==0){
            warn("Backup file %s already exists.", backup);
            free(backup);
            return FALSE;
        }
        warn(BACKUP_MSG, backup);
        if(!extl_serialise(backup, tab))
            warn("Failed backup.");
        free(backup);
    }
        
    return (n!=0);
}


bool ioncore_save_layout()
{
    WScreen *scr=NULL;
    ExtlTab tab=extl_create_table();
    bool ret;
    
    if(tab==extl_table_none())
        return FALSE;
    
    FOR_ALL_SCREENS(scr){
        ExtlTab scrtab=region_get_configuration((WRegion*)scr);
        if(scrtab==extl_table_none()){
            warn("Unable to get configuration for screen %d.",
                 screen_id(scr));
        }else{
            extl_table_seti_t(tab, screen_id(scr), scrtab);
            extl_unref_table(scrtab);
        }
    }
    
    ret=ioncore_write_savefile("layout", tab);
    
    extl_unref_table(tab);
    
    if(!ret)
        warn("Unable to save layout.");
        
    return ret;
}


/*}}}*/


