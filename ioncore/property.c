/*
 * ion/ioncore/property.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
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


/*{{{ Primitives */


static ulong xwindow_get_property_(Window win, Atom atom, Atom type, 
                                   ulong n32expected, bool more, uchar **p,
                                   int *format)
{
    Atom real_type;
    ulong n=-1, extra=0;
    int status;
    
    do{
        status=XGetWindowProperty(ioncore_g.dpy, win, atom, 0L, n32expected, 
                                  False, type, &real_type, format, &n,
                                  &extra, p);
        
        if(status!=Success || *p==NULL)
            return -1;
    
        if(extra==0 || !more)
            break;
        
        XFree((void*)*p);
        n32expected+=(extra+4)/4;
        more=FALSE;
    }while(1);

    if(n==0){
        XFree((void*)*p);
        *p=NULL;
        return -1;
    }
    
    return n;
}


ulong xwindow_get_property(Window win, Atom atom, Atom type, 
                           ulong n32expected, bool more, uchar **p)
{
    int format=0;
    return xwindow_get_property_(win, atom, type, n32expected, more, p, 
                                 &format);
}


/*}}}*/


/*{{{ String property stuff */


char *xwindow_get_string_property(Window win, Atom a, int *nret)
{
    char *p;
    int n;
    
    n=xwindow_get_property(win, a, XA_STRING, 64L, TRUE, (uchar**)&p);
    
    if(nret!=NULL)
        *nret=n;
    
    return (n<=0 ? NULL : p);
}


void xwindow_set_string_property(Window win, Atom a, const char *value)
{
    if(value==NULL){
        XDeleteProperty(ioncore_g.dpy, win, a);
    }else{
        XChangeProperty(ioncore_g.dpy, win, a, XA_STRING,
                        8, PropModeReplace, (uchar*)value, strlen(value));
    }
}


/*}}}*/


/*{{{ Integer property stuff */


bool xwindow_get_integer_property(Window win, Atom a, int *vret)
{
    long *p=NULL;
    ulong n;
    
    n=xwindow_get_property(win, a, XA_INTEGER, 1L, FALSE, (uchar**)&p);
    
    if(n>0 && p!=NULL){
        *vret=*p;
        XFree((void*)p);
        return TRUE;
    }
    
    return FALSE;
}


void xwindow_set_integer_property(Window win, Atom a, int value)
{
    CARD32 data[2];
    
    data[0]=value;
    
    XChangeProperty(ioncore_g.dpy, win, a, XA_INTEGER,
                    32, PropModeReplace, (uchar*)data, 1);
}


/* WM_STATE
 */

bool xwindow_get_state_property(Window win, int *state)
{
    CARD32 *p=NULL;
    
    if(xwindow_get_property(win, ioncore_g.atom_wm_state, 
                            ioncore_g.atom_wm_state, 
                            2L, FALSE, (uchar**)&p)<=0)
        return FALSE;
    
    *state=*p;
    
    XFree((void*)p);
    
    return TRUE;
}


void xwindow_set_state_property(Window win, int state)
{
    CARD32 data[2];
    
    data[0]=state;
    data[1]=None;
    
    XChangeProperty(ioncore_g.dpy, win,
                    ioncore_g.atom_wm_state, ioncore_g.atom_wm_state,
                    32, PropModeReplace, (uchar*)data, 2);
}


/*}}}*/


/*{{{ Text property stuff */


char **xwindow_get_text_property(Window win, Atom a, int *nret)
{
    XTextProperty prop;
    char **list=NULL;
    int n=0;
    Status st=0;
    
    st=XGetTextProperty(ioncore_g.dpy, win, &prop, a);

    if(nret)
        *nret=(!st ? 0 : -1);
    
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

    if(!ioncore_g.use_mb){
        st=XTextPropertyToStringList(&prop, &list, &n);
    }else{
        st=XmbTextPropertyToTextList(ioncore_g.dpy, &prop, &list, &n);
        st=!st;
    }

    XFree(prop.value);
    
    if(!st || n==0 || list==NULL)
        return NULL;
    
    if(nret)
        *nret=n;
    
    return list;
}


