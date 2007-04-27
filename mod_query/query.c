/*
 * ion/mod_query/query.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#include <libextl/extl.h>

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/binding.h>
#include <ioncore/regbind.h>
#include <ioncore/bindmaps.h>
#include <ioncore/stacking.h>
#include <ioncore/key.h>
#include "query.h"
#include "wedln.h"


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
                          ExtlFn handler, ExtlFn completor, 
                          ExtlFn cycle, ExtlFn bcycle)
{
    WRectangle geom;
    WEdlnCreateParams fnp;
    WMPlexAttachParams par;
    WEdln *wedln;

    fnp.prompt=prompt;
    fnp.dflt=dflt;
    fnp.handler=handler;
    fnp.completor=completor;
    
    par.flags=(MPLEX_ATTACH_SWITCHTO|
               MPLEX_ATTACH_LEVEL|
               MPLEX_ATTACH_UNNUMBERED|
               MPLEX_ATTACH_SIZEPOLICY);
    par.szplcy=SIZEPOLICY_FULL_BOUNDS;
    par.level=STACKING_LEVEL_MODAL1+1;

    wedln=(WEdln*)mplex_do_attach_new(mplex, &par,
                                      (WRegionCreateFn*)create_wedln,
                                      (void*)&fnp); 
                                      
    if(wedln!=NULL && cycle!=extl_fn_none()){
        uint kcb, state; 
        bool sub;
        
        if(ioncore_current_key(&kcb, &state, &sub) && !sub){
            wedln->cycle_bindmap=region_add_cycle_bindmap((WRegion*)wedln,
                                                          kcb, state, cycle,
                                                          bcycle);
        }
    }
    
    return wedln;
}

