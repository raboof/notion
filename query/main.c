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

char query_module_ion_version[]=ION_VERSION;


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


/*EXTL_DOC
 * Push an entry into \type{Wedln} history.
 */
EXTL_EXPORT 
void wedln_history_push(const char *str)
{
	edlnhist_push(str);
}


static void load_history()
{
	read_config_for_args("wedln_history", -1, FALSE, NULL, NULL);
}


static void save_history()
{
	char *fname;
	const char *histent;
	FILE *file;
	int i=0;
	
	fname=get_savefile_for("wedln_history");
	
	if(fname==NULL){
		warn("Unable to save wedln history");
		return;
	}
	
	file=fopen(fname, "w");
	
	if(file==NULL){
		warn_err_obj(fname);
		return;
	}
	
	fprintf(file, "local saves={\n");
	
	while(1){
		histent=edlnhist_get(i);
		if(histent==NULL)
			break;
		fprintf(file, "    ");
		write_escaped_string(file, histent);
		fprintf(file, ",\n");
		i++;
	}
	
	fprintf(file, "}\n");
	fprintf(file, "for k=table.getn(saves),1,-1 do "
			"wedln_history_push(saves[k]) end\n");
	
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
	
	read_config_for("query");

	load_history();
	
	loaded_ok=TRUE;
	
	return TRUE;
	
err:
	query_module_deinit();
	return FALSE;
}


/*}}}*/

