/*
 * ion/query/main.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <ioncore/binding.h>
#include <ioncore/conf-bindings.h>
#include <ioncore/readconfig.h>
#include <ioncore/genframe.h>

#include "query.h"
#include "edln.h"
#include "wedln.h"
#include "input.h"
#include "complete.h"


/*{{{ Module information */


#include "../version.h"

char query_module_ion_version[]=ION_VERSION;


/*}}}*/


/*{{{ Bindmaps w/ config */


WBindmap query_bindmap=BINDMAP_INIT;
WBindmap query_wedln_bindmap=BINDMAP_INIT;


EXTL_EXPORT
void query_bindings(ExtlTab tab)
{
	process_bindings(&query_bindmap, NULL, tab);
}


EXTL_EXPORT
void query_wedln_bindings(ExtlTab tab)
{
	process_bindings(&query_wedln_bindmap, NULL, tab);
}

/*}}}*/


/*{{{ Init & deinit */


extern bool query_module_register_exports();
extern void query_module_unregister_exports();


void query_module_deinit()
{
	query_module_unregister_exports();
	deinit_bindmap(&query_bindmap);
	deinit_bindmap(&query_wedln_bindmap);
}


bool query_module_init()
{
	bool ret;
	
	ret=query_module_register_exports();
	
	if(ret)
		ret=read_config_for("query");
	
	if(!ret)
		query_module_deinit();
		
	return ret;
}


/*}}}*/

