/*
 * ion/floatws/main.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <ioncore/binding.h>
#include <ioncore/conf-bindings.h>
#include <ioncore/readconfig.h>
#include <ioncore/genframep.h>
#include <ioncore/genframe-pointer.h>
#include <ioncore/reginfo.h>
#include <ioncore/hooks.h>
#include <ioncore/clientwin.h>
#include <ioncore/extl.h>

#include "floatws.h"
#include "floatframe.h"


/*{{{ Module information */


#include "../version.h"

char floatws_module_ion_version[]=ION_VERSION;


/*}}}*/


/*{{{ Bindmaps w/ config */


WBindmap floatws_bindmap=BINDMAP_INIT;
WBindmap floatframe_bindmap=BINDMAP_INIT;
WBindmap floatframe_moveres_bindmap=BINDMAP_INIT;


static StringIntMap frame_areas[]={
	{"border", 		WGENFRAME_AREA_BORDER},
	{"tab", 		WGENFRAME_AREA_TAB},
	{"empty_tab", 	WGENFRAME_AREA_EMPTY_TAB},
	{"client", 		WGENFRAME_AREA_CLIENT},
	END_STRINGINTMAP
};


/*EXTL_DOC
 * Sets up bindings for WFloatFrames.
 */
EXTL_EXPORT
void floatframe_bindings(ExtlTab tab)
{
	process_bindings(&floatframe_bindmap, frame_areas, tab);
}


/*EXTL_DOC
 * Sets up bindings for move/resize mode of WFloatFrames.
 */
EXTL_EXPORT
void floatframe_moveres_bindings(ExtlTab tab)
{
	process_bindings(&floatframe_moveres_bindmap, NULL, tab);
}


/*EXTL_DOC
 * Sets up bindings available in all objects on WFloatWS:s.
 */
EXTL_EXPORT
void floatws_bindings(ExtlTab tab)
{
	process_bindings(&floatws_bindmap, NULL, tab);
}


/*}}}*/


/*{{{ Init & deinit */


extern bool floatws_module_register_exports();
extern bool floatws_module_unregister_exports();


void floatws_module_deinit()
{
	REMOVE_HOOK(add_clientwin_alt, add_clientwin_floatws_transient);

	floatws_module_unregister_exports();
	deinit_bindmap(&floatws_bindmap);
	deinit_bindmap(&floatframe_bindmap);
	unregister_region_class(&OBJDESCR(WFloatWS));
	unregister_region_class(&OBJDESCR(WFloatFrame));
}



bool floatws_module_init()
{
	if(!floatws_module_register_exports()){
		warn_obj("floatws module", "failed to register functions.");
		goto err;
	}
	
	if(!register_region_class(&OBJDESCR(WFloatWS),
							  (WRegionSimpleCreateFn*) create_floatws,
							  (WRegionLoadCreateFn*) floatws_load) ||
	   !register_region_class(&OBJDESCR(WFloatFrame), NULL,
							  (WRegionLoadCreateFn*) floatframe_load)){
		warn_obj("floatws module", "failed to register classes.");
		goto err;
	}

	read_config_for("floatws");

	if(floatframe_bindmap.nbindings==0){
		warn_obj("floatws module", "Inadequate binding configurations. "
				 "Refusing to load module. Please fix your configuration.");
		goto err;
	}
	
	ADD_HOOK(add_clientwin_alt, add_clientwin_floatws_transient);

	return TRUE;
	
err:
	floatws_module_deinit();
	return FALSE;
}


/*}}}*/

