/*
 * ion/ioncore/winprops.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_WINPROPS_H
#define ION_IONCORE_WINPROPS_H

#include "common.h"
#include "extl.h"

INTRSTRUCT(WWinProp);

DECLSTRUCT(WWinProp){
	char *wclass;
	char *wrole;
	char *winstance;
	ExtlTab proptab;
	WWinProp *next, *prev;
};

void add_winprop(const char *cls, const char *role, const char *inst,
				 ExtlTab tab);
extern ExtlTab find_winproptab_win(Window win);
extern void free_winprops();

#endif /* ION_IONCORE_WINPROPS_H */
