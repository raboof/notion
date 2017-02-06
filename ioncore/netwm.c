/*
 * ion/ioncore/netwm.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009.
 *
 * See the included file LICENSE for details.
 */

#include <X11/Xatom.h>
#include <X11/Xmd.h>

#include <limits.h>
#include <stdint.h>

#include <libtu/util.h>
#include <libtu/minmax.h>
#include "common.h"
#include "global.h"
#include "fullscreen.h"
#include "clientwin.h"
#include "netwm.h"
#include "property.h"
#include "activity.h"
#include "focus.h"
#include "xwindow.h"
#include "extlconv.h"
#include "group.h"
#include "event.h"


/*{{{ Atoms */

static Atom atom_net_wm_name=0;
static Atom atom_net_wm_state=0;
static Atom atom_net_wm_state_fullscreen=0;
static Atom atom_net_wm_state_demands_attention=0;
static Atom atom_net_supporting_wm_check=0;
static Atom atom_net_virtual_roots=0;
static Atom atom_net_active_window=0;
static Atom atom_net_wm_user_time=0;
static Atom atom_net_wm_allowed_actions=0;
static Atom atom_net_wm_moveresize=0;
static Atom atom_net_wm_icon=0;

#define N_NETWM 10

static Atom atom_net_supported=0;

#define SOURCE_UNKNOWN     0
#define SOURCE_APPLICATION 1
#define SOURCE_PAGER       2

/*}}}*/


/*{{{ Initialisation */


void netwm_init()
{
    atom_net_wm_name=XInternAtom(ioncore_g.dpy, "_NET_WM_NAME", False);
    atom_net_wm_state=XInternAtom(ioncore_g.dpy, "_NET_WM_STATE", False);
    atom_net_wm_state_fullscreen=XInternAtom(ioncore_g.dpy, "_NET_WM_STATE_FULLSCREEN", False);
    atom_net_wm_state_demands_attention=XInternAtom(ioncore_g.dpy, "_NET_WM_STATE_DEMANDS_ATTENTION", False);
    atom_net_supported=XInternAtom(ioncore_g.dpy, "_NET_SUPPORTED", False);
    atom_net_supporting_wm_check=XInternAtom(ioncore_g.dpy, "_NET_SUPPORTING_WM_CHECK", False);
    atom_net_virtual_roots=XInternAtom(ioncore_g.dpy, "_NET_VIRTUAL_ROOTS", False);
    atom_net_active_window=XInternAtom(ioncore_g.dpy, "_NET_ACTIVE_WINDOW", False);
    atom_net_wm_user_time=XInternAtom(ioncore_g.dpy, "_NET_WM_USER_TIME", False);
    atom_net_wm_allowed_actions=XInternAtom(ioncore_g.dpy, "_NET_WM_ALLOWED_ACTIONS", False);
    atom_net_wm_moveresize=XInternAtom(ioncore_g.dpy, "_NET_WM_MOVERESIZE", False);
    atom_net_wm_icon=XInternAtom(ioncore_g.dpy, "_NET_WM_ICON", False);
}


void netwm_init_rootwin(WRootWin *rw)
{
    Atom atoms[N_NETWM];
    const char *p[1];

    atoms[0]=atom_net_wm_name;
    atoms[1]=atom_net_wm_state;
    atoms[2]=atom_net_wm_state_fullscreen;
    atoms[3]=atom_net_wm_state_demands_attention;
    atoms[4]=atom_net_supporting_wm_check;
    atoms[5]=atom_net_virtual_roots;
    atoms[6]=atom_net_active_window;
    atoms[7]=atom_net_wm_allowed_actions;
    atoms[8]=atom_net_wm_moveresize;
    atoms[9]=atom_net_wm_icon;

    XChangeProperty(ioncore_g.dpy, WROOTWIN_ROOT(rw),
                    atom_net_supporting_wm_check, XA_WINDOW,
                    32, PropModeReplace, (uchar*)&(rw->dummy_win), 1);
    XChangeProperty(ioncore_g.dpy, rw->dummy_win,
                    atom_net_supporting_wm_check, XA_WINDOW,
                    32, PropModeReplace, (uchar*)&(rw->dummy_win), 1);
    XChangeProperty(ioncore_g.dpy, WROOTWIN_ROOT(rw),
                    atom_net_supported, XA_ATOM,
                    32, PropModeReplace, (uchar*)atoms, N_NETWM);

    p[0]=libtu_progbasename();
    /**
     * Unfortunately we cannot determine the charset of libtu_progbasename()
     * so we'll just have to guess it makes sense in the current locale charset
     */
    xwindow_set_utf8_property(rw->dummy_win, atom_net_wm_name, p, 1);
}


