/*
 * ion/ionws/main.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
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

char ionws_module_ion_version[]=ION_VERSION;


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
	
	if(!ionws_module_read_config())
		goto err2;
	
	if(ionframe_bindmap.nbindings==0 ||
	   ionframe_moveres_bindmap.nbindings==0)
		goto err2;

	return TRUE;
	
err2:
	warn("Unable to load configuration or inadequate binding "
		 "configurations. Refusing to load module.\nYou *must* "
		 "install proper configuration files either in ~/.ion or "
		 ETCDIR"/ion to use Ion.");
	ionws_module_deinit();
	return FALSE;

err:
	ionws_module_deinit();
	return FALSE;
}


/*}}}*/

