/*
 * ion/ioncore/statusbar.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <libtu/objp.h>
#include <ioncore/common.h>
#include <ioncore/infowin.h>
#include <ioncore/binding.h>
#include <ioncore/regbind.h>
#include "statusbar.h"
#include "main.h"


/*{{{ Init/deinit */


bool statusbar_init(WStatusBar *p, WWindow *parent, const WFitParams *fp)
{
    if(!infowin_init(&(p->iwin), parent, fp, "stdisp-statusbar"))
        return FALSE;
    
    region_add_bindmap((WRegion*)p, mod_statusbar_statusbar_bindmap);
    
    ((WRegion*)p)->flags|=REGION_SKIP_FOCUS;
    
    return TRUE;
}



WStatusBar *create_statusbar(WWindow *parent, const WFitParams *fp)
{
    CREATEOBJ_IMPL(WStatusBar, statusbar, (p, parent, fp));
}


void statusbar_deinit(WStatusBar *p)
{
    infowin_deinit(&(p->iwin));
}


/*}}}*/


/*{{{ Load */


WRegion *statusbar_load(WWindow *par, const WFitParams *fp, ExtlTab tab)
{
    return (WRegion*)create_statusbar(par, fp);
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab statusbar_dynfuntab[]={
    END_DYNFUNTAB
};


IMPLCLASS(WStatusBar, WInfoWin, statusbar_deinit, NULL);

    
/*}}}*/

