/*
 * ion/ioncore/screen.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_SCREEN_H
#define ION_IONCORE_SCREEN_H

#include "common.h"
#include "mplex.h"
#include "extl.h"
#include "rectangle.h"
#include "rootwin.h"

#define FOR_ALL_SCREENS(SCR)   \
    for((SCR)=ioncore_g.screens; \
        (SCR)!=NULL;           \
        (SCR)=(SCR)->next_scr)


DECLCLASS(WScreen){
    WMPlex mplex;
    int id;
    Atom atom_workspace;
    bool uses_root;
    WRectangle managed_off;
    WScreen *next_scr, *prev_scr;
};

extern WScreen *create_screen(WRootWin *rootwin, int id, 
                              const WFitParams *fp,
                              bool useroot);

extern int screen_id(WScreen *scr);

extern void screen_set_managed_offset(WScreen *scr, const WRectangle *off);

extern bool screen_init_layout(WScreen *scr, ExtlTab tab);

/* For viewports corresponding to Xinerama rootwins <id> is initially set
 * to the Xinerama screen number. When Xinerama is not enabled, <id> is
 * the X screen number (which is the same for all Xinerama rootwins).
 * For all other viewports <id> is undefined.
 */
extern WScreen *ioncore_find_screen_id(int id);
extern WScreen *ioncore_goto_screen_id(int id);
extern WScreen *ioncore_goto_next_screen();
extern WScreen *ioncore_goto_prev_screen();

#endif /* ION_IONCORE_SCREEN_H */
