/*
 * ion/ioncore/readconfig.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <libtu/output.h>

#include "common.h"
#include "global.h"
#include "readconfig.h"
#include "extl.h"


typedef struct{
    ExtlFn fn;
    ExtlTab tab;
    int status;
} TryCallParam;


/*{{{ Path setup */


static char *userdir=NULL;
static char *sessiondir=NULL;
static char *scriptpath=NULL;


bool ioncore_add_searchdir(const char *dir)
{
    if(scriptpath==NULL){
        scriptpath=scopy(dir);
        if(scriptpath==NULL){
            warn_err();
            return FALSE;
        }
    }else{
        char *p=scat3(dir, ":", scriptpath);
        if(p==NULL){
            warn_err();
            return FALSE;
        }
        free(scriptpath);
        scriptpath=p;
    }
    
    return TRUE;
}


bool ioncore_set_searchpath(const char *path)
{
    char *s=NULL;
    
    if(path!=NULL){
        s=scopy(path);
        if(s==NULL){
            warn_err();
            return FALSE;
        }
    }
    
    if(scriptpath!=NULL)
        free(scriptpath);
    
    scriptpath=s;
    return TRUE;
}


bool ioncore_set_userdirs(const char *appname)
{
    const char *home;
    char *tmp;
    int fails=2;
    
    if(userdir!=NULL)
        return FALSE;
    
    home=getenv("HOME");
    
    if(home==NULL){
        warn(TR("$HOME not set"));
    }else{
        libtu_asprintf(&userdir, "%s/.%s", home, appname);
        if(userdir==NULL){
            warn_err();
        }else{
            fails-=ioncore_add_searchdir(userdir);
        }
        
        libtu_asprintf(&tmp, "%s/.%s/lib", home, appname);
        if(tmp==NULL){
            warn_err();
        }else{
            fails-=ioncore_add_searchdir(tmp);
            free(tmp);
        }
    }
    
    return (fails==0);
}


bool ioncore_set_sessiondir(const char *session)
{
    char *tmp;
    bool ret=FALSE;
    
    if(strchr(session, '/')!=NULL){
        tmp=scopy(session);
    }else if(userdir!=NULL){
        libtu_asprintf(&tmp, "%s/%s", userdir, session);
    }else{
        warn(TR("User directory not set. "
                "Unable to set session directory."));
        return FALSE;
    }
    
    if(tmp==NULL){
        warn_err();
        return FALSE;
    }
    
    if(sessiondir!=NULL)
        free(sessiondir);
    
    sessiondir=tmp;
    
    return TRUE;
}


const char *ioncore_userdir()
{
    return userdir;
}


const char *ioncore_sessiondir()
{
    return sessiondir;
}


const char *ioncore_searchpath()
{
    return scriptpath;
}


/*EXTL_DOC
 * Get important directories (userdir, sessiondir, searchpath).
 */
EXTL_EXPORT
ExtlTab ioncore_get_paths(ExtlTab tab)
{
    tab=extl_create_table();
    extl_table_sets_s(tab, "userdir", userdir);
    extl_table_sets_s(tab, "sessiondir", sessiondir);
    extl_table_sets_s(tab, "searchpath", scriptpath);
    return tab;
}


/*EXTL_DOC
 * Set important directories (sessiondir, searchpath).
 */
EXTL_EXPORT
bool ioncore_set_paths(ExtlTab tab)
{
    char *s;

    if(extl_table_gets_s(tab, "userdir", &s)){
        WARN_FUNC(TR("User directory can not be set."));
        free(s);
        return FALSE;
    }
    
    if(extl_table_gets_s(tab, "sessiondir", &s)){
        ioncore_set_sessiondir(s);
        free(s);
        return FALSE;
    }

    if(extl_table_gets_s(tab, "searchpath", &s)){
        ioncore_set_searchpath(s);
        free(s);
        return FALSE;
    }
    
    return TRUE;
}


/*}}}*/


/*{{{ try_etcpath, do_include, etc. */


