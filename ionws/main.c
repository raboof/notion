/*
 * ion/ionws/main.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <ioncore/common.h>
#include <ioncore/reginfo.h>
#include <ioncore/readconfig.h>

#include "bindmaps.h"
#include "ionws.h"
#include "ionframe.h"


/*{{{ Module information */


#include "../version.h"

char ionwsmod_ion_api_version[]=ION_API_VERSION;


/*}}}*/


/*{{{ Module init & deinit */


extern bool ionwsmod_register_exports();
extern void ionwsmod_unregister_exports();


void ionwsmod_deinit()
{
	ionwsmod_unregister_exports();
	bindmap_deinit(&ionws_bindmap);
	bindmap_deinit(&ionframe_bindmap);
	ioncore_unregister_regclass(&CLASSDESCR(WIonWS));
	ioncore_unregister_regclass(&CLASSDESCR(WIonFrame));
}


static bool register_regions()
{
	if(!ioncore_register_regclass(&CLASSDESCR(WIonFrame), NULL,
							  (WRegionLoadCreateFn*) ionframe_load)){
		return FALSE;
	}
	
	if(!ioncore_register_regclass(&CLASSDESCR(WIonWS),
							  (WRegionSimpleCreateFn*) create_ionws_simple,
							  (WRegionLoadCreateFn*) ionws_load)){
	   return FALSE;
	}
	
	return TRUE;
}


bool ionwsmod_init()
{
	if(!ionwsmod_register_exports()){
		warn_obj("ionwsmod", "Unable to register exports");
		goto err;
	}
	
	if(!register_regions()){
		warn_obj("ionwsmod", "Unable to register classes");
		goto err;
	}
	
	ioncore_read_config("ionws", NULL, TRUE);

	return TRUE;
	
err:
	ionwsmod_deinit();
	return FALSE;
}


/*}}}*/

