/*
 * ion/ioncore/modules.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <ltdl.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "modules.h"
#include "readconfig.h"
#include "../version.h"


INTRSTRUCT(Module);
	
DECLSTRUCT(Module){
	lt_dlhandle handle;
	char *name;
	Module *next, *prev;
};

static Module *modules=NULL;



/*{{{ Loading and initialization code */


static bool check_version(lt_dlhandle handle, char *name)
{
	char *p=scat(name, "_module_ion_version");
	char *versionstr;

	if(p==NULL){
		warn_err();
		return FALSE;
	}
	
	versionstr=(char*)lt_dlsym(handle, p);
	
	free(p);
	
	if(versionstr==NULL)
		return FALSE;
	
	return (strcmp(versionstr, ION_VERSION)==0);
}


static bool call_init(lt_dlhandle handle, char *name)
{
	char *p=scat(name, "_module_init");
	bool (*initfn)(void);

	if(p==NULL){
		warn_err();
		return FALSE;
	}
	
	initfn=(bool (*)())lt_dlsym(handle, p);
	
	free(p);
	
	if(initfn==NULL)
		return TRUE;
	
	return initfn();
}


bool init_module_support()
{
#ifdef CF_PRELOAD_MODULES
	LTDL_SET_PRELOADED_SYMBOLS();
#endif
	if(lt_dlinit()!=0){
		warn("lt_dlinit: %s", lt_dlerror());
		return FALSE;
	}
	return TRUE;
}


EXTL_EXPORT
bool load_module(const char *fname)
/*static bool do_load_module(const char *fname)*/
{
	lt_dlhandle handle=NULL;
	Module *m;
	const char *p;
	char *n;
	size_t l;
	
	if(fname==NULL)
		return FALSE;
	
	handle=lt_dlopenext(fname);
	
	if(handle==NULL){
		warn("%s", lt_dlerror());
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
	lt_dlclose(handle);
	return FALSE;
}


/*EXTL_EXPORT
bool load_module(const char *name)
{
	char *name2=NULL;
	bool ret=FALSE;
	
	if(strchr(name, '/')!=NULL)
		return do_load_module(name);

	if(strchr(name, '.')==NULL){
		name2=scat(name, ".la");
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
}*/


/*}}}*/


/*{{{ Deinit */


static void call_deinit(lt_dlhandle handle, char *name)
{
	char *p=scat(name, "_module_deinit");
	void (*deinitfn)(void);

	if(p==NULL){
		warn_err();
		return;
	}
	
	deinitfn=(void (*)())lt_dlsym(handle, p);
	
	free(p);
	
	if(deinitfn!=NULL)
		deinitfn();
}


static void do_unload_module(Module *m)
{
	call_deinit(m->handle, m->name);

	lt_dlclose(m->handle);
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


