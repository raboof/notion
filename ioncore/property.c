/*
 * ion/ioncore/property.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <X11/Xmd.h>
#include <string.h>

#include "common.h"
#include "property.h"
#include "global.h"


ulong get_property(Display *dpy, Window win, Atom atom, Atom type,
				   ulong n32expected, bool more, uchar **p)
{
	Atom real_type;
	int format;
	ulong n, extra;
	int status;
	
	do{
		status=XGetWindowProperty(dpy, win, atom, 0L, n32expected, False,
								  type, &real_type, &format, &n, &extra, p);
		
		if(status!=Success || *p==NULL)
			return -1;
	
		if(extra==0 || !more)
			break;
		
		XFree((void*)*p);
		n32expected+=extra;
		more=FALSE;
	}while(1);

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
	
	n=get_property(wglobal.dpy, win, a, XA_STRING, 64L, TRUE, (uchar**)&p);
	
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
	ulong n;
	
	n=get_property(wglobal.dpy, win, a, XA_INTEGER, 1L, FALSE, (uchar**)&p);
	
	if(n>0 && p!=NULL){
		*vret=*p;
		XFree((void*)p);
		return TRUE;
	}
	
	return FALSE;
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
					wglobal.atom_wm_state, 2L, FALSE, (uchar**)&p)<=0)
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


/* get_text_property
 */

char **get_text_property(Window win, Atom a, int *nret)
{
	XTextProperty prop;
	char **list=NULL;
	int n=0;
	Status st=0;
	
	if(nret)
		*nret=0;

	st=XGetTextProperty(wglobal.dpy, win, &prop, a);
	
	if(!st)
		return NULL;

#ifdef CF_XFREE86_TEXTPROP_BUG_WORKAROUND
	while(prop.nitems>0){
		if(prop.value[prop.nitems-1]=='\0')
			prop.nitems--;
		else
			break;
	}
#endif

#ifndef CF_UTF8
	st=XTextPropertyToStringList(&prop, &list, &n);
#else
	st=Xutf8TextPropertyToTextList(wglobal.dpy, &prop, &list, &n);
	st=!st;
#endif
	XFree(prop.value);
	
	if(!st || n==0 || list==NULL)
		return NULL;
	
	if(nret)
			*nret=n;
	return list;
}


void set_text_property(Window win, Atom a, const char *str)
{
	XTextProperty prop;
	const char *ptr[1]={NULL};
	Status st;

	ptr[0]=str;
	
#ifndef CF_UTF8
	st=XStringListToTextProperty((char **)&ptr, 1, &prop);
#else
	st=Xutf8TextListToTextProperty(wglobal.dpy, (char **)&ptr, 1,
								   XUTF8StringStyle, &prop);
	st=!st;
#endif
	if(!st)
		return;
	
	XSetTextProperty(wglobal.dpy, win, &prop, a);
	XFree(prop.value);
}
