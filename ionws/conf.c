/*
 * ion/ionws/conf.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <stdlib.h>

#include <libtu/parser.h>
#include <libtu/map.h>

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/readconfig.h>
#include <ioncore/binding.h>
#include <ioncore/conf-bindings.h>
#include <ioncore/funtabs.h>
#include <ioncore/genframep.h>
#include "funtabs.h"
#include "ionframe.h"
#include "bindmaps.h"


static StringIntMap frame_areas[]={
	{"border", 		WGENFRAME_AREA_BORDER},
	{"tab", 		WGENFRAME_AREA_TAB},
	{"empty_tab", 	WGENFRAME_AREA_EMPTY_TAB},
	{"client", 		WGENFRAME_AREA_CLIENT},
	END_STRINGINTMAP
};

	
static bool opt_ionws_bindings(Tokenizer *tokz, int n, Token *toks)
{
	return ioncore_begin_bindings(&ionws_bindmap, NULL);
}


static bool opt_ionframe_bindings(Tokenizer *tokz, int n, Token *toks)
{
	return ioncore_begin_bindings(&ionframe_bindmap, frame_areas);
}


static bool opt_moveres_bindings(Tokenizer *tokz, int n, Token *toks)
{
	return ioncore_begin_bindings(&ionframe_moveres_bindmap, NULL);
}


static ConfOpt opts[]={
	/* bindings */
	{"ionws_bindings", NULL, opt_ionws_bindings, ioncore_binding_opts},
	{"ionframe_bindings", NULL, opt_ionframe_bindings, ioncore_binding_opts},
	{"ionframe_moveres_bindings", NULL, opt_moveres_bindings,
	 ioncore_binding_opts},
	
	END_CONFOPTS
};



bool ionws_module_read_config()
{
	return read_config_for("ionws", opts);
}
