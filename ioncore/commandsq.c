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
	
	while(thing!=NULL){
	
		if(tmp_funclist!=NULL)
			func=lookup_func_ex(name, tmp_funclist);
		else
			func=lookup_func_thing(thing, name);
		
		if(func==NULL){
			if(tmp_funclist==NULL)
				break;
			thing=thing->t_parent;
			continue;
		}
	
		if(!check_args_loose(tokz, toks, n, func->argtypes)){
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

	warn("Unknown function '%s'", name);
	
	return FALSE;
}


static ConfOpt command_opts[]={
	{"#default", NULL, opt_default, NULL},
	{NULL, NULL, NULL, NULL}
};


bool execute_command_sequence(WThing *thing, char *fn)
{
	static bool command_sq=FALSE;
	bool retval;
	Tokenizer *tokz;

	if(command_sq){
		warn("Nested command sequence.");
		return FALSE;
	}
	
	command_sq=TRUE;

	setup_watch(&sq_watch, thing, sq_watch_handler);
	
	tokz=tokz_prepare_buffer(fn, -1);
	tokz->flags|=TOKZ_DEFAULT_OPTION;
	retval=parse_config_tokz(tokz, command_opts);
	tokz_close(tokz);

	reset_watch(&sq_watch);
	
	command_sq=FALSE;

	return retval;
}


bool execute_command_sequence_restricted(WThing *thing, char *fn,
										 WFunclist *funclist)
{
	bool ret;
	
	tmp_funclist=funclist;
	ret=execute_command_sequence(thing, fn);
	tmp_funclist=NULL;
	
	return ret;
}
