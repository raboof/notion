/*
 * wmcore/function.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_FUNCTION_H
#define WMCORE_FUNCTION_H

#include <libtu/tokenizer.h>
#include "common.h"
#include "thing.h"
#include "symlist.h"
#include "hooks.h"

INTRSTRUCT(WFunction)
INTRSTRUCT(WFunclist)

typedef void WFuncHandler(WThing *thing, WFunction *func,
						  int n, const Token *args);


DECLSTRUCT(WFunction){
	WFuncHandler *callhnd;
	WObjDescr *objdescr;
	char *argtypes;
	char *name;
	void *fn;
};


DECLSTRUCT(WFunclist){
	WSymlist *funtabs;
	WHooklist *funtab_removed_hook;
};

#define INIT_FUNCLIST {NULL, NULL}


extern WFunction *lookup_func(const char *name, WFunction *func);
extern WFunction *lookup_func_ex(const char *name, WFunclist *funclist);
extern int complete_func(const char *nam, char ***cp_ret, char **beg,
							WFunction *func);
extern int complete_func_ex(const char *nam, char ***cp_ret, char **beg,
							WFunclist *funclist);

extern bool add_to_funclist(WFunclist *funclist, WFunction *funtab);
extern void remove_from_funclist(WFunclist *funclist, WFunction *funtab);
extern void clear_funclist(WFunclist *funclist);

#endif /* WMCORE_FUNCTION_H */
