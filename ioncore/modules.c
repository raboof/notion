/*
 * ion/ioncore/modules.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef CF_STATIC_MODULES

#include <dlfcn.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "modules.h"
#include "readconfig.h"
#include "../version.h"


INTRSTRUCT(Module)
	
DECLSTRUCT(Module){
	void *handle;
	char *name;
	Module *next, *prev;
};

static Module *modules=NULL;


/*{{{ Loading and initialization code */


static bool check_version(void *handle, char *name)
{
	char *p=scat(name, "_module_ion_version");
	char *versionstr;

	if(p==NULL){
		warn_err();
		return FALSE;
	}
	
	versionstr=(char*)dlsym(handle, p);
	
	free(p);
	
	if(versionstr==NULL)
		return FALSE;
	
	return (strcmp(versionstr, ION_VERSION)==0);
}


static bool call_init(void *handle, char *name)
{
	char *p=scat(name, "_module_init");
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
	
	handle=dlopen(fname, RTLD_NOW|RTLD_GLOBAL);
	
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
	if(!check_version(handle, n)){
		warn_obj(fname, "Module version information not found or version "
				 "mismatch. Refusing to use.");
		goto err3;
	}
	
	if(!call_init(handle, n))
		goto err3;

	LINK_ITEM(modules, m, next, prev);
	
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

	if(strchr(name, '.')==NULL){
		name2=scat(name, ".so");
		if(name2==NULL){
			warn_err();
			return FALSE;
		}
		ret=load_module(name2);
		free(name2);
		return ret;
	}

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
	char *p=scat(name, "_module_deinit");
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
	Module *m;
	
	while(modules!=NULL){
		m=modules->prev;
		UNLINK_ITEM(modules, m, next, prev);
		do_unload_module(m);
	}
}


/*}}}*/


#else /* CF_STATIC_MODULES */


#include "common.h"

INTRSTRUCT(StatModInfo)

DECLSTRUCT(StatModInfo){
	char *name;
	bool (*initfn)();
	void (*deinitfn)();
	bool loaded;
};

#include "static-modules.h"

/*{{{ Dummy functions for systems without sufficient dynamic
 * linking support
 */

bool load_module(const char *name)
{
	int i;
	StatModInfo *smi=available_modules;
		
	for( ; smi->name!=NULL; smi++){
		if(strcmp(smi->name, name)==0)
			break;
	}
	
	if(smi->name==NULL){
		warn_obj(name, "No such statically compiled module found");
		return FALSE;
	}
	
	if(smi->loaded)
		return TRUE;
	
	if(!smi->initfn())
		return FALSE;
		
	smi->loaded=TRUE;
	return TRUE;
}


void unload_modules()
{
	StatModInfo *smi=available_modules;
		
	for( ; smi->name!=NULL; smi++){
		if(!smi->loaded)
			continue;
		smi->deinitfn();
		smi->loaded=FALSE;
	}
}


/*}}}*/


#endif /* CF_STATIC_MODULES */
