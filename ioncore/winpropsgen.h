/*
 * wmcore/winpropsgen.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_WINPROPSGEN_H
#define WMCORE_WINPROPSGEN_H

INTRSTRUCT(WWinPropGen)

DECLSTRUCT(WWinPropGen){
	char *data;
	char *wclass;
	char *winstance;
	WWinPropGen *next, *prev;
};

extern WWinPropGen *find_winpropgen(WWinPropGen *list,
								 const char *wclass, const char *winstance);
extern WWinPropGen *find_winpropgen_win(WWinPropGen *list, Window win);
extern void free_winpropgen(WWinPropGen **list, WWinPropGen *winprop);
extern void add_winpropgen(WWinPropGen **list, WWinPropGen *winprop);
extern bool init_winpropgen(WWinPropGen *winprop, const char *name);
							
#endif /* WMCORE_WINPROPSGEN_H */
