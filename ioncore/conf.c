/*
 * ion/ioncore/conf.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
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

#include "common.h"
#include "global.h"
#include <libextl/readconfig.h>
#include "modules.h"
#include "rootwin.h"
#include "bindmaps.h"
#include "kbresize.h"
#include <libextl/readconfig.h>


char *ioncore_default_ws_type=NULL;


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
 *  \var{frame_add_last} & (boolean) Add new regions in frames last instead
 *                        of after current region. \\
 *  \var{dblclick_delay} & (integer) Delay between clicks of a double click.\\
 *  \var{default_ws_type} & (string) Default workspace type for operations
 *                         that create a workspace.\\
 *  \var{kbresize_delay} & (integer) Delay in milliseconds for ending keyboard
 *                         resize mode after inactivity. \\
 *  \var{kbresize_t_max} & (integer) Controls keyboard resize acceleration. 
 *                         See description below for details. \\
 *  \var{kbresize_t_min} & (integer) See below. \\
 *  \var{kbresize_step} & (floating point) See below. \\
 *  \var{kbresize_maxacc} & (floating point) See below. \\
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
    char *wst;
    
    extl_table_gets_b(tab, "opaque_resize", &(ioncore_g.opaque_resize));
    extl_table_gets_b(tab, "warp", &(ioncore_g.warp_enabled));
    extl_table_gets_b(tab, "switchto", &(ioncore_g.switchto_new));
    extl_table_gets_b(tab, "screen_notify", &(ioncore_g.screen_notify));
    extl_table_gets_b(tab, "frame_add_last", &(ioncore_g.frame_add_last));
    
    if(extl_table_gets_i(tab, "dblclick_delay", &dd))
        ioncore_g.dblclick_delay=maxof(0, dd);
    if(extl_table_gets_s(tab, "default_ws_type", &wst)){
        if(ioncore_default_ws_type!=NULL)
            free(ioncore_default_ws_type);
        ioncore_default_ws_type=wst;
    }
    ioncore_set_moveres_accel(tab);
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
    extl_table_sets_s(tab, "default_ws_type", ioncore_default_ws_type);
    extl_table_sets_b(tab, "frame_add_last", ioncore_g.frame_add_last);
    ioncore_get_moveres_accel(tab);
    
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
    
    unset+=(ioncore_rootwin_bindmap->nbindings==0);
    unset+=(ioncore_mplex_bindmap->nbindings==0);
    unset+=(ioncore_frame_bindmap->nbindings==0);
    
    if(unset>0){
        warn(TR("Some bindmaps were empty, loading ioncore-efbb."));
        extl_read_config("ioncore-efbb", NULL, TRUE);
    }
    
    return (ret && unset==0);
}