/*}}}*/


/*{{{ _NET_WM_STATE */


bool netwm_check_initial_fullscreen(WClientWin *cwin)
{

    int i, n;
    long *data;

    n=xwindow_get_property(cwin->win, atom_net_wm_state, XA_ATOM,
                   1, TRUE, (uchar**)&data);

    if(n<0)
        return FALSE;

    for(i=0; i<n; i++){
        if(data[i]==(long)atom_net_wm_state_fullscreen)
            return TRUE;
    }

    XFree((void*)data);

    return FALSE;
}

/*EXTL_DOC
 * refresh \_NET\_WM\_STATE markers for this window
 */
EXTL_SAFE
EXTL_EXPORT
void ioncore_update_net_state(WClientWin *cwin)
{
    netwm_update_state(cwin);
}

void netwm_update_state(WClientWin *cwin)
{
    CARD32 data[2];
    int n=0;

    if(REGION_IS_FULLSCREEN(cwin))
        data[n++]=atom_net_wm_state_fullscreen;
    if(region_is_activity_r(&(cwin->region)))
        data[n++]=atom_net_wm_state_demands_attention;

    XChangeProperty(ioncore_g.dpy, cwin->win, atom_net_wm_state,
                    XA_ATOM, 32, PropModeReplace, (uchar*)data, n);
}

void netwm_update_allowed_actions(WClientWin *cwin)
{
    CARD32 data[1];
    int n=0;

    /* TODO add support for 'resize' and list it here */
    /* TODO add support for 'minimize' and list it here */
    /* TODO add support for 'maximize_horz' and list it here */
    /* TODO add support for 'maximize_vert' and list it here */
    /* TODO add support for 'fullscreen' and list it here */
    /* TODO add support for 'change desktop' and list it here */
    /* TODO add support for 'close' and list it here */
    /* TODO add support for 'above' and list it here */
    /* TODO add support for 'below' and list it here */

    XChangeProperty(ioncore_g.dpy, cwin->win, atom_net_wm_allowed_actions,
                    XA_ATOM, 32, PropModeReplace, (uchar*)data, n);
}


void netwm_delete_state(WClientWin *cwin)
{
    XDeleteProperty(ioncore_g.dpy, cwin->win, atom_net_wm_state);
}



static void netwm_state_change_rq(WClientWin *cwin,
                                  const XClientMessageEvent *ev)
{
    if((ev->data.l[1]==0 ||
        ev->data.l[1]!=(long)atom_net_wm_state_fullscreen) &&
       (ev->data.l[2]==0 ||
        ev->data.l[2]!=(long)atom_net_wm_state_fullscreen)){
        return;
    }

    /* Ok, full screen add/remove/toggle */
    if(!REGION_IS_FULLSCREEN(cwin)){
        if(ev->data.l[0]==_NET_WM_STATE_ADD ||
           ev->data.l[0]==_NET_WM_STATE_TOGGLE){
            WRegion *grp=region_groupleader_of((WRegion*)cwin);
            bool sw=clientwin_fullscreen_may_switchto(cwin);
            cwin->flags|=CLIENTWIN_FS_RQ;
            if(!region_enter_fullscreen(grp, sw))
                cwin->flags&=~CLIENTWIN_FS_RQ;
        }else{
            /* Should not be set.. */
            cwin->flags&=~CLIENTWIN_FS_RQ;
        }
    }else{
        if(ev->data.l[0]==_NET_WM_STATE_REMOVE ||
           ev->data.l[0]==_NET_WM_STATE_TOGGLE){
            WRegion *grp=region_groupleader_of((WRegion*)cwin);
            bool sw=clientwin_fullscreen_may_switchto(cwin);
            cwin->flags&=~CLIENTWIN_FS_RQ;
            region_leave_fullscreen(grp, sw);
        }else{
            /* Set the flag */
            cwin->flags|=CLIENTWIN_FS_RQ;
        }
    }
}


/*}}}*/


/*{{{ _NET_ACTIVE_WINDOW */


void netwm_set_active(WRegion *reg)
{
    CARD32 data[1]={None};

    if(OBJ_IS(reg, WClientWin))
        data[0]=region_xwindow(reg);

    /* The spec doesn't say how multihead should be handled, so
     * we just update the root window the window is on.
     */
    XChangeProperty(ioncore_g.dpy, region_root_of(reg),
                    atom_net_active_window, XA_WINDOW,
                    32, PropModeReplace, (uchar*)data, 1);
}


