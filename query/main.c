/*
 * ion/query/main.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <ioncore/binding.h>
#include <ioncore/conf-bindings.h>
#include <ioncore/functionp.h>
#include <ioncore/readconfig.h>
#include <ioncore/genframe.h>
#include <ioncore/funtabs.h>

#include "query.h"
#include "edln.h"
#include "wedln.h"
#include "input.h"
#include "complete.h"


/*{{{ Module information */


#include "../version.h"

char query_module_ion_version[]=ION_VERSION;


/*}}}*/



/*{{{ Bindable functions and binding maps */


WBindmap query_bindmap=BINDMAP_INIT;
WBindmap query_edln_bindmap=BINDMAP_INIT;


static void callhnd_edln_void(WThing *thing, WFunction *func,
							  int n, const Token *args)
{
	WEdln *wedln;
	typedef void Func(Edln*);
	
	if(!WTHING_IS(thing, WEdln))
		return;
	
	wedln=(WEdln*)thing;
	
	((Func*)func->fn)(&(wedln->edln));
}


WFunclist query_edln_funclist=INIT_FUNCLIST;

static WFunction query_edln_funtab[]={
	FN_VOID(edln, WEdln,		"back",				edln_back),
	FN_VOID(edln, WEdln,		"forward",			edln_forward),
	FN_VOID(edln, WEdln,		"bol",				edln_bol),
	FN_VOID(edln, WEdln,		"eol",				edln_eol),
	FN_VOID(edln, WEdln,		"skip_word",		edln_skip_word),
	FN_VOID(edln, WEdln,		"bskip_word",		edln_bskip_word),
	FN_VOID(edln, WEdln,		"delete",			edln_delete),
	FN_VOID(edln, WEdln,		"backspace",		edln_backspace),
	FN_VOID(edln, WEdln,		"kill_to_eol",		edln_kill_to_eol),
	FN_VOID(edln, WEdln,		"kill_to_bol",		edln_kill_to_bol),
	FN_VOID(edln, WEdln,		"kill_line",		edln_kill_line),
	FN_VOID(edln, WEdln,		"kill_word",		edln_kill_word),
	FN_VOID(edln, WEdln,		"bkill_word",		edln_bkill_word),
	FN_VOID(edln, WEdln,		"set_mark",			edln_set_mark),
	FN_VOID(edln, WEdln,		"cut",				edln_cut),
	FN_VOID(edln, WEdln,		"copy",				edln_copy),
	FN_VOID(edln, WEdln,		"complete",			edln_complete),
	FN_VOID(edln, WEdln,		"history_next",		edln_history_next),
	FN_VOID(edln, WEdln,		"history_prev",		edln_history_prev),
	FN_VOID(generic, WEdln,		"paste",			wedln_paste),
	FN_VOID(generic, WEdln,		"finish",			wedln_finish),
	END_FUNTAB
};


WFunclist query_input_funclist=INIT_FUNCLIST;

static WFunction query_input_funtab[]={
	FN_VOID(generic, WInput,	"cancel",			input_cancel),	
	FN_VOID(generic, WInput,	"scrollup",			input_scrollup),
	FN_VOID(generic, WInput,	"scrolldown",		input_scrolldown),
	
	FN_VOID(generic, WInput,	"close",			input_cancel),	
	END_FUNTAB
};



static WFunction query_frame_funtab[]={
	FN(ss, 	generic, WGenFrame,	"query_runfile",	query_runfile),
	FN(ss, 	generic, WGenFrame,	"query_runwith",	query_runwith),
	FN(ss,	generic, WGenFrame,	"query_yesno",		query_yesno),
	FN_VOID(generic, WGenFrame,	"query_function",	query_function),
	FN_VOID(generic, WGenFrame,	"query_exec",		query_exec),
	FN_VOID(generic, WGenFrame,	"query_attachclient", query_attachclient),
	FN_VOID(generic, WGenFrame,	"query_gotoclient", query_gotoclient),
	FN_VOID(generic, WGenFrame,	"query_workspace",	query_workspace),
	FN_VOID(generic, WGenFrame,	"query_workspace_with",	query_workspace_with),
	FN_VOID(generic, WGenFrame,	"query_renameworkspace", query_renameworkspace),
	FN_VOID(generic, WGenFrame,	"query_renameframe", query_renameframe),
	END_FUNTAB
};


/*}}}*/


/*{{{ Config */


static bool query_begin_bindings(Tokenizer *tokz, int n, Token *toks)
{
	return ioncore_begin_bindings(&query_bindmap, NULL);
}


static bool query_begin_edln_bindings(Tokenizer *tokz, int n, Token *toks)
{
	return ioncore_begin_bindings(&query_edln_bindmap, NULL);
}


static ConfOpt query_opts[]={
	{"query_bindings", NULL, query_begin_bindings, ioncore_binding_opts},
	{"edln_bindings", NULL, query_begin_edln_bindings, ioncore_binding_opts},
	END_CONFOPTS
};


/*}}}*/


/*{{{ Init & deinit */


void query_module_deinit()
{
	clear_funclist(&query_input_funclist);
	clear_funclist(&query_edln_funclist);
	deinit_bindmap(&query_bindmap);
	deinit_bindmap(&query_edln_bindmap);
}


bool query_module_init()
{
	bool ret;
	
	ret=(add_to_funclist(&query_input_funclist, query_input_funtab) &&
		 add_to_funclist(&query_edln_funclist, query_edln_funtab));
	
	if(ret)
		ret=read_config_for("query", query_opts);
	
	if(ret)
		ret=add_to_funclist(&ioncore_genframe_funclist, query_frame_funtab);
		
	if(!ret)
		query_module_deinit();
		
	return ret;
}


/*}}}*/

