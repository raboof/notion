/*
 * ion/ioncore/mwmhints.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "common.h"
#include "property.h"
#include "mwmhints.h"
#include "global.h"


WMwmHints *get_mwm_hints(Window win)
{
	WMwmHints *hints=NULL;
	int n;
	
	n=get_property(wglobal.dpy, win, wglobal.atom_mwm_hints,
				   wglobal.atom_mwm_hints, MWM_N_HINTS, FALSE, (uchar**)&hints);
	
	if(n<MWM_N_HINTS && hints!=NULL){
		XFree((void*)hints);
		return NULL;
	}
	
	return hints;
}


void check_mwm_hints_nodecor(Window win, bool *nodecor)
{
	WMwmHints *hints;
	int n;
	
	n=get_property(wglobal.dpy, win, wglobal.atom_mwm_hints,
				   wglobal.atom_mwm_hints, MWM_N_HINTS, FALSE, (uchar**)&hints);
	
	if(n<MWM_DECOR_NDX)
		return;
	
	if(hints->flags&MWM_HINTS_DECORATIONS &&
	   (hints->decorations&MWM_DECOR_ALL)==0){
		*nodecor=TRUE;
		
		if(hints->decorations&MWM_DECOR_BORDER ||
		   hints->decorations&MWM_DECOR_TITLE)
			*nodecor=FALSE;
	}
	
	XFree((void*)hints);
}
