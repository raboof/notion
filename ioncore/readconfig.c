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


static char *userdir;
static char* dummy_paths=NULL;
static char **scriptpaths=&dummy_paths;
static int n_scriptpaths=0;
static char *sessiondir=NULL;


typedef struct{
    ExtlFn fn;
    ExtlTab tab;
    int status;
} TryCallParam;


/*{{{ Init */


static bool add_dir(char ***pathsptr, int *n_pathsptr, const char *dir)
{
    char **paths;
    char *dircp;
    int i;
    
    if(dir==NULL)
        return FALSE;
    
    dircp=scopy(dir);
    
    if(dircp==NULL){
        warn_err();
        return FALSE;
    }
    
    if(*n_pathsptr==0)
        paths=ALLOC_N(char*, 2);
    else
        paths=REALLOC_N(*pathsptr, char*, (*n_pathsptr)+1, (*n_pathsptr)+2);
    
    if(paths==NULL){
        warn_err();
        free(dircp);
        return FALSE;
    }
    
    paths[*n_pathsptr]=dircp;
    paths[(*n_pathsptr)+1]=NULL;
    (*n_pathsptr)++;
    *pathsptr=paths;
    
    return TRUE;
}


/*EXTL_DOC
 * Add a directory to search path.
 */
EXTL_EXPORT
bool ioncore_add_searchdir(const char *dir)
{
    return add_dir(&scriptpaths, &n_scriptpaths, dir);
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
        warn("$HOME not set");
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

/*EXTL_DOC
 * Get user configuration file directory.
 */
EXTL_EXPORT
const char* ioncore_userdir()
{
    return userdir;
}


/*EXTL_DOC
 * Get session directory.
 */
EXTL_EXPORT
const char* ioncore_sessiondir()
{
    return sessiondir;
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
        warn("User directory not set. Unable to set session directory.");
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


/*EXTL_DOC
 * Get all directories on search path.
 */
EXTL_EXPORT
ExtlTab ioncore_get_scriptdirs()
{
    int i;
    ExtlTab tab=extl_create_table();
    
    for(i=0; i<n_scriptpaths; i++)
        extl_table_seti_s(tab, i+1, scriptpaths[i]);
    
    return tab;
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

    if(sessiondir!=NULL){
        for(file=files; *file!=NULL; file++){
            ret=do_try(sessiondir, *file, tryfn, tryfnparam);
            if(ret>=0)
                return ret;
        }
    }
    
    for(i=n_scriptpaths-1; i>=0; i--){
        for(file=files; *file!=NULL; file++){
            ret=do_try(scriptpaths[i], *file, tryfn, tryfnparam);
            if(ret>=0)
                return ret;
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
        warn("Falling back to %s", file);
    
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
    
    if(file==NULL){
        warn("No file to include given.");
        return FALSE;
    }
    
    param.status=0;
    
    retval=ioncore_try_config(file, sp, (WTryConfigFn*)try_call, &param,
                              EXTL_COMPILED_EXTENSION, EXTL_EXTENSION);
    
    if(retval==IONCORE_TRYCONFIG_NOTFOUND && warn_nx)
        warn("Unable to find '%s' on configuration file search path.", file);
    
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
        warn("Unable to create session directory \"%s\"\n", sessiondir);
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