static int do_try(const char *dir, const char *file, WTryConfigFn *tryfn,
                  void *tryfnparam)
{
    char *tmp=NULL;
    int ret;
    
    libtu_asprintf(&tmp, "%s/%s", dir, file);
    if(tmp==NULL){
        warn_err();
        return IONCORE_TRYCONFIG_MEMERROR;
    }
    ret=tryfn(tmp, tryfnparam);
    free(tmp);
    return ret;
}


static int try_dir(const char *const *files, const char *cfdir,
                   WTryConfigFn *tryfn, void *tryfnparam)
{
    const char *const *file;
    int ret;
    
    if(cfdir!=NULL){
        for(file=files; *file!=NULL; file++){
            ret=do_try(cfdir, *file, tryfn, tryfnparam);
            if(ret>=0)
                return ret;
        }
    }
    
    return IONCORE_TRYCONFIG_NOTFOUND;
}


static int try_etcpath(const char *const *files, 
                       WTryConfigFn *tryfn, void *tryfnparam)
{
    const char *const *file=NULL;
    int i, ret;
    char *path, *colon, *dir;

    if(sessiondir!=NULL){
        for(file=files; *file!=NULL; file++){
            ret=do_try(sessiondir, *file, tryfn, tryfnparam);
            if(ret>=0)
                return ret;
        }
    }
    
    path=scriptpath;
    while(path!=NULL){
        colon=strchr(path, ':');
        if(colon!=NULL){
            dir=scopyn(path, colon-path);
            path=colon+1;
        }else{
            dir=scopy(path);
            path=NULL;
        }
        
        if(dir==NULL){
            warn_err();
        }else{
            if(*dir!='\0'){
                for(file=files; *file!=NULL; file++){
                    ret=do_try(dir, *file, tryfn, tryfnparam);
                    if(ret>=0){
                        free(dir);
                        return ret;
                    }
                }
            }
            free(dir);
        }
    }
    
    return IONCORE_TRYCONFIG_NOTFOUND;
}


static int try_lookup(const char *file, char **ptr)
{
    if(access(file, F_OK)!=0)
        return IONCORE_TRYCONFIG_NOTFOUND;
    *ptr=scopy(file);
    if(*ptr==NULL)
        warn_err();
    return (*ptr!=NULL);
}


static int try_load(const char *file, TryCallParam *param)
{
    if(access(file, F_OK)!=0)
        return IONCORE_TRYCONFIG_NOTFOUND;
    
    if(param->status==1)
        warn(TR("Falling back to %s."), file);
    
    if(!extl_loadfile(file, &(param->fn))){
        param->status=1;
        return IONCORE_TRYCONFIG_LOAD_FAILED;
    }
    
    return IONCORE_TRYCONFIG_OK;
}


static int try_call(const char *file, TryCallParam *param)
{
    int ret=try_load(file, param);
    
    if(ret!=IONCORE_TRYCONFIG_OK)
        return ret;
    
    ret=extl_call(param->fn, NULL, NULL);
    
    extl_unref_fn(param->fn);
    
    return (ret ? IONCORE_TRYCONFIG_OK : IONCORE_TRYCONFIG_CALL_FAILED);
}


static int try_read_savefile(const char *file, TryCallParam *param)
{
    int ret=try_load(file, param);
    
    if(ret!=IONCORE_TRYCONFIG_OK)
        return ret;
    
    ret=extl_call(param->fn, NULL, "t", &(param->tab));
    
    extl_unref_fn(param->fn);
    
    return (ret ? IONCORE_TRYCONFIG_OK : IONCORE_TRYCONFIG_CALL_FAILED);
}


