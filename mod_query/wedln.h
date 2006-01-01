/*
 * ion/mod_query/wedln.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
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
    bool compl_tab;
};

extern WEdln *create_wedln(WWindow *par, const WFitParams *fp,
                           WEdlnCreateParams *p);
extern void wedln_finish(WEdln *wedln);
extern void wedln_paste(WEdln *wedln);
extern void wedln_draw(WEdln *wedln, bool complete);
extern void wedln_set_completions(WEdln *wedln, ExtlTab completions,
                                  bool autoshow_select_first);
extern void wedln_hide_completions(WEdln *wedln);

#endif /* ION_MOD_QUERY_WEDLN_H */
