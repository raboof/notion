/*
 * ion/ioncore/property.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <X11/Xmd.h>
#include <string.h>

#include "common.h"
#include "property.h"
#include "global.h"


int get_property(Display *dpy, Window win, Atom atom, Atom type,
				 long len, uchar **p)
{
	Atom real_type;
	int format;
	ulong n, extra;
	int status;
	
	status=XGetWindowProperty(dpy, win, atom, 0L, len, False,
							  type, &real_type, &format, &n, &extra, p);
	
	if(status!=Success || *p==NULL)
		return -1;
	
	if(n==0){
		XFree((void*)*p);
		return -1;
	}
	
	return n;
}


/* string
 */

char *get_string_property(Window win, Atom a, int *nret)
{
	char *p;
	int n;
	
	n=get_property(wglobal.dpy, win, a, XA_STRING, 100L, (uchar**)&p);
	
	if(nret!=NULL)
		*nret=n;
	
	return (n<=0 ? NULL : p);
}


void set_string_property(Window win, Atom a, const char *value)
{
	if(value==NULL){
		XDeleteProperty(wglobal.dpy, win, a);
	}else{
		XChangeProperty(wglobal.dpy, win, a, XA_STRING,
						8, PropModeReplace, (uchar*)value, strlen(value));
	}
}


/* integer
 */

bool get_integer_property(Window win, Atom a, int *vret)
{
	CARD32 *p=NULL;
	
	if(get_property(wglobal.dpy, win, a, XA_INTEGER, 1L, (uchar**)&p)<=0)
		return FALSE;
	
	*vret=(CARD32)*p;
	
	XFree((void*)p);
	
	return TRUE;
}


void set_integer_property(Window win, Atom a, int value)
{
	CARD32 data[2];
	
	data[0]=value;
	
	XChangeProperty(wglobal.dpy, win, a, XA_INTEGER,
					32, PropModeReplace, (uchar*)data, 1);
}


/* WM_STATE
 */

bool get_win_state(Window win, int *state)
{
	CARD32 *p=NULL;
	
	if(get_property(wglobal.dpy, win, wglobal.atom_wm_state,
					wglobal.atom_wm_state, 2L, (uchar**)&p)<=0)
		return FALSE;
	
	*state=(CARD32)*p;
	
	XFree((void*)p);
	
	return TRUE;
}


void set_win_state(Window win, int state)
{
	CARD32 data[2];
	
	data[0]=state;
	data[1]=None;
	
	XChangeProperty(wglobal.dpy, win,
					wglobal.atom_wm_state, wglobal.atom_wm_state,
					32, PropModeReplace, (uchar*)data, 2);
}

