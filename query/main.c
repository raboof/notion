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
void query_bindings(ExtlTab tab)
{
	process_bindings(&query_bindmap, NULL, tab);
}

/*EXTL_DOC
 * Add a set of bindings available in the line editor.
 */
EXTL_EXPORT
void query_wedln_bindings(ExtlTab tab)
{
	process_bindings(&query_wedln_bindmap, NULL, tab);
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
	char *filename=get_cfgfile_for("saves/wedln_history");
	if(filename!=NULL){
		read_config(filename);
		free(filename);
	}
}


static void save_history()
{
	char *fname;
	const char *histent;
	FILE *file;
	int i=0;
	
	fname=get_savefile_for("saves/wedln_history");
	
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


void query_module_deinit()
{
	query_module_unregister_exports();
	deinit_bindmap(&query_bindmap);
	deinit_bindmap(&query_wedln_bindmap);
	
	save_history();
}


bool query_module_init()
{
	if(!query_module_register_exports())
		goto err;
	
	read_config_for("query");

	if(query_bindmap.nbindings==0){
		warn_obj("query module", "Inadequate binding configurations. "
				 "Refusing to load module. Please fix your configuration.");
		goto err;
	}

	load_history();
	
	return TRUE;
	
err:
	query_module_deinit();
	return FALSE;
}


/*}}}*/

