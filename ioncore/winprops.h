/*
 * ion/ioncore/winprops.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_WINPROPS_H
#define ION_IONCORE_WINPROPS_H

enum TransientMode{
	TRANSIENT_MODE_NORMAL,
	TRANSIENT_MODE_CURRENT,
	TRANSIENT_MODE_OFF
};

INTRSTRUCT(WWinProp)

DECLSTRUCT(WWinProp){
	char *wclass;
	char *wrole;
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

extern WWinProp *alloc_winprop(const char *cls, const char *role,
							   const char *instance);
extern void add_winprop(WWinProp *winprop);
extern WWinProp *find_winprop_win(Window win);
extern void free_winprop(WWinProp *winprop);
extern void free_winprops();

#endif /* ION_IONCORE_WINPROPS_H */
