/*
 * ion/ioncore/function.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_FUNCTION_H
#define ION_IONCORE_FUNCTION_H

#include <libtu/tokenizer.h>
#include "common.h"
#include "symlist.h"
#include "hooks.h"
#include "region.h"

INTRSTRUCT(WFunction)
INTRSTRUCT(WFunclist)

typedef void WFuncHandler(WObj *obj, WFunction *func,
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
extern WFunction *lookup_func_obj(WObj *obj, const char *name);
extern bool add_to_funclist(WFunclist *funclist, WFunction *funtab);
extern void remove_from_funclist(WFunclist *funclist, WFunction *funtab);
extern void clear_funclist(WFunclist *funclist);
extern int complete_func_obj(const char *nam, char ***cp_ret, char **beg,
							 WObj *obj);
extern int complete_func_reg_mgrs(const char *nam, char ***cp_ret,
								  char **beg, WRegion *reg);

#endif /* ION_IONCORE_FUNCTION_H */
