/*
 * ion/mod_query/wedln.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_MOD_QUERY_WEDLN_H
#define ION_MOD_QUERY_WEDLN_H

#include <libtu/obj.h>
#include <libextl/extl.h>
#include <libmainloop/signal.h>
#include <ioncore/common.h>
#include <ioncore/window.h>
#include <ioncore/xic.h>
#include <ioncore/rectangle.h>
#include <ioncore/binding.h>
#include "listing.h"
#include "input.h"
#include "edln.h"

INTRCLASS(WEdln);
INTRSTRUCT(WEdlnCreateParams);

DECLSTRUCT(WEdlnCreateParams){
    const char *prompt;
    const char *dflt;
    ExtlFn handler;
    ExtlFn completor;
};


DECLCLASS(WEdln){
    WInput input;

    Edln edln;

    char *prompt;
    int prompt_len;
    int prompt_w;
    
    char *info;
    int info_len;
    int info_w;
    
    int vstart;
    
    ExtlFn handler;
    ExtlFn completor;
    
    WTimer *autoshowcompl_timer;
    
    WListing compl_list;
    char *compl_beg;
    char *compl_end;
    int compl_waiting_id;
    int compl_current_id;
    int compl_timed_id;
    uint compl_tab:1;
    uint compl_history_mode:1;
    
    WBindmap *cycle_bindmap;
};

extern WEdln *create_wedln(WWindow *par, const WFitParams *fp,
                           WEdlnCreateParams *p);
extern void wedln_finish(WEdln *wedln);
extern void wedln_paste(WEdln *wedln);
extern void wedln_draw(WEdln *wedln, bool complete);
extern void wedln_set_completions(WEdln *wedln, ExtlTab completions,
                                  bool autoshow_select_first);
extern void wedln_hide_completions(WEdln *wedln);
extern bool wedln_set_histcompl(WEdln *wedln, int sp);
extern bool wedln_get_histcompl(WEdln *wedln);

#endif /* ION_MOD_QUERY_WEDLN_H */
