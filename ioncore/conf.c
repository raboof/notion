/*
 * ion/ioncore/conf.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <stdlib.h>
#include <string.h>

#include <libtu/map.h>
#include <libtu/minmax.h>
#include <libtu/objp.h>
#include <libtu/map.h>
#include <libextl/readconfig.h>

#include "common.h"
#include "global.h"
#include "modules.h"
#include "rootwin.h"
#include "bindmaps.h"
#include "kbresize.h"
#include "reginfo.h"
#include "group-ws.h"
#include "llist.h"


StringIntMap frame_idxs[]={
    {"last", LLIST_INDEX_LAST},
    {"next",  LLIST_INDEX_AFTER_CURRENT},
    {"next-act",  LLIST_INDEX_AFTER_CURRENT_ACT},
    END_STRINGINTMAP
};

static bool get_winprop_fn_set=FALSE;
static ExtlFn get_winprop_fn;

static bool get_layout_fn_set=FALSE;
static ExtlFn get_layout_fn;


/*EXTL_DOC
 * Set ioncore basic settings. The table \var{tab} may contain the
 * following fields.
 * 
 * \begin{tabularx}{\linewidth}{lX}
 *  \tabhead{Field & Description}
 *  \var{opaque_resize} & (boolean) Controls whether interactive move and
 *                        resize operations simply draw a rubberband during
 *                        the operation (false) or immediately affect the 
 *                        object in question at every step (true). \\
 *  \var{warp} &          (boolean) Should focusing operations move the 
 *                        pointer to the object to be focused? \\
 *  \var{switchto} &      (boolean) Should a managing \type{WMPlex} switch
 *                        to a newly mapped client window? \\
 *  \var{screen_notify} & (boolean) Should notification tooltips be displayed
 *                        for hidden workspaces with activity? \\
 *  \var{frame_default_index} & (string) Specifies where to add new regions
 *                        on the mutually exclusive list of a frame. One of
 *                        ''last'', ''next'' (for after current), ''next-act''
 *                        (for after current and anything with activity right
 *                        after it). \\
 *  \var{dblclick_delay} & (integer) Delay between clicks of a double click.\\
 *  \var{kbresize_delay} & (integer) Delay in milliseconds for ending keyboard
 *                         resize mode after inactivity. \\
 *  \var{kbresize_t_max} & (integer) Controls keyboard resize acceleration. 
 *                         See description below for details. \\
 *  \var{kbresize_t_min} & (integer) See below. \\
 *  \var{kbresize_step} & (floating point) See below. \\
 *  \var{kbresize_maxacc} & (floating point) See below. \\
 *  \var{framed_transients} & (boolean) Put transients in nested frames. \\
 *  \var{float_placement_method} & (string) How to place floating frames.
 *                          One of ''udlr'' (up-down, then left-right), 
 *                          ''lrud'' (left-right, then up-down) or ''random''. \\
 *  \var{mousefocus} & String: ''disable'' or ''sloppy''. \\
 *  \var{unsqueeze} & (boolean) Auto-unsqueeze transients/menus/queries/etc. \\
 *  \var{autoraise} & (boolean) Autoraise regions in groups on goto. \\
 * \end{tabularx}
 * 
 * When a keyboard resize function is called, and at most \var{kbresize_t_max} 
 * milliseconds has passed from a previous call, acceleration factor is reset 
 * to 1.0. Otherwise, if at least \var{kbresize_t_min} milliseconds have 
 * passed from the from previous acceleration update or reset the squere root
 * of the acceleration factor is incremented by \var{kbresize_step}. The 
 * maximum acceleration factor (pixels/call modulo size hints) is given by 
 * \var{kbresize_maxacc}. The default values are (200, 50, 30, 100). 
 */
EXTL_EXPORT
void ioncore_set(ExtlTab tab)
{
    int dd, rd;
    char *wst, *tmp;
    ExtlTab t;
    ExtlFn fn;
    
    extl_table_gets_b(tab, "opaque_resize", &(ioncore_g.opaque_resize));
    extl_table_gets_b(tab, "warp", &(ioncore_g.warp_enabled));
    extl_table_gets_b(tab, "switchto", &(ioncore_g.switchto_new));
    extl_table_gets_b(tab, "screen_notify", &(ioncore_g.screen_notify));
    extl_table_gets_b(tab, "framed_transients", &(ioncore_g.framed_transients));
    extl_table_gets_b(tab, "unsqueeze", &(ioncore_g.unsqueeze_enabled));
    extl_table_gets_b(tab, "autoraise", &(ioncore_g.autoraise));
    
    if(extl_table_gets_s(tab, "frame_default_index", &tmp)){
        ioncore_g.frame_default_index=stringintmap_value(frame_idxs, 
                                                         tmp,
                                                         ioncore_g.frame_default_index);
        free(tmp);
    }

    if(extl_table_gets_s(tab, "mousefocus", &tmp)){
        if(strcmp(tmp, "disabled")==0)
            ioncore_g.no_mousefocus=TRUE;
        else if(strcmp(tmp, "sloppy")==0)
            ioncore_g.no_mousefocus=FALSE;
    }
    
    if(extl_table_gets_i(tab, "dblclick_delay", &dd))
        ioncore_g.dblclick_delay=maxof(0, dd);
    
    ioncore_set_moveres_accel(tab);
    
    ioncore_groupws_set(tab);
    
    /* Internal -- therefore undocumented above */
    if(extl_table_gets_f(tab, "_get_winprop", &fn)){
        if(get_winprop_fn_set)
            extl_unref_fn(get_winprop_fn);
        get_winprop_fn=fn;
        get_winprop_fn_set=TRUE;
    }
    
    if(extl_table_gets_f(tab, "_get_layout", &fn)){
        if(get_layout_fn_set)
            extl_unref_fn(get_layout_fn);
        get_layout_fn=fn;
        get_layout_fn_set=TRUE;
    }
    
}


