/*
 * ion/ioncore/binding.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_BINDING_H
#define ION_IONCORE_BINDING_H

#include <libtu/tokenizer.h>

#include "common.h"
#include "function.h"
#include "obj.h"
#include "region.h"


#define ACT_KEYPRESS 		0
#define ACT_BUTTONPRESS		1
#define ACT_BUTTONMOTION	2
#define ACT_BUTTONCLICK		3
#define ACT_BUTTONDBLCLICK	4

#define BINDING_MAXARGS		3
#define BINDING_INIT		\
	{0, 0, 0, 0, FALSE, NULL, NULL}
/*	{0, 0, 0, 0, FALSE, NULL, NULL, 0, {TOK_INIT, TOK_INIT, TOK_INIT}}*/
#define BINDMAP_INIT		{0, 0, NULL, NULL, NULL}


INTRSTRUCT(WBinding)
INTRSTRUCT(WBindingSimple)
INTRSTRUCT(WBindmap)
INTRSTRUCT(WRegBindingInfo)


DECLSTRUCT(WBinding){
	uint kcb; /* keycode or button */
	uint state;
	uint act;
	int area;
	bool waitrel;
	WBindmap *submap;
	char *cmd;
};



DECLSTRUCT(WRegBindingInfo){
	WBindmap *bindmap;
	WRegBindingInfo *next, *prev;
	WRegBindingInfo *bm_next, *bm_prev;
	WRegion *reg;
	WRegion *owner;
};



DECLSTRUCT(WBindingSimple){
	WFunction *func;
	int nargs;
	Token args[BINDING_MAXARGS];
};


DECLSTRUCT(WBindmap){
	int nbindings;
	uint confdefmod;
	WBinding *bindings;
	WBindmap *parent;
	WRegBindingInfo *rbind_list;
};


extern void init_bindings();
extern WBindmap *create_bindmap();
extern void deinit_bindmap(WBindmap *bindmap);
extern void deinit_binding(WBinding *binding);
extern bool add_binding(WBindmap *bindmap, const WBinding *binding);
extern WBinding *lookup_binding(WBindmap *bindmap, int act,
								uint state, uint kcb);
extern WBinding *lookup_binding_area(WBindmap *bindmap, int act,
									 uint state, uint kcb, int area);
extern void grab_binding(const WBinding *binding, Window win);
extern void ungrab_binding(const WBinding *binding, Window win);
/*extern void grab_bindings(WBindmap *bindmap, Window win);*/
extern void call_binding(const WBinding *binding, WRegion *reg);
extern void call_binding_restricted(const WBinding *binding, WRegion *reg,
									WFunclist *funclist);
extern int unmod(int state, int keycode);
extern bool ismod(int keycode);
extern void update_modmap();

#endif /* ION_IONCORE_BINDING_H */
