/*
 * ion/floatws/main.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <libtu/tokenizer.h>
#include <ioncore/binding.h>
#include <ioncore/conf-bindings.h>
#include <ioncore/functionp.h>
#include <ioncore/function.h>
#include <ioncore/readconfig.h>
#include <ioncore/genframep.h>
#include <ioncore/genframe-pointer.h>
#include <ioncore/reginfo.h>

#include "floatws.h"
#include "floatframe.h"


/*{{{ Module information */


#include "../version.h"

char floatws_module_ion_version[]=ION_VERSION;


/*}}}*/


/*{{{ Bindmaps */


WBindmap floatws_bindmap=BINDMAP_INIT;
WBindmap floatframe_bindmap=BINDMAP_INIT;


/*}}}*/


/*{{{ Function tables */


WFunclist floatws_funclist=INIT_FUNCLIST;

static WFunction floatws_funtab[]={
	FN_VOID(generic, WFloatWS, "floatws_circulate", floatws_circulate),
	FN_VOID(generic, WFloatWS, "floatws_backcirculate", floatws_backcirculate),
	END_FUNTAB
};


WFunclist floatframe_funclist=INIT_FUNCLIST;

static WFunction floatframe_funtab[]={
	FN_VOID(generic, WFloatFrame, "floatframe_p_move", genframe_p_move_setup),
	FN_VOID(generic, WFloatFrame, "floatframe_raise", floatframe_raise),
	FN_VOID(generic, WFloatFrame, "floatframe_lower", floatframe_lower),
#if 0
	FN_VOID(generic, WFloatFrame, "floatframe_close", floatframe_close),
	/* Common functions */
	FN_VOID(generic, WFloatFrame, "close",	floatframe_close),
#endif	
	END_FUNTAB
};


/*}}}*/


/*{{{ Config */


static StringIntMap frame_areas[]={
	{"border", 		WGENFRAME_AREA_BORDER},
	{"tab", 		WGENFRAME_AREA_TAB},
	{"empty_tab", 	WGENFRAME_AREA_EMPTY_TAB},
	{"client", 		WGENFRAME_AREA_CLIENT},
	END_STRINGINTMAP
};


static bool begin_floatframe_bindings(Tokenizer *tokz, int n, Token *toks)
{
	return ioncore_begin_bindings(&floatframe_bindmap, frame_areas);
}


static bool begin_floatws_bindings(Tokenizer *tokz, int n, Token *toks)
{
	return ioncore_begin_bindings(&floatws_bindmap, NULL);
}


static ConfOpt opts[]={
	{"floatws_bindings", NULL, begin_floatws_bindings,
	 ioncore_binding_opts},
	{"floatframe_bindings", NULL, begin_floatframe_bindings,
	 ioncore_binding_opts},
	END_CONFOPTS
};


/*}}}*/


/*{{{ Init & deinit */


void floatws_module_deinit()
{
	clear_funclist(&floatws_funclist);
	clear_funclist(&floatframe_funclist);
	deinit_bindmap(&floatws_bindmap);
	deinit_bindmap(&floatframe_bindmap);
	unregister_region_class(&OBJDESCR(WFloatWS));
}



bool floatws_module_init()
{
	if(!add_to_funclist(&floatws_funclist, floatws_funtab) ||
	   !add_to_funclist(&floatframe_funclist, floatframe_funtab)){
		goto err;
	}
	
	if(!register_region_class(&OBJDESCR(WFloatWS),
							  (WRegionSimpleCreateFn*) create_floatws,
							  (WRegionLoadCreateFn*) floatws_load)){
	   goto err;
	}

	if(read_config_for("floatws", opts))
		return TRUE;
	
err:
	floatws_module_deinit();
	return FALSE;
}


/*}}}*/

