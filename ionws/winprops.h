/*
 * ion/winprops.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef ION_WINPROPS_H
#define ION_WINPROPS_H

#include <libtu/parser.h>
#include <wmcore/common.h>
#include <wmcore/winpropsgen.h>

INTRSTRUCT(WWinProp)

DECLSTRUCT(WWinProp){
	WWinPropGen genprop;

	int flags;
	int switchto;
	int stubborn;
	int max_w, max_h;
	int aspect_w, aspect_h;
};

extern WWinProp *alloc_winprop(const char *winname);
extern WWinProp *find_winprop_win(Window win);
extern void free_winprop(WWinProp *winprop);
extern void free_winprops();

extern bool ion_begin_winprop(Tokenizer *tokz, int n, Token *toks);
extern ConfOpt ion_winprop_opts[];

#endif /* ION_WINPROPS_H */
