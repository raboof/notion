/*
 * Ion xrandr module
 * Copyright (C) 2004 Ragnar Rova
 *               2005-2007 Tuomo Valkonen
 *
 * by Ragnar Rova <rr@mima.x.se>
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

#include <limits.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>

#include <libtu/rb.h>

#include <ioncore/common.h>
#include <ioncore/eventh.h>
#include <ioncore/global.h>
#include <ioncore/event.h>
#include <ioncore/mplex.h>
#include <ioncore/xwindow.h>
#include <ioncore/log.h>
#include <ioncore/../version.h>

#include "exports.h"

char mod_xrandr_ion_api_version[]=ION_API_VERSION;

WHook *randr_screen_change_notify=NULL;

static bool hasXrandR=FALSE;
static int xrr_event_base;
static int xrr_error_base;

Rb_node rotations=NULL;

static int rr2scrrot(int rr)
{
    switch(rr){
    case RR_Rotate_0: return SCREEN_ROTATION_0;
    case RR_Rotate_90: return SCREEN_ROTATION_90;
    case RR_Rotate_180: return SCREEN_ROTATION_180;
    case RR_Rotate_270: return SCREEN_ROTATION_270;
    default: return SCREEN_ROTATION_0;
    }
}


static void insrot(int id, int r)
{
    Rb_node node;

    node=rb_inserti(rotations, id, NULL);

    if(node!=NULL)
        node->v.ival=r;
}


bool handle_xrandr_event(XEvent *ev)
{
    if(hasXrandR && ev->type == xrr_event_base + RRScreenChangeNotify) {
        XRRScreenChangeNotifyEvent *rev=(XRRScreenChangeNotifyEvent *)ev;

        WFitParams fp;
        WScreen *screen;
        LOG(DEBUG, RANDR, "XRRScreenChangeNotifyEvent size %dx%d (%dx%d mm)",
            rev->width, rev->height, rev->mwidth, rev->mheight);

        screen=XWINDOW_REGION_OF_T(rev->root, WScreen);

        if(screen!=NULL){
            int r;
            Rb_node node;
            int found;

            r=rr2scrrot(rev->rotation);

            fp.g.x=REGION_GEOM(screen).x;
            fp.g.y=REGION_GEOM(screen).y;

            if(rev->rotation==RR_Rotate_90 || rev->rotation==RR_Rotate_270){
                fp.g.w=rev->height;
                fp.g.h=rev->width;
            }else{
                fp.g.w=rev->width;
                fp.g.h=rev->height;
            }

            fp.mode=REGION_FIT_EXACT;

            node=rb_find_ikey_n(rotations, screen->id, &found);

            if(!found){
                insrot(screen->id, r);
            }else if(r!=node->v.ival){
                int or=node->v.ival;

                fp.mode|=REGION_FIT_ROTATE;
                fp.rotation=(r>or
                             ? SCREEN_ROTATION_0+r-or
                             : (SCREEN_ROTATION_270+1)+r-or);
                node->v.ival=r;
            }

            REGION_GEOM(screen)=fp.g;

            mplex_managed_geom((WMPlex*)screen, &(fp.g));

            mplex_do_fit_managed((WMPlex*)screen, &fp);
        }
        hook_call_v(randr_screen_change_notify);

        return TRUE;
    }
    return FALSE;
}




static bool check_pivots()
{
    WScreen *scr;

    rotations=make_rb();

    if(rotations==NULL)
        return FALSE;

    FOR_ALL_SCREENS(scr){
        Rotation rot=RR_Rotate_90;
        int randr_screen_id = XRRRootToScreen(ioncore_g.dpy, ((WMPlex*) scr)->win.win);
        if (randr_screen_id != -1)
            XRRRotations(ioncore_g.dpy, randr_screen_id, &rot);

        insrot(scr->id, rr2scrrot(rot));
    }

    return TRUE;
}

#define INIT_HOOK_(NM)                             \
    NM=mainloop_register_hook(#NM, create_hook()); \
    if(NM==NULL) return FALSE

bool mod_xrandr_init()
{
    hasXrandR=
        XRRQueryExtension(ioncore_g.dpy,&xrr_event_base,&xrr_error_base);

    if(!check_pivots())
        return FALSE;

    if(hasXrandR){
        XRRSelectInput(ioncore_g.dpy,ioncore_g.rootwins->dummy_win,
                       RRScreenChangeNotifyMask);
    }else{
        warn_obj("mod_xrandr","XRandR is not supported on this display");
    }

    hook_add(ioncore_handle_event_alt,(WHookDummy *)handle_xrandr_event);

    INIT_HOOK_(randr_screen_change_notify);

    return mod_xrandr_register_exports();
}


bool mod_xrandr_deinit()
{
    hook_remove(ioncore_handle_event_alt,
                (WHookDummy *)handle_xrandr_event);

    return TRUE;
}

void add_output(ExtlTab result, XRROutputInfo *output_info, XRRCrtcInfo *crtc_info)
{
    ExtlTab details = extl_create_table();
    /* TODO we probably have to strdup the outputs' data here, because we free
     * the XRROutputInfo later on. However, where can we make sure the strings
     * get freed when the code calling mod_xrandr_get_outputs_for_geom is done
     * with it?
     */
    extl_table_sets_s(details, "name", strdup(output_info->name));
    extl_table_sets_i(details, "x", crtc_info->x);
    extl_table_sets_i(details, "y", crtc_info->y);
    extl_table_sets_i(details, "w", (int)crtc_info->width);
    extl_table_sets_i(details, "h", (int)crtc_info->height);
    extl_table_sets_t(result, strdup(output_info->name), details);
}

