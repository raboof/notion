/*
 * ion/conf.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <stdlib.h>

#include <libtu/parser.h>
#include <libtu/map.h>

#include <wmcore/common.h>
#include <wmcore/global.h>
#include <wmcore/readconfig.h>
#include <wmcore/binding.h>
#include <wmcore/conf-bindings.h>
#include <wmcore/modules.h>
#include "funtab.h"
#include "winprops.h"
#include "frame.h"
#include "frame-pointer.h"
#include "bindmaps.h"


/*{{{ Misc */


static bool opt_opaque_resize(Tokenizer *tokz, int n, Token *toks)
{
	wglobal.opaque_resize=TOK_BOOL_VAL(&(toks[1]));
	
	return TRUE;
}


static bool opt_dblclick_delay(Tokenizer *tokz, int n, Token *toks)
{
	int dd=TOK_LONG_VAL(&(toks[1]));

	wglobal.dblclick_delay=(dd<0 ? 0 : dd);
	
	return TRUE;
}


static bool opt_resize_delay(Tokenizer *tokz, int n, Token *toks)
{
	int rd=TOK_LONG_VAL(&(toks[1]));

	wglobal.resize_delay=(rd<0 ? 0 : rd);
	
	return TRUE;
}


static bool opt_warp_enabled(Tokenizer *tokz, int n, Token *toks)
{
	wglobal.warp_enabled=TOK_BOOL_VAL(&(toks[1]));
	
	return TRUE;
}


/*}}}*/


/*{{{ Bindings*/


static StringIntMap frame_areas[]={
	{"border", 		FRAME_AREA_BORDER},
	{"tab", 		FRAME_AREA_TAB},
	{"empty_tab", 	FRAME_AREA_EMPTY_TAB},
	{"client", 		FRAME_AREA_CLIENT},
	END_STRINGINTMAP
};

	
static bool opt_bindings(Tokenizer *tokz, int n, Token *toks)
{
	wmcore_begin_bindings(&ion_main_bindmap, &ion_main_funclist, frame_areas);
	return TRUE;
}


static bool opt_moveres_bindings(Tokenizer *tokz, int n, Token *toks)
{
	wmcore_begin_bindings(&ion_moveres_bindmap, &ion_moveres_funclist, NULL);
	return TRUE;
}


/*}}}*/


/*{{{ Modules */


static bool opt_module(Tokenizer *tokz, int n, Token *toks)
{
	return load_module(TOK_STRING_VAL(&(toks[1])));
}


/*}}}*/


static ConfOpt opts[]={
	/* misc */
	{"dblclick_delay", "l", opt_dblclick_delay, NULL},
	{"resize_delay", "l", opt_resize_delay, NULL},
	{"opaque_resize", "b", opt_opaque_resize, NULL},
	{"warp_enabled", "b", opt_warp_enabled, NULL},
	
	/* window props */
	{"winprop" , "s", ion_begin_winprop, ion_winprop_opts},

	/* bindings */
	{"bindings", NULL, opt_bindings, wmcore_binding_opts},
	{"moveres_bindings", NULL, opt_moveres_bindings, wmcore_binding_opts},
	
	/* modules */
	{"module", "s", opt_module, NULL},
	
	END_CONFOPTS
};



bool ion_read_config(const char *cfgfile)
{
	if(cfgfile!=NULL)
		return read_config(cfgfile, opts);
	else
		return read_config_for("ion", opts);
}