void xwindow_set_text_property(Window win, Atom a, const char **ptr, int n)
{
    XTextProperty prop;
    Status st;

    if(!ioncore_g.use_mb){
        st=XStringListToTextProperty((char **)&ptr, 1, &prop);
    }else{
#ifdef X_HAVE_UTF8_STRING        
        st=XmbTextListToTextProperty(ioncore_g.dpy, (char **)ptr, n,
                                     XUTF8StringStyle, &prop);
#else        
        st=XmbTextListToTextProperty(ioncore_g.dpy, (char **)ptr, n,
                                     XTextStyle, &prop);
#endif        
        st=!st;
    }
    
    if(!st)
        return;
    
    XSetTextProperty(ioncore_g.dpy, win, &prop, a);
    XFree(prop.value);
}


/*}}}*/


/*{{{ Exports */


EXTL_EXPORT
int ioncore_x_intern_atom(const char *name, bool only_if_exists)
{
    return XInternAtom(ioncore_g.dpy, name, only_if_exists);
}


EXTL_EXPORT
char *ioncore_x_get_atom_name(const char *name, int atom)
{
    return XGetAtomName(ioncore_g.dpy, atom);
}


#define CP(TYPE)                                              \
    {                                                         \
        TYPE *d=(TYPE*)p;                                     \
        for(i=0; i<n; i++) extl_table_seti_i(tab, i+1, d[i]); \
    }


EXTL_EXPORT
ExtlTab ioncore_x_get_window_property(int win, int atom, int atom_type,
                                      int n32expected, bool more)
{
    uchar *p=NULL;
    ExtlTab tab;
    int format=0;
    int i, n;
    
    n=xwindow_get_property_(win, atom, atom_type, n32expected, more, &p, 
                            &format);
    
    if(p==NULL)
        return extl_table_none();
    
    if(n<=0 || (format!=8 && format!=16 && format!=32)){
        free(p);
        return extl_table_none();
    }
    
    tab=extl_create_table();
    
    switch(format){
    case 8: CP(char); break;
    case 16: CP(short); break;
    case 32: CP(long); break;
    }

    return tab;
}


#define GET(TYPE)                                          \
    {                                                      \
        TYPE *d=ALLOC_N(TYPE, n);                          \
        if(d==NULL) return;                                \
        for(i=0; i<n; i++) {                               \
            if(!extl_table_geti_i(tab, i+1, &tmp)) return; \
            d[i]=tmp;                                      \
        }                                                  \
        p=(uchar*)d;                                       \
    }

        
static bool get_mode(const char *mode, int *m)
{
    if(strcmp(mode, "replace")==0)
        *m=PropModeReplace;
    else if(strcmp(mode, "prepend")==0)
        *m=PropModePrepend;
    else if(strcmp(mode, "append")==0)
        *m=PropModeAppend;
    else
        return FALSE;
    
    return TRUE;
}


EXTL_EXPORT
void ioncore_x_change_property(int win, int atom, int atom_type,
                               int format, const char *mode, ExtlTab tab)
{
    int tmp, m, i, n=extl_table_get_n(tab);
    uchar *p;
        
    if(n<0 || !get_mode(mode, &m)){
        warn(TR("Invalid arguments."));
        return;
    }

    switch(format){
    case 8: GET(char); break;
    case 16: GET(short); break;
    case 32: GET(long); break;
    default:
        warn(TR("Invalid arguments."));
        return;
    }
    
    XChangeProperty(ioncore_g.dpy, win, atom, atom_type, format, m, p, n);
    
    free(p);
}


EXTL_EXPORT
void ioncore_x_delete_property(int win, int atom)
{
    XDeleteProperty(ioncore_g.dpy, win, atom);
}


EXTL_EXPORT
ExtlTab ioncore_x_get_text_property(int win, int atom)
{
    char **list;
    int i, n;
    ExtlTab tab=extl_table_none();
    
    list=xwindow_get_text_property(win, atom, &n);
    
    if(list!=NULL){
        if(n!=0){
            tab=extl_create_table();
            for(i=0; i<n; i++)
                extl_table_seti_s(tab, i+1, list[i]);
        }
        XFreeStringList(list);
    }
    
    return tab;
}


EXTL_EXPORT
void ioncore_x_set_text_property(int win, int atom, ExtlTab tab)
{
    char **list;
    int i, n=extl_table_get_n(tab);
    
    list=ALLOC_N(char*, n);

    if(list==NULL)
        return;
    
    for(i=0; i<n; i++){
        list[i]=NULL;
        extl_table_geti_s(tab, i+1, &(list[i]));
    }
    
    xwindow_set_text_property(win, atom, (const char **)list, n);
    
    XFreeStringList(list);
}


/*}}}*/

