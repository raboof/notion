/*
 * ion/ioncore/modules.c
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
#include <dlfcn.h>
#include <unistd.h>

#include <libtu/rb.h>

#include "common.h"
#include "modules.h"
#include "readconfig.h"
#include "../version.h"


#ifndef CF_PRELOAD_MODULES


typedef void *dlhandle;


/*{{{ Module list */


static Rb_node modules=NULL;


static dlhandle get_handle(const char *name)
{
    int found=0;
    Rb_node nd;
    
    nd=rb_find_key_n(modules, name, &found);
    if(found)
        return nd->v.val;
    return NULL;
}


static const char *get_name(dlhandle handle)
{
    Rb_node nd;
    
    rb_traverse(nd, modules){
        if(nd->v.val==handle)
            return (const char *)(nd->k.key);
    }
    return NULL;
}


static Rb_node add_module(char *name, dlhandle handle)
{
    return rb_insert(modules, name, handle);
}


/*}}}*/


/*{{{ Module symbol access */


static void *get_module_symbol(dlhandle handle, 
                               const char *modulename, 
                               const char *name)
{
    char *p;
    void *ret;

    p=scat(modulename, name);
    
    if(p==NULL){
        warn_err();
        return NULL;
    }
    
    ret=dlsym(handle, p);
    
    free(p);
    
    return ret;
}

static bool check_version(dlhandle handle, const char *modulename)
{
    char *versionstr=(char*)get_module_symbol(handle, modulename, 
                                              "_ion_api_version");
    if(versionstr==NULL)
        return FALSE;
    return (strcmp(versionstr, ION_API_VERSION)==0);
}


static bool call_init(dlhandle handle, const char *modulename)
{
    bool (*initfn)(void);
    
    initfn=(bool (*)())get_module_symbol(handle, modulename, "_init");
    
    if(initfn==NULL)
        return TRUE;
    
    return initfn();
}


static void call_deinit(dlhandle handle, const char *modulename)
{
    void (*deinitfn)(void);
    
    deinitfn=(void (*)())get_module_symbol(handle, modulename, "_deinit");
    
    if(deinitfn!=NULL)
        deinitfn();
}


/*}}}*/


/*{{{ Init */


bool ioncore_init_module_support()
{
    modules=make_rb();
    if(modules==NULL){
        warn_err();
        return FALSE;
    }

    return TRUE;
}


static int try_load(const char *file, void *param)
{
    dlhandle handle=NULL;
    const char *slash, *dot;
    char *name;
    Rb_node mod;
    
    if(access(file, F_OK)!=0)
        return IONCORE_TRYCONFIG_NOTFOUND;

    slash=strrchr(file, '/');
    dot=strrchr(file, '.');
    
    if(slash==NULL)
        slash=file;
    else
        slash++;
    
    if(dot<=slash){
        warn("Invalid module name.");
        goto err1;
    }
    
    name=ALLOC_N(char, dot-slash);
    if(name==NULL){
        warn_err();
        goto err1;
    }
    
    strncpy(name, slash, dot-slash);
    name[dot-slash]='\0';
    
    if(get_handle(name)){
        warn_obj(file, "Module with this name already loaded.");
        goto err2;
    }
        
    handle=dlopen(file, RTLD_NOW|RTLD_GLOBAL);

    if(handle==NULL){
        warn_obj(file, "%s", dlerror());
        goto err2;
    }
    
    if(get_name(handle))
        return IONCORE_TRYCONFIG_OK;
    
    if(!check_version(handle, name)){
        warn_obj(file, "Module version information not found or version "
                 "mismatch. Refusing to use.");
        goto err3;
    }
    
    mod=add_module(name, handle);
    
    if(mod==NULL){
        warn_err();
        goto err3;
    }
    
    if(!call_init(handle, name)){
        warn_obj(file, "Unable to initialise module.");
        rb_delete_node(mod);
        goto err3;
    }
    
    return IONCORE_TRYCONFIG_OK;

err3:    
    dlclose(handle);
err2:
    free(name);
err1:
    return IONCORE_TRYCONFIG_LOAD_FAILED;
}


static bool do_load_module(const char *modname)
{
    int retval;
    
    retval=ioncore_try_config(modname, NULL, (WTryConfigFn*)try_load, NULL,
                              "so", NULL);
    
    if(retval==IONCORE_TRYCONFIG_NOTFOUND)
        warn("Unable to find '%s' on search path.", modname);
    
    return (retval==IONCORE_TRYCONFIG_OK);
}


/*}}}*/


/*{{{ Deinit */


static void do_unload_module(Rb_node mod)
{
    char *name=(char*)mod->k.key;
    dlhandle handle=mod->v.val;
    
    call_deinit(handle, name);

    dlclose(handle);
    free(name);
}


void ioncore_unload_modules()
{
    Rb_node mod;
    
    rb_traverse(mod, modules){
        do_unload_module(mod);
    }
}


/*}}}*/


#else


/*{{{ Static module support */


static bool call_init(WStaticModuleInfo *handle)
{
    if(handle->init!=NULL)
        return handle->init();
    return TRUE;
}


static void call_deinit(WStaticModuleInfo *handle)
{
    if(handle->deinit!=NULL)
        handle->deinit();
}


extern WStaticModuleInfo ioncore_static_modules[];


static bool do_load_module(const char *name)
{
    WStaticModuleInfo *mod;
    
    for(mod=ioncore_static_modules; mod->name!=NULL; mod++){
        if(strcmp(mod->name, name)==0)
            break;
    }
    
    if(mod->name==NULL){
        warn_obj(name, "Unknown module.");
        return FALSE;
    }
    
    if(mod->loaded)
        return TRUE;

    if(!call_init(mod)){
        warn_obj(name, "Unable to initialise module.");
        return FALSE;
    }
    
    mod->loaded=TRUE;
    
    return TRUE;
}


void ioncore_unload_modules()
{
    WStaticModuleInfo *mod;
        
    for(mod=ioncore_static_modules; mod->name!=NULL; mod++){
        if(mod->loaded){
            call_deinit(mod);
            mod->loaded=FALSE;
        }
    }
}


bool ioncore_init_module_support()
{
    return TRUE;
}


/*}}}*/


#endif


/*{{{ Exports */


/*EXTL_DOC
 * Attempt to load module \var{modname}. Ion will use libltdl to search
 * the library path (the default setting is \file{\~{}/.ion3/libs} and
 * \file{\$PREFIX/lib/ion}) and also try diffent extensions, so only
 * the module name should usually be necessary to give here.
 */
EXTL_EXPORT
bool ioncore_load_module(const char *modname)
{
    if(modname==NULL){
        warn("No module to load given.");
        return FALSE;
    }
    return do_load_module(modname);
}


/*}}}*/

