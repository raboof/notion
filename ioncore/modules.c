/*
 * wmcore/modules.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef CF_NO_MODULE_SUPPORT

#include <dlfcn.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "modules.h"
#include "readconfig.h"


INTRSTRUCT(Module)
	
DECLSTRUCT(Module){
	void *handle;
	char *name;
	Module *next;
};

static Module *modules=NULL, *last_module=NULL;


/*{{{ Loading and initialization code */


static bool call_init(void *handle, char *name)
{
	char *p=scat(name, "_init");
	bool (*initfn)(void);

	if(p==NULL){
		warn_err();
		return FALSE;
	}
	
	initfn=(bool (*)())dlsym(handle, p);
	
	free(p);
	
	if(initfn==NULL)
		return TRUE;
	
	return initfn();
}


static bool do_load_module(const char *fname)
{
	void *handle;
	Module *m;
	const char *p;
	char *n;
	size_t l;
	
	handle=dlopen(fname, RTLD_LAZY);
	
	if(handle==NULL){
		warn_obj(fname, "%s", dlerror());
		return FALSE;
	}

	for(m=modules; m!=NULL; m=m->next){
		if(m->handle==handle)
			return TRUE;
	}
	
	/* Get the module name without directory or extension */
	
	p=strrchr(fname, '/');
	
	if(p!=NULL)
		fname=p+1;
	
	for(p=fname; *p!='\0'; p++){
		if(!isalnum(*p) && *p!='_')
			break;
	}
	
	n=ALLOC_N(char, p-fname);
	
	if(n==NULL){
		warn_err();
		goto err1;
	}
	 
	memcpy(n, fname, p-fname);
	n[p-fname]='\0';
	
	/* Allocate space for module info */
	
	m=ALLOC(Module);
	
	if(m==NULL){
		warn_err();
		goto err2;
	}
	
	m->name=n;
	m->handle=handle;
	m->next=NULL;
	
	/* initialize */
	
	if(!call_init(handle, n))
		goto err3;

	if(last_module==NULL){
		modules=last_module=m;
	}else{
		last_module->next=m;
		last_module=m;
	}
	
	return TRUE;
	
err3:
	free(m);
err2:
	free(n);
err1:
	dlclose(handle);
	return FALSE;
}


bool load_module(const char *name)
{
	char *name2=NULL;
	bool ret=FALSE;
	
	if(strchr(name, '/')!=NULL)
		return do_load_module(name);

	name2=find_module(name);
	
	if(name2!=NULL){
		ret=do_load_module(name2);
		free(name2);
	}
	
	return ret;
}


/*}}}*/


/*{{{ Deinit */


static void call_deinit(void *handle, char *name)
{
	char *p=scat(name, "_deinit");
	void (*deinitfn)(void);

	if(p==NULL){
		warn_err();
		return;
	}
	
	deinitfn=(void (*)())dlsym(handle, p);
	
	free(p);
	
	if(deinitfn!=NULL)
		deinitfn();
}


static void do_unload_module(Module *m)
{
	call_deinit(m->handle, m->name);

	dlclose(m->handle);
	if(m->name!=NULL)
		free(m->name);
	free(m);
}


void unload_modules()
{
	Module *m, *next;
	
	for(m=modules; m!=NULL; m=next){
		next=m->next;
		do_unload_module(m);
	}
	modules=NULL;
	last_module=NULL;
}


/*}}}*/


#else /* CF_MODULE_SUPPORT */


#include "common.h"


/*{{{ Dummy functions for systems without sufficient dynamic
 * linking support
 */

bool load_module(const char *name)
{
	warn_obj(name, "Unable to load: module support not enabled.");
	return FALSE;
}


void unload_modules()
{
}


/*}}}*/


#endif
