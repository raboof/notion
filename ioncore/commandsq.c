/*
 * ion/ioncore/commandsq.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <libtu/parser.h>
#include <libtu/tokenizer.h>
#include <libtu/output.h>

#include "common.h"
#include "global.h"
#include "objp.h"
#include "region.h"

static WWatch *sq_watch=NULL;
static WFunclist *tmp_funclist=NULL;
static WRegion *commands_at_next=NULL;

/* We don't want to refer to destroyed things. */
static void sq_watch_handler(WWatch *watch, WThing *t)
{
	WRegion *reg;
	
	assert(WTHING_IS(t, WRegion));
	
	reg=REGION_MANAGER((WRegion*)t);
	
	if(reg!=NULL)
		setup_watch(watch, (WThing*)reg, sq_watch_handler);
}


void commands_at_leaf()
{
	WRegion *reg;
	
	assert(sq_watch->thing!=NULL);
	
	if(!WTHING_IS(sq_watch->thing, WRegion))
		return;
	
	reg=(WRegion*)sq_watch->thing;
	
	while(reg->active_sub!=NULL)
			reg=reg->active_sub;
	
	commands_at_next=reg;
}


static bool opt_default(Tokenizer *tokz, int n, Token *toks)
{
	WRegion *reg=(WRegion*)sq_watch->thing;
	WFunction *func;
	char *name=TOK_IDENT_VAL(&(toks[0]));
	
	while(reg!=NULL){
		if(tmp_funclist!=NULL)
			func=lookup_func_ex(name, tmp_funclist);
		else
			func=lookup_func_thing((WThing*)reg, name);
		
		if(func==NULL){
			if(tmp_funclist!=NULL)
				break;
			reg=REGION_MANAGER(reg);
			continue;
		}
	
		if(!check_args_loose(tokz, toks, n, func->argtypes)){
			warn("Argument check for function '%s' failed. Prototype is '%s'.",
				 name, func->argtypes);
			return FALSE;
		}
	
		func->callhnd((WThing*)reg, func, n-1, &(toks[1]));
	
		if(commands_at_next!=NULL){
			setup_watch(sq_watch, (WThing*)commands_at_next,
						sq_watch_handler);
			commands_at_next=NULL;
		}else if(wglobal.focus_next!=NULL){
			setup_watch(sq_watch, (WThing*)wglobal.focus_next,
						sq_watch_handler);
		}

		return TRUE;
	}

	warn("Unknown function '%s'", name);
	
	return FALSE;
}


static ConfOpt command_opts[]={
	{"#default", NULL, opt_default, NULL},
	{NULL, NULL, NULL, NULL}
};


bool execute_command_sequence(WRegion *reg, char *fn)
{
	static int command_sq=0;
	bool retval;
	Tokenizer *tokz;
	WWatch watch=WWATCH_INIT;
	WWatch *old_watch=sq_watch;
	
	if(command_sq>=CF_MAX_COMMAND_SQ_NEST){
		warn("Too many nested command sequences.");
		return FALSE;
	}

	command_sq++;

	setup_watch(&watch, (WThing*)reg, sq_watch_handler);
	sq_watch=&watch;
	
	tokz=tokz_prepare_buffer(fn, -1);
	tokz->flags|=TOKZ_DEFAULT_OPTION;
	retval=parse_config_tokz(tokz, command_opts);
	tokz_close(tokz);

	sq_watch=old_watch;
	reset_watch(&watch);
	
	command_sq--;

	return retval;
}


bool execute_command_sequence_restricted(WRegion *reg, char *fn,
										 WFunclist *funclist)
{
	bool ret;
	
	WFunclist *old_tmp=tmp_funclist;
	tmp_funclist=funclist;
	
	ret=execute_command_sequence(reg, fn);
	
	tmp_funclist=old_tmp;
	
	return ret;
}
