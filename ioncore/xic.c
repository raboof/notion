/*
 * ion/ioncore/xic.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <ctype.h>
#include <string.h>
#include "common.h"
#include "global.h"

/*#define X_LOCALE*/
#include <X11/Xlocale.h>


static XIM input_method=NULL;
static XIMStyle input_style=(XIMPreeditNothing|XIMStatusNothing);


static void init_xlocale(void)
{
	char *p;
	int i;
    XIM xim=NULL;
    XIMStyles *xim_styles = NULL;
  	bool found=FALSE;

	if((p=XSetLocaleModifiers(""))!=NULL && *p)
	    xim=XOpenIM(wglobal.dpy, NULL, NULL, NULL);
	
	if(xim==NULL && (p=XSetLocaleModifiers("@im=none"))!=NULL && *p)
	    xim=XOpenIM(wglobal.dpy, NULL, NULL, NULL);

    if(xim==NULL){
		warn("Failed to open input method");
		return;
    }
	
    if(XGetIMValues(xim, XNQueryInputStyle, &xim_styles, NULL) || !xim_styles) {
		warn("input method doesn't support any style");
		XCloseIM(xim);
		return;
    }
	
	for(i=0; (ushort)i<xim_styles->count_styles; i++){
		if(input_style==xim_styles->supported_styles[i]){
			found=TRUE;
			break;
		}
	}
	
    XFree(xim_styles);
	
    if(!found){
		warn("input method doesn't support my preedit type");
		XCloseIM(xim);
		return;
    }

	input_method=xim;	
}


XIC create_xic(Window win)
{
	static bool tried=FALSE;
	XIC xic;
	
	if(input_method==NULL && !tried){
		init_xlocale();
		tried=TRUE;
	}
	
	if(input_method==NULL)
		return NULL;
	
    xic=XCreateIC(input_method, XNInputStyle, input_style,
				  XNClientWindow, win, XNFocusWindow, win,
				  NULL);
	
    if(xic==NULL)
		warn("Failed to create input context");

	return xic;
}

