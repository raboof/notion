/*
 * wmcore/function.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <string.h>
#include "common.h"
#include "functionp.h"
#include "modules.h"
#include "completehelp.h"


/*{{{ lookup_func */


WFunction *lookup_func(const char *name, WFunction *func)
{
	while(func->callhnd!=NULL){
		if(strcmp(func->name, name)==0)
			return func;
		func++;
	}
	return NULL;
}


WFunction *lookup_func_ex(const char *name, WFunclist *funclist)
{
	WFunction *func, *fr;
	
	ITERATE_SYMLIST(WFunction*, func, funclist->funtabs){
		fr=lookup_func(name, func);
		if(fr!=NULL)
			return fr;
	}
	
	return NULL;
}


static int complete_funtab(const char *nam, char ***cp_ret, char **beg,
						   int n, WFunction *func)
{
	char *name;
	char **cp;
	int l=strlen(nam);
	
	for(; func->callhnd!=NULL; func++){
		
		if((name=func->name)==NULL)
			continue;
		
		/*if(func->argtypes!=NULL && strcmp(func->argtypes, ""))
			continue;*/
		
		if(l && strncmp(name, nam, l))
			continue;
		
		add_to_complist_copy(cp_ret, &n, name);
	}
	
	return n;
}


int complete_func_ex(const char *nam, char ***cp_ret, char **beg,
					 WFunclist *funclist)
{
	WFunction *func;
	int n=0;
	
	*cp_ret=NULL;

	ITERATE_SYMLIST(WFunction*, func, funclist->funtabs){
		n=complete_funtab(nam, cp_ret, beg, n, func);
	}
	
	return n;
}


int complete_func(const char *nam, char ***cp_ret, char **beg,
				  WFunction *func)
{
	*cp_ret=NULL;
	return complete_funtab(nam, cp_ret, beg, 0, func);
}


/*}}}*/


/*{{{ Call handlers */


void callhnd_direct(WThing *thing, WFunction *func,
					int n, const Token *args)
{
	typedef void Func(WThing*, int, const Token*);
	((Func*)func->fn)(thing, n, args);
}


void callhnd_generic_void(WThing *thing, WFunction *func,
						  int n, const Token *args)
{
	typedef void Func(WThing*);
	thing=find_parent(thing, func->objdescr);
	if(thing!=NULL)
		((Func*)func->fn)(thing);
}


void callhnd_generic_l(WThing *thing, WFunction *func,
					   int n, const Token *args)
{
	typedef void Func(WThing*, int);
	thing=find_parent(thing, func->objdescr);
	if(thing!=NULL)
		((Func*)func->fn)(thing, TOK_LONG_VAL(args));
}


void callhnd_generic_d(WThing *thing, WFunction *func,
					   int n, const Token *args)
{
	typedef void Func(WThing*, double);
	thing=find_parent(thing, func->objdescr);
	if(thing!=NULL)
		((Func*)func->fn)(thing, TOK_DOUBLE_VAL(args));
}


void callhnd_generic_s(WThing *thing, WFunction *func,
					   int n, const Token *args)
{
	typedef void Func(WThing*, char*);
	thing=find_parent(thing, func->objdescr);
	if(thing!=NULL)
		((Func*)func->fn)(thing, TOK_STRING_VAL(args));
}


void callhnd_generic_ss(WThing *thing, WFunction *func,
						int n, const Token *args)
{
	typedef void Func(WThing*, char*, char*);
	thing=find_parent(thing, func->objdescr);
	if(thing!=NULL)
		((Func*)func->fn)(thing, TOK_STRING_VAL(args), TOK_STRING_VAL(args+1));
}


void callhnd_global_void(WThing *thing, WFunction *func,
						 int n, const Token *args)
{
	typedef void Func();
	((Func*)func->fn)();
}


void callhnd_global_l(WThing *thing, WFunction *func,
					  int n, const Token *args)
{
	typedef void Func(int);
	((Func*)func->fn)(TOK_LONG_VAL(args));
}


void callhnd_global_ll(WThing *thing, WFunction *func,
					   int n, const Token *args)
{
	typedef void Func(int, int);
	((Func*)func->fn)(TOK_LONG_VAL(args), TOK_LONG_VAL(args+1));
}


void callhnd_global_s(WThing *thing, WFunction *func,
					  int n, const Token *args)
{
	typedef void Func(char*);
	((Func*)func->fn)(TOK_STRING_VAL(args));
}


/*}}}*/


/*{{{ add/remove */


bool add_to_funclist(WFunclist *funclist, WFunction *funtab)
{
	return add_to_symlist(&(funclist->funtabs), (void*)funtab);
}


void remove_from_funclist(WFunclist *funclist, WFunction *funtab)
{
	remove_from_symlist(&(funclist->funtabs), (void*)funtab);
	CALL_HOOKS(funclist->funtab_removed_hook, (funtab));
}


void clear_funclist(WFunclist *funclist)
{
	while(funclist->funtabs!=NULL)
		remove_from_funclist(funclist, (WFunction*)funclist->funtabs);
}


/*}}}*/
