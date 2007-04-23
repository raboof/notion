/*
 * ion/ioncore/clientwin.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_CLIENTWIN_H
#define ION_IONCORE_CLIENTWIN_H

#include <libextl/extl.h>
#include <libtu/ptrlist.h>
#include <libmainloop/hooks.h>
#include "common.h"
#include "region.h"
#include "window.h"
#include "rectangle.h"
#include "attach.h"
#include "manage.h"
#include "pholder.h"
#include "sizepolicy.h"


#define CLIENTWIN_P_WM_DELETE        0x00001
#define CLIENTWIN_P_WM_TAKE_FOCUS    0x00002
#define CLIENTWIN_PROP_ACROBATIC     0x00010
#define CLIENTWIN_PROP_TRANSPARENT   0x00020
#define CLIENTWIN_PROP_IGNORE_CFGRQ  0x00040
#define CLIENTWIN_PROP_MINSIZE       0x00100
#define CLIENTWIN_PROP_MAXSIZE       0x00200
#define CLIENTWIN_PROP_ASPECT        0x00400
#define CLIENTWIN_PROP_RSZINC        0x00800
#define CLIENTWIN_PROP_I_MINSIZE     0x01000
#define CLIENTWIN_PROP_I_MAXSIZE     0x02000
#define CLIENTWIN_PROP_I_ASPECT      0x04000
#define CLIENTWIN_PROP_I_RSZINC      0x08000
#define CLIENTWIN_USE_NET_WM_NAME    0x10000
#define CLIENTWIN_FS_RQ              0x20000
#define CLIENTWIN_UNMAP_RQ           0x40000
#define CLIENTWIN_NEED_CFGNTFY       0x80000


DECLCLASS(WClientWin){
    WRegion region;
    
    int flags;
    int state;
    int event_mask;
    Window win;
    
    int orig_bw;

    Colormap cmap;
    Colormap *cmaps;
    Window *cmapwins;
    int n_cmapwins;

    XSizeHints size_hints;
    
    ExtlTab proptab;
};


extern void clientwin_get_protocols(WClientWin *cwin);
extern void clientwin_get_size_hints(WClientWin *cwin);
extern void clientwin_unmapped(WClientWin *cwin);
extern void clientwin_destroyed(WClientWin *cwin);
extern void clientwin_kill(WClientWin *cwin);
extern void clientwin_rqclose(WClientWin *cwin, bool relocate_ignored);

extern void clientwin_tfor_changed(WClientWin *cwin);

extern void clientwin_get_set_name(WClientWin *cwin);

extern void clientwin_handle_configure_request(WClientWin *cwin,
                                               XConfigureRequestEvent *ev);

extern void clientwin_broken_app_resize_kludge(WClientWin *cwin);

extern WRegion *clientwin_load(WWindow *par, const WFitParams *fp,
                               ExtlTab tab);

/* Some standard winprops */

enum{
    TRANSIENT_MODE_NORMAL,
    TRANSIENT_MODE_CURRENT,
    TRANSIENT_MODE_OFF
};

extern bool clientwin_get_switchto(const WClientWin *cwin);
extern int clientwin_get_transient_mode(const WClientWin *cwin);
extern WClientWin *clientwin_get_transient_for(const WClientWin *cwin);

/* Hooks */

/* This hook has parameters (WClientWin*, WManageParams*). */
extern WHook *clientwin_do_manage_alt;
/* This hook has just WClientWin* as parameter. */
extern WHook *clientwin_mapped_hook;
/* This hook has an X Window id as parameter. */
extern WHook *clientwin_unmapped_hook;
/* This hook has (WClientWin*, const XPropertyEvent *) as parameters on
 * C side, and (WClientWin*, int atom) on Lua side.
 */
extern WHook *clientwin_property_change_hook;

/* Manage */

extern WClientWin *ioncore_manage_clientwin(Window win, bool maprq);

#endif /* ION_IONCORE_CLIENTWIN_H */
