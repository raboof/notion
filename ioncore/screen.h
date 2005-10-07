/*
 * ion/ioncore/screen.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_SCREEN_H
#define ION_IONCORE_SCREEN_H

#include <libextl/extl.h>
#include <libmainloop/hooks.h>
#include "common.h"
#include "mplex.h"
#include "rectangle.h"
#include "rootwin.h"
#include "pholder.h"

#define FOR_ALL_SCREENS(SCR)   \
    for((SCR)=ioncore_g.screens; \
        (SCR)!=NULL;           \
        (SCR)=(SCR)->next_scr)

#define FOR_ALL_SCREENS_W_NEXT(SCR, NXT)                               \
    for((SCR)=ioncore_g.screens, NXT=((SCR) ? (SCR)->next_scr : NULL); \
        (SCR)!=NULL;                                                   \
        (SCR)=(NXT), NXT=((SCR) ? (SCR)->next_scr : NULL))

enum{
    SCREEN_ROTATION_0,
    SCREEN_ROTATION_90,
    SCREEN_ROTATION_180,
    SCREEN_ROTATION_270
};


DECLCLASS(WScreen){
    WMPlex mplex;
    int id;
    Atom atom_workspace;
    bool uses_root;
    int rotation;
    WRectangle managed_off;
    WScreen *next_scr, *prev_scr;
    Watch notifywin_watch;
};

extern WScreen *create_screen(WRootWin *rootwin, int id, 
                              const WFitParams *fp,
                              bool useroot);

extern int screen_id(WScreen *scr);

extern void screen_set_managed_offset(WScreen *scr, const WRectangle *off);

extern bool screen_init_layout(WScreen *scr, ExtlTab tab);

extern void screen_notify(WScreen *scr, const char *notstr);
extern void screen_unnotify(WScreen *scr);

extern WPHolder *screen_get_rescue_pholder_for(WScreen *scr, WRegion *mgd);

/* For viewports corresponding to Xinerama rootwins <id> is initially set
 * to the Xinerama screen number. When Xinerama is not enabled, <id> is
 * the X screen number (which is the same for all Xinerama rootwins).
 * For all other viewports <id> is undefined.
 */
extern WScreen *ioncore_find_screen_id(int id);
extern WScreen *ioncore_goto_screen_id(int id);
extern WScreen *ioncore_goto_next_screen();
extern WScreen *ioncore_goto_prev_screen();

/* Handlers of this hook receive a WScreen* as the sole parameter. */
extern WHook *screen_managed_changed_hook;

#endif /* ION_IONCORE_SCREEN_H */
