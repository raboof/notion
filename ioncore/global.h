/*
 * ion/ioncore/global.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_GLOBAL_H
#define ION_IONCORE_GLOBAL_H

#include "common.h"

#include <X11/Xutil.h>
#include <X11/Xresource.h>

#include <libtu/stringstore.h>

#include "rootwin.h"
#include "screen.h"
#include "window.h"
#include "clientwin.h"


enum{
    IONCORE_INPUTMODE_NORMAL,
    IONCORE_INPUTMODE_GRAB,
    IONCORE_INPUTMODE_WAITRELEASE
};

enum{
    IONCORE_OPMODE_INIT,
    IONCORE_OPMODE_NORMAL,
    IONCORE_OPMODE_DEINIT
};

enum{
    IONCORE_FOCUSNEXT_OTHER,
    IONCORE_FOCUSNEXT_POINTERHACK,
    IONCORE_FOCUSNEXT_ENTERWINDOW,
    IONCORE_FOCUSNEXT_FALLBACK
};

enum{
    IONCORE_WINDOWSTACKINGREQUEST_IGNORE,
    IONCORE_WINDOWSTACKINGREQUEST_ACTIVATE
};


INTRSTRUCT(WGlobal);


DECLSTRUCT(WGlobal){
    int argc;
    char **argv;

    Display *dpy;
    const char *display;
    int conn;

    XContext win_context;
    Atom atom_wm_state;
    Atom atom_wm_change_state;
    Atom atom_wm_protocols;
    Atom atom_wm_delete;
    Atom atom_wm_take_focus;
    Atom atom_wm_colormaps;
    Atom atom_wm_window_role;
    Atom atom_checkcode;
    Atom atom_selection;
    Atom atom_mwm_hints;
    Atom atom_dockapp_hack;

    WRootWin *rootwins;
    WScreen *screens;
    WRegion *focus_next;
    bool warp_next;
    int focus_next_source;

    /* We could have a display WRegion but the screen-link could impose
     * some problems so these are handled as a special case.
     *
     * This is a doubly-linked list with links 'active_next' and 'active_prev'
     *
     * This is the list of previously-focused windows, in order of recent
     * usefulness. The currently-focussed window is deemed most important (first
     * item on this list) if focuslist_insert_delay is disabled, or if
     * focuslist_insert_delay is enabled and its timer has expired
     */
    WRegion* focuslist;

    /* This is the region that is currently focused. It is usually the first
     * item in the focuslist, but not always. It isn't the first item if
     * focuslist_insert_delay is enabled, and the corresponding timer is active */
    WRegion* focus_current;


    int input_mode;
    int opmode;

    Time dblclick_delay;
    int opaque_resize;
    bool warp_enabled;
    int warp_margin;
    double warp_factor[2];
    bool switchto_new;
    bool screen_notify;
    int frame_default_index;
    bool framed_transients;
    bool no_mousefocus;
    bool unsqueeze_enabled;
    bool autoraise;
    bool autosave_layout;
    int  window_stacking_request;
    Time usertime_diff_current;
    Time usertime_diff_new;
    Time focuslist_insert_delay;
    Time workspace_indicator_timeout;
    bool activity_notification_on_all_screens;

    bool use_mb; /* use mb routines? */
    bool enc_sb; /* 8-bit charset? If unset, use_mb must be set. */
    bool enc_utf8; /* mb encoding is utf8? */

    const char *sm_client_id;

    struct{
        StringId activated,
                 inactivated,
                 activity,
                 sub_activity,
                 name,
                 icon,
                 unset_manager,
                 set_manager,
                 tag,
                 set_return,
                 unset_return,
                 pseudoactivated,
                 pseudoinactivated,
                 deinit,
                 map,
                 unmap;
    } notifies;

    /** XShape extension presence */
    bool shape_extension;
    int shape_event_basep;
    int shape_error_basep;
};


extern WGlobal ioncore_g;

#endif /* ION_IONCORE_GLOBAL_H */
