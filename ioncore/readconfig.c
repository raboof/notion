/*
 * ion/ioncore/readconfig.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>
#include <unistd.h>
#include <ltdl.h>
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
	int status;
	va_list args;
	const char *spec;
	const char *rspec;
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


bool ioncore_add_scriptdir(const char *dir)
{
	return add_dir(&scriptpaths, &n_scriptpaths, dir);
}


bool ioncore_add_moduledir(const char *dir)
{
	return (lt_dlinsertsearchdir(lt_dlgetsearchpath(), dir)==0);
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
			fails-=ioncore_add_scriptdir(userdir);
		}
		
		libtu_asprintf(&tmp, "%s/.%s/lib", home, appname);
		if(tmp==NULL){
			warn_err();
		}else{
			fails-=ioncore_add_moduledir(tmp);
			free(tmp);
		}
	}
	
	return (fails==0);
}


const char* ioncore_userdir()
{
	return userdir;
}


bool ioncore_set_sessiondir(const char *session)
{
	char *tmp;
	bool ret=FALSE;
	
	if(sessiondir!=NULL)
		return FALSE;
	
	if(userdir!=NULL){
		libtu_asprintf(&tmp, "%s/%s", userdir, session);
		if(tmp==NULL){
			warn_err();
		}else{
			ret=ioncore_add_scriptdir(tmp);
			if(sessiondir!=NULL)
				free(sessiondir);
			sessiondir=tmp;
		}
	}
	
	return ret;
}


bool ioncore_add_default_dirs()
{
	int fails=4;
	
	fails-=ioncore_add_scriptdir(EXTRABINDIR); /* ion-completefile */
	fails-=ioncore_add_scriptdir(ETCDIR);
	fails-=ioncore_add_scriptdir(SHAREDIR);
	fails-=ioncore_add_moduledir(MODULEDIR);
	
	return (fails==0);
}


/*}}}*/


/*{{{ try_etcpath, do_include, etc. */


static int do_try(const char *dir, const char *file, TryConfigFn *tryfn,
				  void *tryfnparam)
{
	char *tmp=NULL;
	int ret;
	
	libtu_asprintf(&tmp, "%s/%s", dir, file);
	if(tmp==NULL){
		warn_err();
		return TRYCONFIG_MEMERROR;
	}
	ret=tryfn(tmp, tryfnparam);
	free(tmp);
	return ret;
}


static int try_dir(const char *const *files, const char *cfdir,
				   TryConfigFn *tryfn, void *tryfnparam)
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
	
	return TRYCONFIG_NOTFOUND;
}


static int try_etcpath(const char *const *files, 
						TryConfigFn *tryfn, void *tryfnparam)
{
	const char *const *file;
	int i, ret;
	
	for(i=n_scriptpaths-1; i>=0; i--){
		for(file=files; *file!=NULL; file++){
			ret=do_try(scriptpaths[i], *file, tryfn, tryfnparam);
			if(ret>=0)
				return ret;
		}
	}
	
	return TRYCONFIG_NOTFOUND;
}


static int try_lookup(const char *file, char **ptr)
{
	if(access(file, F_OK)!=0)
		return TRYCONFIG_NOTFOUND;
	*ptr=scopy(file);
	if(*ptr==NULL)
		warn_err();
	return (*ptr!=NULL);
}


static int try_load(const char *file, TryCallParam *param)
{
	if(access(file, F_OK)!=0)
		return TRYCONFIG_NOTFOUND;
	
	if(param->status==1)
		warn("Falling back to %s", file);
	
	if(!extl_loadfile(file, &(param->fn))){
		param->status=1;
		return TRYCONFIG_LOAD_FAILED;
	}
	
	return TRYCONFIG_OK;
}


static int try_call(const char *file, TryCallParam *param)
{
	int ret=try_load(file, param);
	
	if(ret!=TRYCONFIG_OK)
		return ret;
	
	ret=extl_call_vararg(param->fn, param->spec, param->rspec, &(param->args));
	
	extl_unref_fn(param->fn);
	
	return (ret ? TRYCONFIG_OK : TRYCONFIG_CALL_FAILED);
}


static int try_call_nargs(const char *file, TryCallParam *param)
{
	int ret=try_load(file, param);
	
	if(ret!=TRYCONFIG_OK)
		return ret;
	
	ret=extl_call(param->fn, NULL, NULL);
	
	extl_unref_fn(param->fn);
	
	return (ret ? TRYCONFIG_OK : TRYCONFIG_CALL_FAILED);
}


static int do_try_config(const char *fname, const char *cfdir,
						 TryConfigFn *tryfn, void *tryfnparam)
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
#ifdef EXTL_COMPILED_EXTENSION
		files[n]=scat(fname, "." EXTL_COMPILED_EXTENSION);
		if(files[n]==NULL)
			warn_err();
		else
			n++;
#endif
		files[n]=scat(fname, "." EXTL_EXTENSION);
		if(files[n]==NULL)
			warn_err();
		else
			n++;
	}
	
	if(!search)
		ret=try_dir((const char**)&files, cfdir, tryfn, tryfnparam);
	else
		ret=try_etcpath((const char**)&files, tryfn, tryfnparam);
	
	while(n>0)
		free(files[--n]);
	
	return ret;
}


int try_config(const char *module, TryConfigFn *tryfn, void *tryfnparam)
{
	return do_try_config(module, NULL, tryfn, tryfnparam);
}


EXTL_EXPORT
char *lookup_script(const char *file, const char *try_in_dir)
{
	const char *files[]={NULL, NULL};
	char* tmp=NULL;
	
	if(file!=NULL){
		files[0]=file;

		if(try_in_dir!=NULL)
			try_dir(files, try_in_dir, (TryConfigFn*)try_lookup, &tmp);
		if(tmp==NULL)
			try_etcpath(files, (TryConfigFn*)try_lookup, &tmp);
	}
	
	return tmp;
}


bool do_include(const char *file, const char *current_file_dir)
{
	TryCallParam param;
	int retval;
	
	if(file==NULL){
		warn("No file to include given.");
		return FALSE;
	}
	
	param.status=0;
	
	retval=do_try_config(file, current_file_dir,
						 (TryConfigFn*)try_call_nargs, &param);
	
	if(retval==TRYCONFIG_NOTFOUND)
		warn("Unable to find '%s'.", file);
	
	return (retval==TRYCONFIG_OK);
}


/*}}}*/


/*{{{ read_config */


bool read_config(const char *module)
{
	return read_config_args(module, TRUE, NULL, NULL);
}


bool read_config_args(const char *module, bool warn_nx,
					  const char *spec, const char *rspec, ...)
{
	bool ret;
	TryCallParam param;
	
	param.status=0;
	param.spec=spec;
	param.rspec=rspec;
	
	va_start(param.args, rspec);
	
	ret=try_config(module, (TryConfigFn*)try_call, &param);
	
	if(ret==TRYCONFIG_NOTFOUND && warn_nx){
		warn("Unable to find configuration file '%s' on search path", 
			 module);
	}
	
	va_end(param.args);
	
	return (ret==TRYCONFIG_OK);
}


/*}}}*/


/*{{{ get_savefile */


/*EXTL_DOC
 * Get a file name to save (session) data in. The string \var{basename} should
 * contain no path or extension components.
 */
EXTL_EXPORT
char *get_savefile(const char *basename)
{
	char *res=NULL;
	
	if(sessiondir!=NULL)
		libtu_asprintf(&res, "%s/%s." EXTL_EXTENSION, sessiondir, basename);
	
	return res;
}


/*}}}*/
