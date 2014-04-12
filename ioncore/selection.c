/*
 * ion/ioncore/selection.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
 */

#include <X11/Xmd.h>
#include <string.h>

#include "common.h"
#include "global.h"
#include "property.h"
#include "xwindow.h"
#include "utildefines.h"
#include <libextl/extl.h>


static char *selection_data=NULL;
static int selection_length;
static bool continuation_set=FALSE;
static ExtlFn continuation;

#define CLIPATOM(X) XA_PRIMARY

static Atom XA_COMPOUND_TEXT(Display *UNUSED(unused))
{
    static Atom a=None;
    
    if(a==None)
        a=XInternAtom(ioncore_g.dpy, "COMPOUND_TEXT", False);
        
    return a;
}


void ioncore_handle_selection_request(XSelectionRequestEvent *ev)
{
    XSelectionEvent sev;
    XTextProperty prop;
    const char *p[1];
    bool ok=FALSE;
    
    sev.property=None;
    
    if(selection_data==NULL || ev->property==None)
        goto refuse;
    
    p[0]=selection_data;
    
    if(!ioncore_g.use_mb && ev->target==XA_STRING){
        Status st=XStringListToTextProperty((char **)p, 1, &prop);
        ok=(st!=0);
    }else if(ioncore_g.use_mb){
        XICCEncodingStyle style;
        
        if(ev->target==XA_STRING){
            style=XStringStyle;
            ok=TRUE;
        }else if(ev->target==XA_COMPOUND_TEXT(ioncore_g.dpy)){
            style=XCompoundTextStyle;
            ok=TRUE;
        }
        
        if(ok){
            int st=XmbTextListToTextProperty(ioncore_g.dpy, (char **)p, 1,
                                             style, &prop);
            ok=(st>=0);
        }
    }
    
    if(ok){
        XSetTextProperty(ioncore_g.dpy, ev->requestor, &prop, ev->property);
        sev.property=ev->property;
        XFree(prop.value);
    }

refuse:    
    sev.type=SelectionNotify;
    sev.requestor=ev->requestor;
    sev.selection=ev->selection;
    sev.target=ev->target;
    sev.time=ev->time;
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


void ioncore_handle_selection(XSelectionEvent *ev)
{
    Atom prop=ev->property;
    Window win=ev->requestor;
    
    if(prop!=None){
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
    
    XSetSelectionOwner(ioncore_g.dpy, CLIPATOM(ioncore_g.dpy),
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
    
    if(ioncore_g.use_mb)
        a=XA_COMPOUND_TEXT(ioncore_g.dpy);
    
    XConvertSelection(ioncore_g.dpy, CLIPATOM(ioncore_g.dpy), a,
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

