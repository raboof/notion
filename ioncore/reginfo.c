/*
 * ion/ioncore/reginfo.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>
#include <ctype.h>

#include "common.h"
#include "region.h"
#include <libtu/objp.h>
#include "attach.h"
#include "reginfo.h"


static WRegClassInfo *reg_class_infos;


/*{{{ Registration */


bool ioncore_register_regclass(ClassDescr *descr, WRegionSimpleCreateFn *sc_fn,
                               WRegionLoadCreateFn *lc_fn)
{
    WRegClassInfo *info;
    
    if(descr==NULL)
        return FALSE;
    
    info=ALLOC(WRegClassInfo);
    if(info==NULL){
        warn_err();
        return FALSE;
    }
    
    info->descr=descr;
    info->sc_fn=sc_fn;
    info->lc_fn=lc_fn;
    LINK_ITEM(reg_class_infos, info, next, prev);
    
    return TRUE;
}


void ioncore_unregister_regclass(ClassDescr *descr)
{
    WRegClassInfo *info;
    
    for(info=reg_class_infos; info!=NULL; info=info->next){
        if(descr==info->descr){
            UNLINK_ITEM(reg_class_infos, info, next, prev);
            free(info);
            return;
        }
    }
}


/*}}}*/


/*{{{ Lookup */


WRegClassInfo *ioncore_lookup_regclass(const char *name, 
                                       bool inheriting_ok,
                                       bool need_simplefn,
                                       bool need_loadfn)
{
    WRegClassInfo *info;
    ClassDescr *descr;

    if(name==NULL)
        return NULL;
    
    for(info=reg_class_infos; info!=NULL; info=info->next){
        for(descr=info->descr; 
            descr!=NULL; 
            descr=(inheriting_ok ? descr->ancestor : NULL)){
            
            if(strcmp(descr->name, name)==0){
                if(need_simplefn && !info->sc_fn)
                    break;
                if(need_loadfn && !info->lc_fn)
                    break;
                return info;
            }
        }
    }
    return NULL;
}


/*}}}*/