int ioncore_try_config(const char *fname, const char *cfdir,
                       WTryConfigFn *tryfn, void *tryfnparam,
                       const char *ext1, const char *ext2)
{
    char *files[]={NULL, NULL, NULL};
    int n=0, ret;
    bool search=FALSE;
    
    if(cfdir==NULL || fname[0]!='.' || fname[1]!='/'){
        search=(fname[0]!='/');
        cfdir="";
    }

    if(strrchr(fname, '.')>strrchr(fname, '/')){
        files[n]=scopy(fname);
        if(files[n]==NULL)
            warn_err();
        else
            n++;
    }else{
        if(ext1!=NULL){
            files[n]=scat3(fname, ".", ext1);
            if(files[n]==NULL)
                warn_err();
            else
                n++;
        }

        if(ext2!=NULL){
            files[n]=scat3(fname, ".", ext2);
            if(files[n]==NULL)
                warn_err();
            else
                n++;
        }
    }
    
    if(!search)
        ret=try_dir((const char**)&files, cfdir, tryfn, tryfnparam);
    else
        ret=try_etcpath((const char**)&files, tryfn, tryfnparam);
    
    while(n>0)
        free(files[--n]);
    
    return ret;
}


/*EXTL_DOC
 * Lookup script \var{file}. If \var{try_in_dir} is set, it is tried
 * before the standard search path.
 */
EXTL_EXPORT
char *ioncore_lookup_script(const char *file, const char *sp)
{
    const char *files[]={NULL, NULL};
    char* tmp=NULL;
    
    if(file!=NULL){
        files[0]=file;

        if(sp!=NULL)
            try_dir(files, sp, (WTryConfigFn*)try_lookup, &tmp);
        if(tmp==NULL)
            try_etcpath(files, (WTryConfigFn*)try_lookup, &tmp);
    }
    
    return tmp;
}


bool ioncore_read_config(const char *file, const char *sp, bool warn_nx)
{
    TryCallParam param;
    int retval;
    
    if(file==NULL)
        return FALSE;
    
    param.status=0;
    
    retval=ioncore_try_config(file, sp, (WTryConfigFn*)try_call, &param,
                              EXTL_COMPILED_EXTENSION, EXTL_EXTENSION);
    
    if(retval==IONCORE_TRYCONFIG_NOTFOUND && warn_nx)
        warn(TR("Unable to find '%s' on search path."), file);

    return (retval==IONCORE_TRYCONFIG_OK);
}


bool ioncore_read_savefile(const char *basename, ExtlTab *tabret)
{
    TryCallParam param;
    int retval;
    
    param.status=0;
    param.tab=extl_table_none();
    
    retval=ioncore_try_config(basename, NULL, (WTryConfigFn*)try_read_savefile,
                              &param, EXTL_EXTENSION, NULL);

    *tabret=param.tab;
    
    return (retval==IONCORE_TRYCONFIG_OK);
}


/*}}}*/


/*{{{ ioncore_get_savefile */


static bool ensuredir(char *f)
{
    char *p;
    int tryno=0;
    bool ret=TRUE;
    
    if(access(f, F_OK)==0)
        return TRUE;
    
    if(mkdir(f, 0700)==0)
        return TRUE;
    
    p=strrchr(f, '/');
    if(p==NULL){
        warn_err_obj(f);
        return FALSE;
    }
    
    *p='\0';
    if(!ensuredir(f))
        return FALSE;
    *p='/';
        
    if(mkdir(f, 0700)==0)
        return TRUE;
    
    warn_err_obj(f);
    return FALSE;
}


/*EXTL_DOC
 * Get a file name to save (session) data in. The string \var{basename} 
 * should contain no path or extension components.
 */
EXTL_EXPORT
char *ioncore_get_savefile(const char *basename)
{
    char *res=NULL;
    
    if(sessiondir==NULL)
        return NULL;
    
    if(!ensuredir(sessiondir)){
        warn(TR("Unable to create session directory \"%s\"\n"), 
             sessiondir);
        return NULL;
    }
    
    libtu_asprintf(&res, "%s/%s." EXTL_EXTENSION, sessiondir, basename);
    
    return res;
}


EXTL_EXPORT
bool ioncore_write_savefile(const char *basename, ExtlTab tab)
{
    bool ret=FALSE;
    char *fname=ioncore_get_savefile(basename);
    
    if(fname!=NULL){
        ret=extl_serialise(fname, tab);
        free(fname);
    }
    
    return ret;
}


/*}}}*/

