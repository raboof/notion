/*
 * ion/ioncore/selection.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <X11/Xmd.h>
#include <string.h>

#include "common.h"
#include "global.h"
#include "property.h"
#include "xwindow.h"
#include <libextl/extl.h>


static char *selection_data=NULL;
static int selection_length;
static bool continuation_set=FALSE;
static ExtlFn continuation;

void ioncore_handle_selection_request(XSelectionRequestEvent *ev)
{
    XSelectionEvent sev;
    const char *p[1];
    
    if(selection_data==NULL)
        return;
    
    p[0]=selection_data;
    
    xwindow_set_text_property(ev->requestor, ev->property, p, 1);
    
    sev.type=SelectionNotify;
    sev.requestor=ev->requestor;
    sev.selection=ev->selection;
    sev.target=ev->target;
    sev.time=ev->time;
    sev.property=ev->property;
    XSendEvent(ioncore_g.dpy, ev->requestor, False, 0L, (XEvent*)&sev);
}


static void ins(Window win, const char *str, int n)
{
    if(!continuation_set){
        WWindow *wwin=XWINDOW_REGION_OF_T(win, WWindow);
        if(wwin)
            window_insstr(wwin, str, n);
    }else{
        char *tmp=scopyn(str, n);
        if(tmp!=NULL){
            extl_call(continuation, "s", NULL, tmp);
            free(tmp);
        }
    }
}

    
static void insert_selection(Window win, Atom prop)
{
    char **p=xwindow_get_text_property(win, prop, NULL);
    if(p!=NULL){
        ins(win, p[0], strlen(p[0]));
        XFreeStringList(p);
    }
}


static void insert_cutbuffer(Window win)
{
    char *p;
    int n;
    
    p=XFetchBytes(ioncore_g.dpy, &n);
    
    if(n<=0 || p==NULL)
        return;
    
    ins(win, p, n);
}


void ioncore_handle_selection(XSelectionEvent *ev)
{
    Atom prop=ev->property;
    Window win=ev->requestor;
    WWindow *wwin;
    
    if(prop==None){
        insert_cutbuffer(win);
    }else{
        insert_selection(win, prop);
        XDeleteProperty(ioncore_g.dpy, win, prop);
    }
    
    if(continuation_set){
        extl_unref_fn(continuation);
        continuation_set=FALSE;
    }
}


void ioncore_clear_selection()
{
    if(selection_data!=NULL){
        free(selection_data);
        selection_data=NULL;
    }
}


void ioncore_set_selection_n(const char *p, int n)
{
    if(selection_data!=NULL)
        free(selection_data);
    
    selection_data=ALLOC_N(char, n+1);
    
    if(selection_data==NULL)
        return;
    
    memcpy(selection_data, p, n);
    selection_data[n]='\0';
    selection_length=n;
    
    XStoreBytes(ioncore_g.dpy, p, n);
    
    XSetSelectionOwner(ioncore_g.dpy, XA_PRIMARY,
                       DefaultRootWindow(ioncore_g.dpy),
                       CurrentTime);
}


/*EXTL_DOC
 * Set primary selection and cutbuffer0 to \var{p}.
 */
EXTL_EXPORT
void ioncore_set_selection(const char *p)
{
    if(p==NULL)
        ioncore_clear_selection();
    else
        ioncore_set_selection_n(p, strlen(p));
}


void ioncore_request_selection_for(Window win)
{
    Atom a=XA_STRING;
    
    if(continuation_set){
        extl_unref_fn(continuation);
        continuation_set=FALSE;
    }
    
    if(ioncore_g.use_mb){
#ifdef X_HAVE_UTF8_STRING
        a=XInternAtom(ioncore_g.dpy, "UTF8_STRING", True);
#else
        a=XInternAtom(ioncore_g.dpy, "COMPOUND_TEXT", True);
#endif
    }
    
    XConvertSelection(ioncore_g.dpy, XA_PRIMARY, a,
                      ioncore_g.atom_selection, win, CurrentTime);
}


/*EXTL_DOC
 * Request (string) selection. The function \var{fn} will be called 
 * with the selection when and if it is received.
 */
EXTL_EXPORT
void ioncore_request_selection(ExtlFn fn)
{
    assert(ioncore_g.rootwins!=NULL);
    ioncore_request_selection_for(ioncore_g.rootwins->dummy_win);
    continuation=extl_ref_fn(fn);
    continuation_set=TRUE;
}

