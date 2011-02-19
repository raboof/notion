/*
 * ion/ioncore/modules.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <ctype.h>
#include <dlfcn.h>
#include <unistd.h>

#include <libtu/rb.h>
#include <libextl/readconfig.h>

#include "common.h"
#include "modules.h"
#include "../version.h"


#ifndef CF_PRELOAD_MODULES


/*{{{ Module list */


typedef void *dlhandle;


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
    
    if(p==NULL)
        return NULL;
    
    ret=dlsym(handle, p);
    
    free(p);
    
    return ret;
}

static void (*get_module_fptr(dlhandle handle,
                              const char *modulename,
                              const char *name))(void **)
{
    void (*result)(void**);

    /* 'result = (void (*)(void))get_module_symbol(handle, modulename, 
     * name);' would seem more natural, but the C99 standard leaves 
     * casting from "void *" to a function pointer undefined. 
     * The assignment used below is the POSIX.1-2003 (Technical
     * Corrigendum 1) workaround; see the Rationale for the
     * POSIX specification of dlsym(). */
    *(void **) (&result) = get_module_symbol(handle, modulename, name);
    
    return result;
}

static char *get_version(dlhandle handle, const char *modulename)
{
    return (char*)get_module_symbol(handle, modulename, 
                                              "_ion_api_version");
}

static bool check_has_version(dlhandle handle, const char *modulename)
{
    return get_version(handle, modulename) != NULL;
}

static bool check_version(dlhandle handle, const char *modulename)
{
    char *versionstr=get_version(handle, modulename);
    if(versionstr==NULL)
        return FALSE;
    return (strcmp(versionstr, NOTION_API_VERSION)==0);
}


static bool call_init(dlhandle handle, const char *modulename)
{
    bool (*initfn)(void);
    
    initfn=(bool (*)())get_module_fptr(handle, modulename, "_init");
    
    if(initfn==NULL)
        return TRUE;
    
    return initfn();
}


static void call_deinit(dlhandle handle, const char *modulename)
{
    void (*deinitfn)(void);
    
    deinitfn=(void (*)())get_module_fptr(handle, modulename, "_deinit");
    
    if(deinitfn!=NULL)
        deinitfn();
}


/*}}}*/


/*{{{ Init */


bool ioncore_init_module_support()
{
    modules=make_rb();
    return (modules!=NULL);
}


static int try_load(const char *file, void *param)
{
    dlhandle handle=NULL;
    const char *slash, *dot;
    char *name;
    Rb_node mod;
    
    if(access(file, F_OK)!=0)
        return EXTL_TRYCONFIG_NOTFOUND;

    slash=strrchr(file, '/');
    dot=strrchr(file, '.');
    
    if(slash==NULL)
        slash=file;
    else
        slash++;
    
    if(dot<=slash){
        warn(TR("Invalid module name."));
        goto err1;
    }
    
    name=ALLOC_N(char, dot-slash+1);
    if(name==NULL)
        goto err1;
    
    strncpy(name, slash, dot-slash);
    name[dot-slash]='\0';
    
    if(get_handle(name)){
        warn_obj(file, TR("The module is already loaded."));
        goto err2;
    }
        
    handle=dlopen(file, RTLD_NOW|RTLD_GLOBAL);

    if(handle==NULL){
        warn_obj(file, "%s", dlerror());
        goto err2;
    }
    
    if(get_name(handle))
        return EXTL_TRYCONFIG_OK;
    
    if(!check_has_version(handle, name)){
        warn_obj(file, TR("Module version information for %s not found. "
                          "Refusing to use."), name);
        goto err3;
    }

    if(!check_version(handle, name)){
        warn_obj(file, TR("Module version mismatch: expected '%s', found '%s'."
                          " Refusing to use."), 
                       NOTION_API_VERSION, get_version(handle, name));
        goto err3;
    }
    
    mod=add_module(name, handle);
    
    if(mod==NULL)
        goto err3;
    
    if(!call_init(handle, name)){
        warn_obj(file, TR("Unable to initialise module %s."), name);
        rb_delete_node(mod);
        goto err3;
    }
    
    return EXTL_TRYCONFIG_OK;

err3:    
    dlclose(handle);
err2:
    free(name);
err1:
    return EXTL_TRYCONFIG_LOAD_FAILED;
}


static bool do_load_module(const char *modname)
{
    int retval;
    
    retval=extl_try_config(modname, NULL, (ExtlTryConfigFn*)try_load, 
                           NULL, "so", NULL);
    
    if(retval==EXTL_TRYCONFIG_NOTFOUND)
        warn(TR("Unable to find '%s' on search path."), modname);
    
    return (retval==EXTL_TRYCONFIG_OK);
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
        warn_obj(name, TR("Unknown module."));
        return FALSE;
    }
    
    if(mod->loaded)
        return TRUE;

    if(!call_init(mod)){
        warn_obj(name, TR("Unable to initialise module."));
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
 * Attempt to load a C-side module. 
 */
EXTL_EXPORT
bool ioncore_load_module(const char *modname)
{
    if(modname==NULL){
        warn(TR("No module to load given."));
        return FALSE;
    }
    return do_load_module(modname);
}


/*}}}*/