static void netwm_active_window_rq(WClientWin *cwin,
                                   const XClientMessageEvent *ev)
{
    long source=ev->data.l[0];
    /**
     * By default we ignore non-pager activity requests, as they're known to
     * steal focus from newly-created windows :(
     */
    bool ignore=source!=SOURCE_PAGER;

    extl_table_gets_b(cwin->proptab, "ignore_net_active_window", &ignore);

    if(!ignore)
        region_goto((WRegion*)cwin);
    else
        region_set_activity((WRegion*)cwin, SETPARAM_SET);
}


/*}}}*/


/*{{{ _NET_WM_NAME */


char **netwm_get_name(WClientWin *cwin)
{
    return xwindow_get_text_property(cwin->win, atom_net_wm_name, NULL);
}


/*}}}*/


/*{{{ netwm_handle_client_message */


void netwm_handle_client_message(const XClientMessageEvent *ev)
{
    /* Check _NET_WM_STATE fullscreen request */
    if(ev->message_type==atom_net_wm_state && ev->format==32){
        WClientWin *cwin=XWINDOW_REGION_OF_T(ev->window, WClientWin);
        if(cwin!=NULL)
            netwm_state_change_rq(cwin, ev);
    }
    /* Check _NET_ACTIVE_WINDOW request */
    else if(ev->message_type==atom_net_active_window && ev->format==32){
        WClientWin *cwin=XWINDOW_REGION_OF_T(ev->window, WClientWin);
        if(cwin!=NULL)
            netwm_active_window_rq(cwin, ev);
    }
}


/*}}}*/


/*{{{ netwm_handle_property */


bool netwm_handle_property(WClientWin *cwin, const XPropertyEvent *ev)
{
    if(ev->atom==atom_net_wm_name){
        clientwin_get_set_name(cwin);
        return TRUE;
    } else if(ev->atom==atom_net_wm_icon){
        clientwin_refresh_icon(cwin);
        return TRUE;
    }

    return FALSE;
}


/*}}}*/


/*{{{ _NET_WM_ICON stuff */

static cairo_user_data_key_t cairo_userdata_key;


/** Create a surface object from this image data.
 * \param width The width of the image.
 * \param height The height of the image
 * \param data The image's data in ARGB format, will be copied by this function.
 *
 * Adapted from awesome-wm
 */
cairo_surface_t *draw_surface_from_data(int width, int height, ulong *data)
{
    unsigned long int len = width * height;
    unsigned long int i;
    uint32_t *buffer = ALLOC_N(uint32_t, len);
    cairo_surface_t *surface;

    /* Cairo wants premultiplied alpha, meh :( */
    for(i = 0; i < len; i++)
    {
        uint32_t cardinal = data[i];
        uint8_t a = (cardinal >> 24) & 0xff;
        double alpha = a / 255.0;
        uint8_t r = ((cardinal >> 16) & 0xff) * alpha;
        uint8_t g = ((cardinal >>  8) & 0xff) * alpha;
        uint8_t b = ((cardinal >>  0) & 0xff) * alpha;
        buffer[i] = (a << 24) | (r << 16) | (g << 8) | b;
    }

    surface =
        cairo_image_surface_create_for_data((unsigned char *) buffer,
                                            CAIRO_FORMAT_ARGB32,
                                            width,
                                            height,
                                            width*4);
    /* This makes sure that buffer will be freed */
    cairo_surface_set_user_data(surface, &cairo_userdata_key, buffer, &free);

    return surface;
}


/**
 * Extracts the icon from _NET_WM_ICON with size closest matching 'preffered_size'
 * Returns NULL if no icon is found.
 * 
 * Adapted from awesome-wm
 */
