/*
 * ion/query/main.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <ioncore/binding.h>
#include <ioncore/conf-bindings.h>
#include <ioncore/readconfig.h>
#include <ioncore/frame.h>
#include <ioncore/saveload.h>
#include <ioncore/extl.h>

#include "query.h"
#include "edln.h"
#include "wedln.h"
#include "input.h"
#include "complete.h"


/*{{{ Module information */


#include "../version.h"

char mod_query_ion_api_version[]=ION_API_VERSION;


/*}}}*/


/*{{{ Bindmaps w/ config */


WBindmap mod_query_input_bindmap=BINDMAP_INIT;
WBindmap mod_query_wedln_bindmap=BINDMAP_INIT;


EXTL_EXPORT_AS(global, __defbindings_WInput)
bool mod_query_defbindings_WInput(ExtlTab tab)
{
    return bindmap_do_table(&mod_query_input_bindmap, NULL, tab);
}


EXTL_EXPORT_AS(global, __defbindings_WEdln)
bool mod_query_defbindings_WEdln(ExtlTab tab)
{
    return bindmap_do_table(&mod_query_wedln_bindmap, NULL, tab);
}

/*}}}*/


/*{{{ Init & deinit */


extern bool mod_query_register_exports();
extern void mod_query_unregister_exports();


static void load_history()
{
    ExtlTab tab;
    int i, n;
        
    if(!ioncore_read_savefile("query_history", &tab))
        return;
    
    n=extl_table_get_n(tab);
    
    for(i=n; i>=1; i--){
        char *s=NULL;
        if(extl_table_geti_s(tab, i, &s)){
            mod_query_history_push(s);
            free(s);
        }
    }
    
    extl_unref_table(tab);
}


static void save_history()
{
    ExtlTab tab;
    int i;
    
    tab=extl_create_table();

    for(i=0; ; i++){
        const char *histent=mod_query_history_get(i);
        if(!histent)
            break;
        extl_table_seti_s(tab, i+1, histent);
    }
    
    ioncore_write_savefile("query_history", tab);
    
    extl_unref_table(tab);
}


static bool loaded_ok=FALSE;

void mod_query_deinit()
{
    mod_query_unregister_exports();
    bindmap_deinit(&mod_query_input_bindmap);
    bindmap_deinit(&mod_query_wedln_bindmap);
    
    if(loaded_ok)
        save_history();
}


bool mod_query_init()
{
    if(!mod_query_register_exports())
        goto err;
    
    ioncore_read_config("query", NULL, TRUE);

    load_history();
    
    loaded_ok=TRUE;
    
    return TRUE;
    
err:
    mod_query_deinit();
    return FALSE;
}


/*}}}*/

