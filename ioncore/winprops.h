/*
 * wmcore/winprops.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_WINPROPS_H
#define WMCORE_WINPROPS_H

enum TransientMode{
	TRANSIENT_MODE_NORMAL,
	TRANSIENT_MODE_CURRENT,
	TRANSIENT_MODE_OFF
};

INTRSTRUCT(WWinProp)

DECLSTRUCT(WWinProp){
	char *data;
	char *wclass;
	char *winstance;
	WWinProp *next, *prev;
	
	int flags;
	int manage_flags;
	int stubborn;
	int max_w, max_h;
	int aspect_w, aspect_h;
	char *target_name;
	int transient_mode;
};

extern WWinProp *alloc_winprop(const char *winname);
extern void add_winprop(WWinProp *winprop);
extern WWinProp *find_winprop_win(Window win);
extern void free_winprop(WWinProp *winprop);
extern void free_winprops();

#endif /* WMCORE_WINPROPS_H */
