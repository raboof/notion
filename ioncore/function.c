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
#include "objp.h"


/* TODO: sort funtabs and use a more efficient search method */


/*{{{ lookup_func */


WFunction *lookup_func(const char *name, WFunction *func)
{
	if(func==NULL)
		return NULL;
	
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
	
	if(funclist==NULL)
		return NULL;
	
	ITERATE_SYMLIST(WFunction*, func, funclist->funtabs){
		fr=lookup_func(name, func);
		if(fr!=NULL)
			return fr;
	}
	
	return NULL;
}


WFunction *lookup_func_thing(WThing *thing, const char *name)
{
	WObjDescr *descr;
	WFunction *func;
	
	if(thing==NULL)
		return NULL;
	
	descr=thing->obj.obj_type;
	
	while(descr!=NULL){
		func=lookup_func_ex(name, (WFunclist*)descr->funclist);
		if(func!=NULL)
			return func;
		descr=descr->ancestor;
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
		
		if(l && strncmp(name, nam, l))
			continue;
		
		add_to_complist_copy(cp_ret, &n, name);

/*	{
		int i;
		fprintf(stderr, "-->%d [%s]\n", n, name);
		for(i=1; i<n; i++){
			fprintf(stderr, "%s\n", *cp_ret+i);
		}
		fprintf(stderr, "-------------\n");
	}*/
	}
	return n;
}


static int complete_func_ex(const char *nam, char ***cp_ret, char **beg,
							int n, WFunclist *funclist)
{
	WFunction *func;
	
	if(funclist!=NULL){
		ITERATE_SYMLIST(WFunction*, func, funclist->funtabs){
			n=complete_funtab(nam, cp_ret, beg, n, func);
		}
	}

	return n;
}


static int do_complete_func_thing(const char *nam, char ***cp_ret, char **beg,
								  int n, WThing *thing)
{
	WObjDescr *descr;
	
	fprintf(stderr, "%s\n", WOBJ_TYPESTR(thing));
	descr=thing->obj.obj_type;
	
	while(descr!=NULL){
		fprintf(stderr, "->%s\n", descr->name);

		n=complete_func_ex(nam, cp_ret, beg, n,
						   (WFunclist*)descr->funclist);
		descr=descr->ancestor;
	}

	return n;
}


int complete_func_thing(const char *nam, char ***cp_ret, char **beg,
						WThing *thing)
{
	return do_complete_func_thing(nam, cp_ret, beg, 0, thing);
}


int complete_func_thing_parents(const char *nam, char ***cp_ret, char **beg,
								WThing *thing)
{
	int n=0;
	
	while(thing!=NULL){
		n=do_complete_func_thing(nam, cp_ret, beg, n, thing);
		thing=thing->t_parent;
	}
	
	return n;
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
	if(thing!=NULL && wobj_is((WObj*)thing, func->objdescr))
		((Func*)func->fn)(thing);
}


void callhnd_generic_l(WThing *thing, WFunction *func,
					   int n, const Token *args)
{
	typedef void Func(WThing*, int);
	if(thing!=NULL && wobj_is((WObj*)thing, func->objdescr))
		((Func*)func->fn)(thing, TOK_LONG_VAL(args));
}


void callhnd_generic_b(WThing *thing, WFunction *func,
					   int n, const Token *args)
{
	typedef void Func(WThing*, bool);
	if(thing!=NULL && wobj_is((WObj*)thing, func->objdescr))
		((Func*)func->fn)(thing, TOK_BOOL_VAL(args));
}


void callhnd_generic_d(WThing *thing, WFunction *func,
					   int n, const Token *args)
{
	typedef void Func(WThing*, double);
	if(thing!=NULL && wobj_is((WObj*)thing, func->objdescr))
		((Func*)func->fn)(thing, TOK_DOUBLE_VAL(args));
}


void callhnd_generic_s(WThing *thing, WFunction *func,
					   int n, const Token *args)
{
	typedef void Func(WThing*, char*);
	if(thing!=NULL && wobj_is((WObj*)thing, func->objdescr))
		((Func*)func->fn)(thing, TOK_STRING_VAL(args));
}


void callhnd_generic_ss(WThing *thing, WFunction *func,
						int n, const Token *args)
{
	typedef void Func(WThing*, char*, char*);
	if(thing!=NULL && wobj_is((WObj*)thing, func->objdescr))
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
