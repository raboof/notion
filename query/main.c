/*
 * ion/query/main.c
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
#include <ioncore/genframe.h>
#include <ioncore/saveload.h>

#include "query.h"
#include "edln.h"
#include "wedln.h"
#include "input.h"
#include "complete.h"


/*{{{ Module information */


#include "../version.h"

char query_module_ion_api_version[]=ION_API_VERSION;


/*}}}*/


/*{{{ Bindmaps w/ config */


WBindmap query_bindmap=BINDMAP_INIT;
WBindmap query_wedln_bindmap=BINDMAP_INIT;


/*EXTL_DOC
 * Describe bindings common to all \type{WInput}s (\type{WEdln},
 * \type{WMessage}).
 */
EXTL_EXPORT
bool query_bindings(ExtlTab tab)
{
	return process_bindings(&query_bindmap, NULL, tab);
}

/*EXTL_DOC
 * Add a set of bindings available in the line editor.
 */
EXTL_EXPORT
bool query_wedln_bindings(ExtlTab tab)
{
	return process_bindings(&query_wedln_bindmap, NULL, tab);
}

/*}}}*/


/*{{{ Init & deinit */


extern bool query_module_register_exports();
extern void query_module_unregister_exports();


static void load_history()
{
	read_config_args("query_history", FALSE, NULL, NULL);
}


static void save_history()
{
	char *fname;
	const char *histent;
	FILE *file;
	int i=0;
	
	fname=get_savefile("query_history");
	
	if(fname==NULL){
		warn("Unable to save query history");
		return;
	}
	
	file=fopen(fname, "w");
	
	if(file==NULL){
		warn_err_obj(fname);
		return;
	}
	
	free(fname);
	
	fprintf(file, "local saves={\n");
	
	while(1){
		histent=query_history_get(i);
		if(histent==NULL)
			break;
		fprintf(file, "    ");
		write_escaped_string(file, histent);
		fprintf(file, ",\n");
		i++;
	}
	
	fprintf(file, "}\n");
	fprintf(file, "for k=table.getn(saves),1,-1 do "
			"query_history_push(saves[k]) end\n");
	
	query_history_clear();
	
	fclose(file);
}


static bool loaded_ok=FALSE;

void query_module_deinit()
{
	query_module_unregister_exports();
	deinit_bindmap(&query_bindmap);
	deinit_bindmap(&query_wedln_bindmap);
	
	if(loaded_ok)
		save_history();
}


bool query_module_init()
{
	if(!query_module_register_exports())
		goto err;
	
	read_config("query");

	load_history();
	
	loaded_ok=TRUE;
	
	return TRUE;
	
err:
	query_module_deinit();
	return FALSE;
}


/*}}}*/

