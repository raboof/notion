/*
 * ion/menu/main.c
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
#include <ioncore/saveload.h>

#include "menu.h"


/*{{{ Module information */


#include "../version.h"

char menu_module_ion_version[]=ION_VERSION;


/*}}}*/


/*{{{ Bindmaps w/ config */


WBindmap menu_bindmap=BINDMAP_INIT;


/*EXTL_DOC
 * Describe \type{WMenu} bindings.
 */
EXTL_EXPORT
bool menu_bindings(ExtlTab tab)
{
	return process_bindings(&menu_bindmap, NULL, tab);
}


/*}}}*/


/*{{{ Init & deinit */


extern bool menu_module_register_exports();
extern void menu_module_unregister_exports();


void menu_module_deinit()
{
	menu_module_unregister_exports();
	deinit_bindmap(&menu_bindmap);
}


bool menu_module_init()
{
	if(!menu_module_register_exports()){
		menu_module_deinit();
		return FALSE;
	}
	
	read_config_for("menulib");
	read_config_for("menu");
	
	return TRUE;
}


/*}}}*/

