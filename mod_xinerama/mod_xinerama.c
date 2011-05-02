/*
 * Ion xinerama module
 * based on mod_xrandr by Ragnar Rova and Tuomo Valkonen
 *
 * by Thomas Themel <themel0r@wannabehacker.com>
 *
 * splitted to read and setup part callable by lua
 * by Tomas Ebenlendr <ebik@ucw.cz>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License,or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not,write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>
#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/mplex.h>
#include <ioncore/group-ws.h>
#include <ioncore/sizepolicy.h>
#include <ioncore/../version.h>
#include "exports.h"

#ifdef MOD_XINERAMA_DEBUG
#include <stdio.h>
#endif

char mod_xinerama_ion_api_version[]=ION_API_VERSION;

/* {{{ the actual xinerama query */
static int xinerama_event_base;
static int xinerama_error_base;
static bool xinerama_ready = FALSE;

/*EXTL_DOC
 * Queries the Xinerama extension for screen configuration.
 *
 * Example output: \{\{x=0,y=0,w=1024,h=768\},\{x=1024,y=0,w=1280,h=1024\}\}
 */
EXTL_SAFE
EXTL_EXPORT
ExtlTab mod_xinerama_query_screens()
{
    XineramaScreenInfo* sInfo;
    int nRects,i;
    if (xinerama_ready) {
        ExtlTab ret = extl_create_table();

        sInfo = XineramaQueryScreens(ioncore_g.dpy, &nRects);

        if(!sInfo) return ret;
       
        for(i = 0 ; i < nRects ; ++i) {
            ExtlTab rect = extl_create_table();
            extl_table_sets_i(rect,"x",sInfo[i].x_org);
            extl_table_sets_i(rect,"y",sInfo[i].y_org);
            extl_table_sets_i(rect,"w",sInfo[i].width);
            extl_table_sets_i(rect,"h",sInfo[i].height);
            extl_table_seti_t(ret,i+1,rect);
        }

        XFree(sInfo);
        return ret;
    }
    return extl_table_none();
}
/* }}} */

/* {{{ Controlling notion screens from lua */

/* Set up one new screen 
 * @param screen dimensions (x/y/w/h) 
 * @returns true on success, false on failure
 */
EXTL_EXPORT
bool mod_xinerama_setup_new_screen(int screen_id, ExtlTab screen)
{
    WRootWin* rootWin = ioncore_g.rootwins;
    WMPlexAttachParams par = MPLEXATTACHPARAMS_INIT;
    WFitParams fp;

    WScreen* newScreen;
    WRegion* reg=NULL;

    extl_table_gets_i(screen,"x",&(fp.g.x));
    extl_table_gets_i(screen,"y",&(fp.g.y));
    extl_table_gets_i(screen,"w",&(fp.g.w));
    extl_table_gets_i(screen,"h",&(fp.g.h));
    fp.mode = REGION_FIT_EXACT;

#ifdef MOD_XINERAMA_DEBUG
    printf("Rectangle #%d: x=%d y=%d width=%u height=%u\n", 
           i, fp.g.x, fp.g.y, fp.g.w, fp.g.h);
#endif

    par.flags = MPLEX_ATTACH_GEOM|MPLEX_ATTACH_SIZEPOLICY|MPLEX_ATTACH_UNNUMBERED ;
    par.geom = fp.g;
    par.szplcy = SIZEPOLICY_FULL_EXACT;

    newScreen = (WScreen*) mplex_do_attach_new(&rootWin->scr.mplex, &par,
        (WRegionCreateFn*)create_screen, NULL);

    if(newScreen == NULL) {
        warn(TR("Unable to create Xinerama workspace %d."), screen_id);
        return FALSE;
    }

    newScreen->id = screen_id;
    rootWin->scr.id = -2;
    return TRUE;
 }

/* }}} */

/* {{{ Module init and deinit */
bool mod_xinerama_init()
{
    xinerama_ready = XineramaQueryExtension(ioncore_g.dpy,&xinerama_event_base, &xinerama_error_base);
    if (!xinerama_ready)
        warn(TR("No Xinerama support detected, mod_xinerama won't do anything."));

    return mod_xinerama_register_exports();
}


void mod_xinerama_deinit()
{
    mod_xinerama_unregister_exports();
}
/* }}} */