EXTL_SAFE
EXTL_EXPORT
ExtlTab mod_xrandr_get_all_outputs()
{
    int i;
    XRRScreenResources *res = XRRGetScreenResources(ioncore_g.dpy, ioncore_g.rootwins->dummy_win);
    ExtlTab result = extl_create_table();

    for(i=0; i < res->noutput; i++){
        XRROutputInfo *output_info = XRRGetOutputInfo(ioncore_g.dpy, res, res->outputs[i]);
        if(output_info->crtc != None){
            XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(ioncore_g.dpy, res, output_info->crtc);

            add_output(result, output_info, crtc_info);

            XRRFreeCrtcInfo(crtc_info);
        }
        XRRFreeOutputInfo(output_info);
    }

    return result;

}

/*EXTL_DOC
 * Queries the RandR extension for outputs with this geometry
 *
 * Returns a table with the matching RandR window names as keys
 */
EXTL_SAFE
EXTL_EXPORT
ExtlTab mod_xrandr_get_outputs_for_geom(ExtlTab geom)
{
    int i;
    XRRScreenResources *res = XRRGetScreenResources(ioncore_g.dpy, ioncore_g.rootwins->dummy_win);
    ExtlTab result = extl_create_table();

    for(i=0; i < res->noutput; i++){
        int x,y;
        int w,h;
        XRROutputInfo *output_info = XRRGetOutputInfo(ioncore_g.dpy, res, res->outputs[i]);
        if(output_info->crtc != None){
            XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(ioncore_g.dpy, res, output_info->crtc);

            extl_table_gets_i(geom, "x", &x);
            extl_table_gets_i(geom, "y", &y);
            extl_table_gets_i(geom, "w", &w);
            extl_table_gets_i(geom, "h", &h);
            if(x==crtc_info->x && y==crtc_info->y
               && w==(int)crtc_info->width && h==(int)crtc_info->height){
                add_output(result, output_info, crtc_info);
            }

            XRRFreeCrtcInfo(crtc_info);
        }
        XRRFreeOutputInfo(output_info);
    }

    return result;
}

