/*
 * ion/ionws/main.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <ioncore/common.h>
#include <ioncore/reginfo.h>
#include <query/main.h>

#include "conf.h"
#include "bindmaps.h"
#include "ionws.h"
#include "ionframe.h"


/*{{{ Module information */


#include "../version.h"

char ionws_module_ion_api_version[]=ION_API_VERSION;


/*}}}*/


/*{{{ Module init & deinit */


extern bool ionws_module_register_exports();
extern void ionws_module_unregister_exports();


void ionws_module_deinit()
{
	ionws_module_unregister_exports();
	deinit_bindmap(&ionws_bindmap);
	deinit_bindmap(&ionframe_bindmap);
	deinit_bindmap(&ionframe_moveres_bindmap);
	unregister_region_class(&OBJDESCR(WIonWS));
	unregister_region_class(&OBJDESCR(WIonFrame));
}


static bool register_regions()
{
	if(!register_region_class(&OBJDESCR(WIonFrame), NULL,
							  (WRegionLoadCreateFn*) ionframe_load)){
		return FALSE;
	}
	
	if(!register_region_class(&OBJDESCR(WIonWS),
							  (WRegionSimpleCreateFn*) create_ionws_simple,
							  (WRegionLoadCreateFn*) ionws_load)){
	   return FALSE;
	}
	
	return TRUE;
}


bool ionws_module_init()
{
	if(!ionws_module_register_exports()){
		warn_obj("ionws module", "Unable to register exports");
		goto err;
	}
	
	if(!register_regions()){
		warn_obj("ionws module", "Unable to register classes");
		goto err;
	}
	
	ionws_module_read_config();
	
	return TRUE;
	
err:
	ionws_module_deinit();
	return FALSE;
}


/*}}}*/

