/*
 * wmcore/commandsq.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <libtu/parser.h>
#include <libtu/tokenizer.h>
#include <libtu/output.h>

#include "common.h"
#include "global.h"


static WWatch sq_watch=WWATCH_INIT;
static WFunclist *tmp_funclist=NULL;

/* We don't want to refer to destroyed things. */
static void sq_watch_handler(WWatch *watch, WThing *t)
{
	WThing *t2=t->t_parent;
	
	if(t2!=NULL)
		setup_watch(watch, t2, sq_watch_handler);
}


static bool opt_default(Tokenizer *tokz, int n, Token *toks)
{
	WThing *thing=sq_watch.thing;
	WFunction *func;
	char *name=TOK_IDENT_VAL(&(toks[0]));
	
	if(thing==NULL)
		return FALSE;
	
	func=lookup_func_ex(name, tmp_funclist);
	
	if(func==NULL){
		warn("Unknown function '%s' or not in ion_main_funclist.", name);
		return FALSE;
	}
	
	if(!check_args(tokz, toks, n, func->argtypes)){
		warn("Argument check for function '%s' failed. Prototype is '%s'.",
			 name, func->argtypes);
		return FALSE;
	}
	
	func->callhnd(thing, func, n-1, &(toks[1]));
	
	if(wglobal.focus_next!=NULL)
		setup_watch(&sq_watch, (WThing*)wglobal.focus_next,
					sq_watch_handler);
	
	return TRUE;
}


static ConfOpt command_opts[]={
	{"#default", NULL, opt_default, NULL},
	{NULL, NULL, NULL, NULL}
};


bool execute_command_sequence(WThing *thing, char *fn, WFunclist *funclist)
{
	static bool command_sq=FALSE;
	bool retval;
	Tokenizer *tokz;

	if(funclist==NULL)
		return FALSE;
	
	if(command_sq){
		warn("Nested command sequence.");
		return FALSE;
	}
	
	command_sq=TRUE;
	tmp_funclist=funclist;

	setup_watch(&sq_watch, thing, sq_watch_handler);
	
	tokz=tokz_prepare_buffer(fn, -1);
	tokz->flags|=TOKZ_DEFAULT_OPTION;
	retval=parse_config_tokz(tokz, command_opts);
	tokz_close(tokz);

	reset_watch(&sq_watch);
	
	tmp_funclist=NULL;
	command_sq=FALSE;

	return retval;
}

