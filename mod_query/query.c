/*
 * ion/mod_query/query.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libextl/extl.h>

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/binding.h>
#include <ioncore/regbind.h>
#include <ioncore/key.h>
#include "query.h"
#include "wedln.h"


static void create_cycle_binding(WEdln *wedln, XKeyEvent *ev, ExtlFn cycle)
{
    WBindmap *bindmap=create_bindmap();
    WBinding b;
    
    if(bindmap==NULL)
        return;
        
    b.ksb=XKeycodeToKeysym(ioncore_g.dpy, ev->keycode, 0);
    b.kcb=ev->keycode;
    b.state=ev->state;
    b.act=BINDING_KEYPRESS;
    b.area=0;
    b.wait=FALSE;
    b.submap=NULL;
    b.func=extl_ref_fn(cycle);
    
    if(!bindmap_add_binding(bindmap, &b)){
        extl_unref_fn(b.func);
        bindmap_destroy(bindmap);
        return;
    }
    
    if(!region_add_bindmap((WRegion*)wedln, bindmap)){
        bindmap_destroy(bindmap);
        return;
    }
    
    wedln->cycle_bindmap=bindmap;
}
    

/*--lowlevel routine not to be called by the user--EXTL_DOC
 * Show a query window in \var{mplex} with prompt \var{prompt}, initial
 * contents \var{dflt}. The function \var{handler} is called with
 * the entered string as the sole argument when \fnref{WEdln.finish}
 * is called. The function \var{completor} is called with the created
 * \type{WEdln} is first argument and the string to complete is the
 * second argument when \fnref{WEdln.complete} is called.
 */
EXTL_EXPORT
WEdln *mod_query_do_query(WMPlex *mplex, const char *prompt, const char *dflt,
                          ExtlFn handler, ExtlFn completor, ExtlFn cycle)
{
    WRectangle geom;
    WEdlnCreateParams fnp;
    WMPlexAttachParams par;
    XKeyEvent *ev=ioncore_current_key_event();
    WEdln *wedln;

    fnp.prompt=prompt;
    fnp.dflt=dflt;
    fnp.handler=handler;
    fnp.completor=completor;
    
    par.flags=(MPLEX_ATTACH_SWITCHTO|
               MPLEX_ATTACH_MODAL|
               MPLEX_ATTACH_UNNUMBERED|
               MPLEX_ATTACH_SIZEPOLICY);
    par.szplcy=SIZEPOLICY_FULL_BOUNDS;

    wedln=(WEdln*)mplex_do_attach_new(mplex, &par,
                                      (WRegionCreateFn*)create_wedln,
                                      (void*)&fnp); 
                                      
    if(wedln!=NULL && ev!=NULL && cycle!=extl_fn_none())
        create_cycle_binding(wedln, ev, cycle);
        
    
    return wedln;
}