/*EXTL_DOC
 * Get ioncore basic settings. For details see \fnref{ioncore.set}.
 */
EXTL_SAFE
EXTL_EXPORT
ExtlTab ioncore_get()
{
    ExtlTab tab=extl_create_table();
    
    extl_table_sets_b(tab, "opaque_resize", ioncore_g.opaque_resize);
    extl_table_sets_b(tab, "warp", ioncore_g.warp_enabled);
    extl_table_sets_b(tab, "switchto", ioncore_g.switchto_new);
    extl_table_sets_i(tab, "dblclick_delay", ioncore_g.dblclick_delay);
    extl_table_sets_b(tab, "screen_notify", ioncore_g.screen_notify);
    extl_table_sets_b(tab, "framed_transients", ioncore_g.framed_transients);
    extl_table_sets_b(tab, "unsqueeze", ioncore_g.unsqueeze_enabled);
    extl_table_sets_b(tab, "autoraise", ioncore_g.autoraise);
    

    extl_table_sets_s(tab, "frame_default_index", 
                      stringintmap_key(frame_idxs, 
                                       ioncore_g.frame_default_index,
                                       NULL));
    
    extl_table_sets_s(tab, "mousefocus", (ioncore_g.no_mousefocus
                                          ? "disabled" 
                                          : "sloppy"));

    ioncore_get_moveres_accel(tab);
    
    ioncore_groupws_get(tab);
    
    return tab;
}


ExtlTab ioncore_get_winprop(WClientWin *cwin)
{
    ExtlTab tab=extl_table_none();
    
    if(get_winprop_fn_set){
        extl_protect(NULL);
        extl_call(get_winprop_fn, "o", "t", cwin, &tab);
        extl_unprotect(NULL);
    }
    
    return tab;
}


ExtlTab ioncore_get_layout(const char *layout)
{
    ExtlTab tab=extl_table_none();
    
    if(get_layout_fn_set){
        extl_protect(NULL);
        extl_call(get_layout_fn, "s", "t", layout, &tab);
        extl_unprotect(NULL);
    }
    
    return tab;
}
    

/*EXTL_DOC
 * Get important directories (userdir, sessiondir, searchpath).
 */
EXTL_SAFE
EXTL_EXPORT
ExtlTab ioncore_get_paths(ExtlTab tab)
{
    tab=extl_create_table();
    extl_table_sets_s(tab, "userdir", extl_userdir());
    extl_table_sets_s(tab, "sessiondir", extl_sessiondir());
    extl_table_sets_s(tab, "searchpath", extl_searchpath());
    return tab;
}


/*EXTL_DOC
 * Set important directories (sessiondir, searchpath).
 */
EXTL_EXPORT
bool ioncore_set_paths(ExtlTab tab)
{
    char *s;

    if(extl_table_gets_s(tab, "userdir", &s)){
        warn(TR("User directory can not be set."));
        free(s);
        return FALSE;
    }
    
    if(extl_table_gets_s(tab, "sessiondir", &s)){
        extl_set_sessiondir(s);
        free(s);
        return FALSE;
    }

    if(extl_table_gets_s(tab, "searchpath", &s)){
        extl_set_searchpath(s);
        free(s);
        return FALSE;
    }
    
    return TRUE;
}


/* Exports these in ioncore. */

/*EXTL_DOC
 * Lookup script \var{file}. If \var{try_in_dir} is set, it is tried
 * before the standard search path.
 */
EXTL_SAFE
EXTL_EXPORT_AS(ioncore, lookup_script)
char *extl_lookup_script(const char *file, const char *sp);


/*EXTL_DOC
 * Get a file name to save (session) data in. The string \var{basename} 
 * should contain no path or extension components.
 */
EXTL_SAFE
EXTL_EXPORT_AS(ioncore, get_savefile)
char *extl_get_savefile(const char *basename);


/*EXTL_DOC
 * Write \var{tab} in file with basename \var{basename} in the
 * session directory.
 */
EXTL_SAFE
EXTL_EXPORT_AS(ioncore, write_savefile)
bool extl_write_savefile(const char *basename, ExtlTab tab);


/*EXTL_DOC
 * Read a savefile.
 */
EXTL_SAFE
EXTL_EXPORT_AS(ioncore, read_savefile)
ExtlTab extl_extl_read_savefile(const char *basename);

    

bool ioncore_read_main_config(const char *cfgfile)
{
    bool ret;
    int unset=0;

    if(cfgfile==NULL)
        cfgfile="cfg_ion";
    
    ret=extl_read_config(cfgfile, ".", TRUE);
    
    unset+=(ioncore_screen_bindmap->nbindings==0);
    unset+=(ioncore_mplex_bindmap->nbindings==0);
    unset+=(ioncore_frame_bindmap->nbindings==0);
    
    if(unset>0){
        warn(TR("Some bindmaps were empty, loading ioncore_efbb."));
        extl_read_config("ioncore_efbb", NULL, TRUE);
    }
    
    return (ret && unset==0);
}
