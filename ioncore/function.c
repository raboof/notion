/*
 * ion/ioncore/function.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
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


WFunction *lookup_func_obj(WObj *obj, const char *name)
{
	WObjDescr *descr;
	WFunction *func;
	
	if(obj==NULL)
		return NULL;
	
	descr=obj->obj_type;
	
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


static int do_complete_func_obj(const char *nam, char ***cp_ret, char **beg,
								int n, WObj *obj)
{
	WObjDescr *descr;
	
	descr=obj->obj_type;
	
	while(descr!=NULL){
		n=complete_func_ex(nam, cp_ret, beg, n,
						   (WFunclist*)descr->funclist);
		descr=descr->ancestor;
	}

	return n;
}


int complete_func_obj(const char *nam, char ***cp_ret, char **beg,
					  WObj *obj)
{
	return do_complete_func_obj(nam, cp_ret, beg, 0, obj);
}


int complete_func_reg_mgrs(const char *nam, char ***cp_ret, char **beg,
						   WRegion *reg)
{
	int n=0;
	
	while(reg!=NULL){
		n=do_complete_func_obj(nam, cp_ret, beg, n, (WObj*)reg);
		reg=REGION_MANAGER(reg);
	}
	
	return n;
}


/*}}}*/


/*{{{ Call handlers */


void callhnd_direct(WObj *obj, WFunction *func,
					int n, const Token *args)
{
	typedef void Func(WObj*, int, const Token*);
	((Func*)func->fn)(obj, n, args);
}


void callhnd_generic_void(WObj *obj, WFunction *func,
						  int n, const Token *args)
{
	typedef void Func(WObj*);
	if(obj!=NULL && wobj_is((WObj*)obj, func->objdescr))
		((Func*)func->fn)(obj);
}


void callhnd_generic_l(WObj *obj, WFunction *func,
					   int n, const Token *args)
{
	typedef void Func(WObj*, int);
	if(obj!=NULL && wobj_is((WObj*)obj, func->objdescr))
		((Func*)func->fn)(obj, TOK_LONG_VAL(args));
}


void callhnd_generic_b(WObj *obj, WFunction *func,
					   int n, const Token *args)
{
	typedef void Func(WObj*, bool);
	if(obj!=NULL && wobj_is((WObj*)obj, func->objdescr))
		((Func*)func->fn)(obj, TOK_BOOL_VAL(args));
}


void callhnd_generic_d(WObj *obj, WFunction *func,
					   int n, const Token *args)
{
	typedef void Func(WObj*, double);
	if(obj!=NULL && wobj_is((WObj*)obj, func->objdescr))
		((Func*)func->fn)(obj, TOK_DOUBLE_VAL(args));
}


void callhnd_generic_s(WObj *obj, WFunction *func,
					   int n, const Token *args)
{
	typedef void Func(WObj*, char*);
	if(obj!=NULL && wobj_is((WObj*)obj, func->objdescr))
		((Func*)func->fn)(obj, TOK_STRING_VAL(args));
}


void callhnd_generic_ss(WObj *obj, WFunction *func,
						int n, const Token *args)
{
	typedef void Func(WObj*, char*, char*);
	if(obj!=NULL && wobj_is((WObj*)obj, func->objdescr))
		((Func*)func->fn)(obj, TOK_STRING_VAL(args), TOK_STRING_VAL(args+1));
}


void callhnd_global_void(WObj *obj, WFunction *func,
						 int n, const Token *args)
{
	typedef void Func();
	((Func*)func->fn)();
}


void callhnd_global_l(WObj *obj, WFunction *func,
					  int n, const Token *args)
{
	typedef void Func(int);
	((Func*)func->fn)(TOK_LONG_VAL(args));
}


void callhnd_global_ll(WObj *obj, WFunction *func,
					   int n, const Token *args)
{
	typedef void Func(int, int);
	((Func*)func->fn)(TOK_LONG_VAL(args), TOK_LONG_VAL(args+1));
}


void callhnd_global_s(WObj *obj, WFunction *func,
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
		remove_from_funclist(funclist, (WFunction*)funclist->funtabs->symbol);
}


/*}}}*/
