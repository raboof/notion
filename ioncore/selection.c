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


static char *selection_data=NULL;
static int selection_length;

void ioncore_handle_selection_request(XSelectionRequestEvent *ev)
{
    XSelectionEvent sev;
    
    if(selection_data==NULL)
        return;
    
    xwindow_set_text_property(ev->requestor, ev->property, selection_data);
    
    sev.type=SelectionNotify;
    sev.requestor=ev->requestor;
    sev.selection=ev->selection;
    sev.target=ev->target;
    sev.time=ev->time;
    sev.property=ev->property;
    XSendEvent(ioncore_g.dpy, ev->requestor, False, 0L, (XEvent*)&sev);
}


static void insert_selection(WWindow *wwin, Window win, Atom prop)
{
    char **p=xwindow_get_text_property(win, prop, NULL);
    if(p!=NULL){
        window_insstr(wwin, p[0], strlen(p[0]));
        XFreeStringList(p);
    }
}


static void insert_cutbuffer(WWindow *wwin)
{
    char *p;
    int n;
    
    p=XFetchBytes(ioncore_g.dpy, &n);
    
    if(n<=0 || p==NULL)
        return;
    
    window_insstr(wwin, p, n);
}


void ioncore_handle_selection(XSelectionEvent *ev)
{
    Atom prop=ev->property;
    Window win=ev->requestor;
    WWindow *wwin;
    
    wwin=XWINDOW_REGION_OF_T(win, WWindow);
    
    if(wwin==NULL)
        return;
    
    if(prop==None){
        insert_cutbuffer(wwin);
        return;
    }

    insert_selection(wwin, win, prop);
    XDeleteProperty(ioncore_g.dpy, win, prop);
}


void ioncore_clear_selection()
{
    if(selection_data!=NULL){
        free(selection_data);
        selection_data=NULL;
    }
}


void ioncore_set_selection(const char *p, int n)
{
    if(selection_data!=NULL)
        free(selection_data);
    
    selection_data=ALLOC_N(char, n+1);
    
    if(selection_data==NULL){
        warn_err();
        return;
    }
    memcpy(selection_data, p, n);
    selection_data[n]='\0';
    selection_length=n;
    
    XStoreBytes(ioncore_g.dpy, p, n);
    
    XSetSelectionOwner(ioncore_g.dpy, XA_PRIMARY,
                       DefaultRootWindow(ioncore_g.dpy),
                       CurrentTime);
}


void ioncore_request_selection_for(Window win)
{
    Atom a=XA_STRING;
    
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

