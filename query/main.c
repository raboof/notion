/*
 * ion/query/main.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <wmcore/binding.h>
#include <wmcore/conf-bindings.h>
#include <wmcore/functionp.h>
#include <wmcore/readconfig.h>
#include <src/funtabs.h>

#include "query.h"
#include "edln.h"
#include "wedln.h"
#include "input.h"
#include "complete.h"


WBindmap *query_bindmap=NULL;
WBindmap *query_edln_bindmap=NULL;


/*{{{ Bindable functions */


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


static WFunclist query_edln_funclist=INIT_FUNCLIST;

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


static WFunclist query_funclist=INIT_FUNCLIST;

static WFunction query_funtab[]={
	FN_VOID(generic, WInput,	"cancel",			input_cancel),	
	FN_VOID(generic, WInput,	"scrollup",			input_scrollup),
	FN_VOID(generic, WInput,	"scrolldown",		input_scrolldown),
	END_FUNTAB
};



static WFunction query_frame_funtab[]={
	FN(ss, 	generic, WFrame,	"query_runfile",	query_runfile),
	FN(ss, 	generic, WFrame,	"query_runwith",	query_runwith),
	FN(ss,	generic, WFrame,	"query_yesno",		query_yesno),
	FN_VOID(generic, WFrame,	"query_function",	query_function),
	FN_VOID(generic, WFrame,	"query_exec",		query_exec),
	FN_VOID(generic, WFrame,	"query_attachclient", query_attachclient),
	FN_VOID(generic, WFrame,	"query_gotoclient", query_gotoclient),
	FN_VOID(generic, WFrame,	"query_workspace",	query_workspace),
	FN_VOID(generic, WFrame,	"query_workspace_with",	query_workspace_with),
	FN_VOID(generic, WFrame,	"query_renameworkspace", query_renameworkspace),
	FN_VOID(generic, WFrame,	"query_renameframe", query_renameframe),
	END_FUNTAB
};


/*}}}*/


/*{{{ Config */


static bool query_begin_bindings(Tokenizer *tokz, int n, Token *toks)
{
	return wmcore_begin_bindings(query_bindmap, &query_funclist, NULL);
}


static bool query_begin_edln_bindings(Tokenizer *tokz, int n, Token *toks)
{
	return wmcore_begin_bindings(query_edln_bindmap, &query_edln_funclist,
								 NULL);
}


static ConfOpt query_opts[]={
	{"query_bindings", NULL, query_begin_bindings, wmcore_binding_opts},
	{"edln_bindings", NULL, query_begin_edln_bindings, wmcore_binding_opts},
	END_CONFOPTS
};


/*}}}*/


/*{{{ Init & deinit */


void query_deinit();


bool query_init()
{
	bool ret;
	
	query_bindmap=create_bindmap();
	
	if(query_bindmap==NULL)
		return FALSE;

	query_edln_bindmap=create_bindmap();
	if(query_edln_bindmap==NULL){
		free(query_bindmap);
		query_bindmap=NULL;
		return FALSE;
	}

	ret=(add_to_funclist(&query_funclist, query_funtab) &&
		 add_to_funclist(&query_edln_funclist, query_edln_funtab));
	
	if(ret)
		ret=read_config_for("query", query_opts);
	
	if(ret)
		ret=add_to_funclist(&ion_frame_funclist, query_frame_funtab);
		
	if(!ret)
		query_deinit();
		
	return ret;
}


void query_deinit()
{
	clear_funclist(&query_funclist);
	clear_funclist(&query_edln_funclist);
	remove_from_funclist(&ion_frame_funclist, query_frame_funtab);
	
	if(query_bindmap!=NULL){
		destroy_bindmap(query_bindmap);
		query_bindmap=NULL;
	}
}


/*}}}*/

