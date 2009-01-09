/*
 * ion/mod_statusbar/main.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
 */

#include <libtu/minmax.h>
#include <libextl/readconfig.h>
#include <ioncore/saveload.h>
#include <ioncore/bindmaps.h>
#include <ioncore/global.h>
#include <ioncore/ioncore.h>

#include "statusbar.h"
#include "exports.h"


/*{{{ Module information */


#include "../version.h"

char mod_statusbar_ion_api_version[]=ION_API_VERSION;


/*}}}*/


/*{{{ Bindmaps */


WBindmap *mod_statusbar_statusbar_bindmap=NULL;


/*}}}*/


/*{{{ Systray */


static bool is_systray(WClientWin *cwin)
{
    static Atom atom__kde_net_wm_system_tray_window_for=None;
    Atom actual_type=None;
    int actual_format;
    unsigned long nitems;
    unsigned long bytes_after;
    unsigned char *prop;
    char *dummy;
    bool is=FALSE;

    if(extl_table_gets_s(cwin->proptab, "statusbar", &dummy)){
        free(dummy);
        return TRUE;
    }
    
    if(atom__kde_net_wm_system_tray_window_for==None){
        atom__kde_net_wm_system_tray_window_for=XInternAtom(ioncore_g.dpy,
                                                            "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR",
                                                            False);
    }
    if(XGetWindowProperty(ioncore_g.dpy, cwin->win,
                          atom__kde_net_wm_system_tray_window_for, 0,
                          sizeof(Atom), False, AnyPropertyType, 
                          &actual_type, &actual_format, &nitems,
                          &bytes_after, &prop)==Success){
        if(actual_type!=None){
            is=TRUE;
        }
        XFree(prop);
    }
    
    return is;
}


static bool clientwin_do_manage_hook(WClientWin *cwin, const WManageParams *param)
{
    WStatusBar *sb=NULL;
    
    if(!is_systray(cwin))
        return FALSE;
    
    sb=mod_statusbar_find_suitable(cwin, param);
    if(sb==NULL)
        return FALSE;

    return region_manage_clientwin((WRegion*)sb, cwin, param,
                                   MANAGE_PRIORITY_NONE);
}

    
/*}}}*/


/*{{{ Init & deinit */


void mod_statusbar_deinit()
{
    hook_remove(clientwin_do_manage_alt, 
                (WHookDummy*)clientwin_do_manage_hook);

    if(mod_statusbar_statusbar_bindmap!=NULL){
        ioncore_free_bindmap("WStatusBar", mod_statusbar_statusbar_bindmap);
        mod_statusbar_statusbar_bindmap=NULL;
    }

    ioncore_unregister_regclass(&CLASSDESCR(WStatusBar));

    mod_statusbar_unregister_exports();
}


bool mod_statusbar_init()
{
    mod_statusbar_statusbar_bindmap=ioncore_alloc_bindmap("WStatusBar", NULL);
    
    if(mod_statusbar_statusbar_bindmap==NULL)
        return FALSE;

    if(!ioncore_register_regclass(&CLASSDESCR(WStatusBar),
                                  (WRegionLoadCreateFn*) statusbar_load)){
        mod_statusbar_deinit();
        return FALSE;
    }

    if(!mod_statusbar_register_exports()){
        mod_statusbar_deinit();
        return FALSE;
    }
    
    hook_add(clientwin_do_manage_alt, 
             (WHookDummy*)clientwin_do_manage_hook);

    /*ioncore_read_config("cfg_statusbar", NULL, TRUE);*/
    
    return TRUE;
}


/*}}}*/

