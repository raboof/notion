/*
 * ion/ioncore/saveload.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
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
#include <libextl/readconfig.h>
#include <libextl/extl.h>

#include "common.h"
#include "global.h"
#include "region.h"
#include "screen.h"
#include "saveload.h"
#include "names.h"
#include "attach.h"
#include "reginfo.h"
#include "extlconv.h"
#include "group-ws.h"


static bool loading_layout=FALSE;
static bool layout_load_error=FALSE;


/*{{{ Session management module support */


static SMAddCallback *add_cb;
static SMCfgCallback *cfg_cb;
static SMPHolderCallback *ph_cb;
static bool clientwin_was_missing=FALSE;


void ioncore_set_sm_callbacks(SMAddCallback *add, SMCfgCallback *cfg)
{
    add_cb=add;
    cfg_cb=cfg;
}


void ioncore_get_sm_callbacks(SMAddCallback **add, SMCfgCallback **cfg)
{
    *add=add_cb;
    *cfg=cfg_cb;
}


void ioncore_set_sm_pholder_callback(SMPHolderCallback *phcb)
{
    ph_cb=phcb;
}


void ioncore_clientwin_load_missing()
{
    clientwin_was_missing=TRUE;
}


/*}}}*/


/*{{{ Load support functions */


WRegion *create_region_load(WWindow *par, const WFitParams *fp, 
                            ExtlTab tab)
{
    char *objclass=NULL, *name=NULL;
    WRegionLoadCreateFn* fn=NULL;
    WRegClassInfo *info=NULL;
    WRegion *reg=NULL;
    bool grouped=FALSE;
    char *grouped_name=NULL;
    
    if(!extl_table_gets_s(tab, "type", &objclass))
        return NULL;
    
    if(objclass==NULL)
        return NULL;
    
    info=ioncore_lookup_regclass(objclass, FALSE);
    if(info!=NULL)
        fn=info->lc_fn;
    
    if(fn==NULL){
        warn(TR("Unknown class \"%s\", cannot create region."),
             objclass);
        layout_load_error=loading_layout;
        return NULL;
    }

    free(objclass);
    
    clientwin_was_missing=FALSE;
    
    reg=fn(par, fp, tab);
    
    if(reg==NULL){
        if(clientwin_was_missing && add_cb!=NULL && ph_cb!=NULL){
            WPHolder *ph=ph_cb();
            if(ph!=NULL){
                if(!add_cb(ph, tab))
                    destroy_obj((Obj*)ph);
            }
        }
    }else{
        if(!OBJ_IS(reg, WClientWin)){
            if(extl_table_gets_s(tab, "name", &name)){
                region_set_name(reg, name);
                free(name);
            }
        }
    }
    
    ph_cb=NULL;
    
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


static bool get_config_clientwins=TRUE;


ExtlTab region_get_configuration(WRegion *reg)
{
    ExtlTab tab=extl_table_none();
    if(get_config_clientwins || !OBJ_IS(reg, WClientWin)){
        CALL_DYN_RET(tab, ExtlTab, region_get_configuration, reg, (reg));
    }
    return tab;
}


/*EXTL_DOC
 * Get configuration tree. If \var{clientwins} is unset, client windows
 * are filtered out.
 */
EXTL_EXPORT_AS(WRegion, get_configuration)
ExtlTab region_get_configuration_extl(WRegion *reg, bool clientwins)
{
    ExtlTab tab;
    
    get_config_clientwins=clientwins;
    
    tab=region_get_configuration(reg);
    
    get_config_clientwins=TRUE;
    
    return tab;
}


/*}}}*/


/*{{{ save_workspaces, load_workspaces */


static const char backup_msg[]=DUMMY_TR(
"There were errors loading layout. Backing up current layout savefile as\n"
"%s.\n"
"If you are _not_ running under a session manager and wish to restore your\n"
"old layout, copy this backup file over the layout savefile found in the\n"
"same directory while Ion is not running and after having fixed your other\n"
"configuration files that are causing this problem. (Maybe a missing\n"
"module?)");


bool ioncore_init_layout()
{
    ExtlTab tab;
    WScreen *scr;
    bool ok;
    int n=0;
    
    ok=extl_read_savefile("saved_layout", &tab);
    
    loading_layout=TRUE;
    layout_load_error=FALSE;
    
    FOR_ALL_SCREENS(scr){
        ExtlTab scrtab=extl_table_none();
        bool scrok=FALSE;
        
        /* Potential Xinerama or such support should set the screen ID
         * of the root window to less than zero, and number its own
         * fake screens up from 0.
         */
        if(screen_id(scr)<0)
            continue;

        if(ok)
            scrok=extl_table_geti_t(tab, screen_id(scr), &scrtab);
        
        n+=(TRUE==screen_init_layout(scr, scrtab));
        
        if(scrok)
            extl_unref_table(scrtab);
    }

    loading_layout=FALSE;

    if(layout_load_error){
        time_t t=time(NULL);
        char tm[]="saved_layout.backup-YYYYMMDDHHMMSS\0\0\0\0";
        char *backup;
        
        strftime(tm+20, 15, "%Y%m%d%H%M%S", localtime(&t));
        backup=extl_get_savefile(tm);
        if(backup==NULL){
            warn(TR("Unable to get file for layout backup."));
            return FALSE;
        }
        if(access(backup, F_OK)==0){
            warn(TR("Backup file %s already exists."), backup);
            free(backup);
            return FALSE;
        }
        warn(TR(backup_msg), backup);
        if(!extl_serialise(backup, tab))
            warn(TR("Failed backup."));
        free(backup);
    }
        
    if(n==0){
        warn(TR("Unable to initialise layout on any screen."));
        return FALSE;
    }else{
        return TRUE;
    }
}


bool ioncore_save_layout()
{
    WScreen *scr=NULL;
    ExtlTab tab=extl_create_table();
    bool ret;
    
    if(tab==extl_table_none())
        return FALSE;
    
    FOR_ALL_SCREENS(scr){
        ExtlTab scrtab;
        
        /* See note above */
        if(screen_id(scr)<0)
            continue;
            
        scrtab=region_get_configuration((WRegion*)scr);
        
        if(scrtab==extl_table_none()){
            warn(TR("Unable to get configuration for screen %d."),
                 screen_id(scr));
        }else{
            extl_table_seti_t(tab, screen_id(scr), scrtab);
            extl_unref_table(scrtab);
        }
    }
    
    ret=extl_write_savefile("saved_layout", tab);
    
    extl_unref_table(tab);
    
    if(!ret)
        warn(TR("Unable to save layout."));
        
    return ret;
}


/*}}}*/