cairo_surface_t * netwm_window_icon(WClientWin *cwin, uint preferred_size)
{
    cairo_surface_t *ret=NULL;
    ulong *data=NULL, *end, *found_data = 0;
    uint found_size = 0;

    Atom real_type;
    ulong length=-1, extra=0;
    int status;

    int format;


    status=XGetWindowProperty(ioncore_g.dpy, cwin->win, atom_net_wm_icon, 0L, UINT_MAX,
                              False, XA_CARDINAL, &real_type, &format, &length,
                              &extra, (uchar**)&data);

    /* In theory the server could've returned another type than requested, but I
     * assume that doesn't happen in practice since 'xwindow_get_property'
     * doesn't expose that aspect */
    /* int length = xwindow_get_property(cwin->win, atom_net_wm_icon, XA_CARDINAL, 128*128L, TRUE, p); */

    if(length < 2) {
        goto cleanup;
    }

    if (!data) {
        fprintf(stderr, "_NET_WM_ICON but no data? %d", cwin->win);
        goto cleanup;
    }

    end = data + length;

    ulong *data_iter=data;

    /* Goes over the icon data and picks the icon that best matches the size preference.
     * In case the size match is not exact, picks the closest bigger size if present,
     * closest smaller size otherwise.
     */
    while (data_iter + 1 < end) {
        /* check whether the data_iter size specified by width and height fits into the array we got */
        ulong data_size = (ulong) data_iter[0] * data_iter[1];
        if (data_size > (ulong) (end - data_iter - 2)) {
            break;   
        }

        /* use the greater of the two dimensions to match against the preferred size */
        uint size = MAXOF(data_iter[0], data_iter[1]);

        /* pick the icon if it's a better match than the one we already have */
        bool found_icon_too_small = found_size < preferred_size;
        bool found_icon_too_large = found_size > preferred_size;
        bool icon_empty = data_iter[0] == 0 || data_iter[1] == 0;
        bool better_because_bigger =  found_icon_too_small && size > found_size;
        bool better_because_smaller = found_icon_too_large &&
            size >= preferred_size && size < found_size;
        if (!icon_empty && (better_because_bigger || better_because_smaller || found_size == 0))
        {
            found_data = data_iter;
            found_size = size;
        }

        data_iter += data_size + 2;
    }

    if (!found_data) {
        goto cleanup;
    }

    ret=draw_surface_from_data(found_data[0], found_data[1], found_data + 2);

 cleanup:
    if(data)
        XFree((void*)data);
    return ret;
}

/*}}}*/

/*{{{ user time */

/** When a new window is mapped, look at the netwm user time to find out
 * whether the new window should be switched to and get the focus.
 *
 * It is unclear what the desired behavior would be, and how we should takee
 * into consideration ioncore_g.usertime_diff_new and IONCORE_CLOCK_SKEW_MS,
 * so for now we deny raising the new window only in the special case where
 * its user time is set to 0, specifically preventing it from being raised.
 */
void netwm_check_manage_user_time(WClientWin *cwin, WManageParams *param)
{
    /* the currently focussed window */
    WClientWin *cur=OBJ_CAST(ioncore_g.focus_current, WClientWin);
    /* the new window */
    Window win=region_xwindow((WRegion*)cwin);
    /* user time */
    CARD32 ut=0;
    /* whether the new (got) and current (gotcut) windows had their usertime
     * set */
    bool got=FALSE, gotcut=FALSE;
    bool nofocus=FALSE;

    if(cur!=NULL){
        Window curwin;
        /* current window user time */
        CARD32 cut;
        if(param->tfor==cur)
            return;
        curwin=region_xwindow((WRegion*)cur);
        gotcut=xwindow_get_cardinal_property(curwin, atom_net_wm_user_time, &cut);
    }

    got=xwindow_get_cardinal_property(win, atom_net_wm_user_time, &ut);

    /* The special value of zero on a newly mapped window can be used to
     * request that the window not be initially focused when it is mapped */
    if (got && ut == 0)
        nofocus = TRUE;

    /* there was some other logic here, but it was not clear how it was meant
     * to work and prevented newly created windows from receiving the focus
     * in some cases
     * (https://sourceforge.net/tracker/?func=detail&aid=3109576&group_id=314802&atid=1324528)
     * Stripped until we decide how this is supposed to behave.
     */

    if(nofocus){
        param->switchto=FALSE;
        param->jumpto=FALSE;
    }
}


/*}}}*/

/*{{{ _NET_WM_VIRTUAL_ROOTS */

int count_screens()
{
    int result = 0;
    WScreen *scr;

    FOR_ALL_SCREENS(scr){
        result++;
    }

    return result;
}

/*EXTL_DOC
 * refresh \_NET\_WM\_VIRTUAL\_ROOTS
 */
EXTL_SAFE
EXTL_EXPORT
void ioncore_screens_updated(WRootWin *rw)
{
    int current_screen = 0;
    int n_screens;

    long *virtualroots;
    WScreen *scr;

    n_screens = count_screens();
    virtualroots = (long*)malloc(n_screens * sizeof(long));

    FOR_ALL_SCREENS(scr){
        virtualroots[current_screen] = region_xwindow((WRegion *)scr);
        current_screen++;
    }

    XChangeProperty(ioncore_g.dpy, WROOTWIN_ROOT(rw),
                    atom_net_virtual_roots, XA_WINDOW,
                    32, PropModeReplace, (uchar*)virtualroots, n_screens);

    free(virtualroots);
}

/*}}}*/
