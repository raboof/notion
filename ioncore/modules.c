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

#include <ltdl.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "modules.h"
#include "readconfig.h"
#include "../version.h"


static lt_dlcaller_id ltid=0;


/*{{{ Loading and initialization code */


static void *get_module_symbol(lt_dlhandle handle, char *name)
{
	const lt_dlinfo *info;
	char *p;
	void *ret;
	
	info=lt_dlgetinfo(handle);
	
	if(info==NULL || info->name==NULL){
		warn("lt_dlgetinfo() failed to return module name.");
		return NULL;
	}

	p=scat(info->name, name);
	if(p==NULL){
		warn_err();
		return NULL;
	}
	
	ret=lt_dlsym(handle, p);
	
	free(p);
	
	return ret;
}
							   

static bool check_version(lt_dlhandle handle)
{
	char *versionstr=(char*)get_module_symbol(handle, "_module_ion_api_version");
	if(versionstr==NULL)
		return FALSE;
	return (strcmp(versionstr, ION_API_VERSION)==0);
}


static bool call_init(lt_dlhandle handle)
{
	bool (*initfn)(void);
	
	initfn=(bool (*)())get_module_symbol(handle, "_module_init");
	
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
	
	ltid=lt_dlcaller_register();
	
	return TRUE;
}


/*EXTL_DOC
 * Attempt to load module \var{modname}. Ion will use libltdl to search
 * the library path (the default setting is \file{\~{}/.ion2/libs} and
 * \file{\$PREFIX/lib/ion}) and also try diffent extensions, so only
 * the module name should usually be necessary to give here.
 */
EXTL_EXPORT
bool load_module(const char *modname)
{
	lt_dlhandle handle=NULL;
	
	if(modname==NULL)
		return FALSE;
	
	handle=lt_dlopenext(modname);
	
	if(handle==NULL){
		warn("Failed to load module %s: %s", modname, lt_dlerror());
		return FALSE;
	}
	
	if(lt_dlcaller_set_data(ltid, handle, &ltid)!=NULL){
		warn("Module %s already loaded", modname);
		return TRUE;
	}
	
	if(!check_version(handle)){
		warn_obj(modname, "Module version information not found or version "
				 "mismatch. Refusing to use.");
		goto err;
	}
	
	if(!call_init(handle))
		goto err;
	
	return TRUE;
	
err:
	lt_dlclose(handle);
	return FALSE;
}

/*}}}*/


/*{{{ Deinit */


static void call_deinit(lt_dlhandle handle, char *name)
{
	void (*deinitfn)(void);
	
	deinitfn=(void (*)())get_module_symbol(handle, "_module_deinit");
	
	if(deinitfn!=NULL)
		deinitfn();
}


static int do_unload_module(lt_dlhandle handle, void *unused)
{
	const lt_dlinfo *info;

	info=lt_dlgetinfo(handle);
	if(info==NULL || info->name==NULL){
		warn("Unable to get module name.");
	}else{
		call_deinit(handle, info->name);
	}

	lt_dlclose(handle);
	
	return 0;
}


void unload_modules()
{
	lt_dlforeach(do_unload_module, NULL);
}


/*}}}*/


