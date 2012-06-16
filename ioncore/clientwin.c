/*
 * ion/ioncore/clientwin.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <limits.h>
#include <ctype.h>

#include <libtu/objp.h>
#include <libtu/minmax.h>
#include <libextl/extl.h>
#include <libmainloop/defer.h>
#include <libmainloop/hooks.h>
#include "common.h"
#include "global.h"
#include "property.h"
#include "focus.h"
#include "sizehint.h"
#include "event.h"
#include "clientwin.h"
#include "colormap.h"
#include "resize.h"
#include "attach.h"
#include "regbind.h"
#include "bindmaps.h"
#include "names.h"
#include "saveload.h"
#include "manage.h"
#include "extlconv.h"
#include "fullscreen.h"
#include "event.h"
#include "rootwin.h"
#include "activity.h"
#include "netwm.h"
#include "xwindow.h"
#include "bindmaps.h"
#include "return.h"
#include "conf.h"
#include "group.h"


static void set_clientwin_state(WClientWin *cwin, int state);
static bool send_clientmsg(Window win, Atom a, Time stmp);


WHook *clientwin_do_manage_alt=NULL;
WHook *clientwin_mapped_hook=NULL;
WHook *clientwin_unmapped_hook=NULL;
WHook *clientwin_property_change_hook=NULL;


/*{{{ Get properties */


void clientwin_get_protocols(WClientWin *cwin)
{
    Atom *protocols=NULL, *p;
    int n;
    
    cwin->flags&=~(CLIENTWIN_P_WM_DELETE|CLIENTWIN_P_WM_TAKE_FOCUS);
    
    if(!XGetWMProtocols(ioncore_g.dpy, cwin->win, &protocols, &n))
        return;
    
    for(p=protocols; n; n--, p++){
        if(*p==ioncore_g.atom_wm_delete)
            cwin->flags|=CLIENTWIN_P_WM_DELETE;
        else if(*p==ioncore_g.atom_wm_take_focus)
            cwin->flags|=CLIENTWIN_P_WM_TAKE_FOCUS;
    }
    
    if(protocols!=NULL)
        XFree((char*)protocols);
}


static WSizePolicy get_sizepolicy_winprop(WClientWin *cwin,
                                          const char *propname,
                                          WSizePolicy value)
{
    char *szplcy;

    if(extl_table_gets_s(cwin->proptab, propname, &szplcy)){
        string2sizepolicy(szplcy, &value);
	free(szplcy);
    }
    return value;
}


#define SIZEHINT_PROPS (CLIENTWIN_PROP_MAXSIZE|   \
                        CLIENTWIN_PROP_MINSIZE|   \
                        CLIENTWIN_PROP_ASPECT|    \
                        CLIENTWIN_PROP_RSZINC|    \
                        CLIENTWIN_PROP_I_MAXSIZE| \
                        CLIENTWIN_PROP_I_MINSIZE| \
                        CLIENTWIN_PROP_I_ASPECT|  \
                        CLIENTWIN_PROP_I_RSZINC)


#define DO_SZH(NAME, FLAG, IFLAG, SZHFLAG, W, H, C)  \
    if(extl_table_is_bool_set(tab, "ignore_" NAME)){ \
        cwin->flags|=IFLAG;                          \
    }else if(extl_table_gets_t(tab, NAME, &tab2)){   \
        if(extl_table_gets_i(tab2, "w", &i1) &&      \
           extl_table_gets_i(tab2, "h", &i2)){       \
            cwin->size_hints.W=i1;                   \
            cwin->size_hints.H=i2;                   \
            C                                        \
            cwin->size_hints.flags|=SZHFLAG;         \
            cwin->flags|=FLAG;                       \
        }                                            \
        extl_unref_table(tab2);                      \
    }


static void clientwin_get_winprops(WClientWin *cwin)
{
    ExtlTab tab, tab2;
    int i1, i2;
    
    tab=ioncore_get_winprop(cwin);
    
    cwin->proptab=tab;
    
    if(tab==extl_table_none())
        return;

    if(extl_table_is_bool_set(tab, "transparent"))
        cwin->flags|=CLIENTWIN_PROP_TRANSPARENT;

    if(extl_table_is_bool_set(tab, "acrobatic"))
        cwin->flags|=CLIENTWIN_PROP_ACROBATIC;
    
    if(extl_table_is_bool_set(tab, "lazy_resize"))
        cwin->flags|=CLIENTWIN_PROP_LAZY_RESIZE;
    
    DO_SZH("max_size", CLIENTWIN_PROP_MAXSIZE, CLIENTWIN_PROP_I_MAXSIZE,
           PMaxSize, max_width, max_height, { });
           
    DO_SZH("min_size", CLIENTWIN_PROP_MINSIZE, CLIENTWIN_PROP_I_MINSIZE,
           PMinSize, min_width, min_height, { });
           
    DO_SZH("resizeinc", CLIENTWIN_PROP_RSZINC, CLIENTWIN_PROP_I_RSZINC,
           PResizeInc, width_inc, height_inc, { });

    DO_SZH("aspect", CLIENTWIN_PROP_ASPECT, CLIENTWIN_PROP_I_ASPECT,
           PAspect, min_aspect.x, min_aspect.y, 
           { cwin->size_hints.max_aspect.x=i1;
             cwin->size_hints.max_aspect.y=i2;
           });
           
    if(extl_table_is_bool_set(tab, "ignore_cfgrq"))
        cwin->flags|=CLIENTWIN_PROP_IGNORE_CFGRQ;

#if 0    
    cwin->szplcy=get_sizepolicy_winprop(cwin, "sizepolicy", 
                                        SIZEPOLICY_DEFAULT);
    cwin->transient_szplcy=get_sizepolicy_winprop(cwin, 
                                                  "transient_sizepolicy",
                                                  DFLT_SZPLCY);
#endif
}


void clientwin_get_size_hints(WClientWin *cwin)
{
    XSizeHints tmp=cwin->size_hints;
    
    xwindow_get_sizehints(cwin->win, &(cwin->size_hints));
    
    if(cwin->flags&CLIENTWIN_PROP_I_MAXSIZE){
        cwin->size_hints.flags&=~PMaxSize;
    }else if(cwin->flags&CLIENTWIN_PROP_MAXSIZE){
        cwin->size_hints.max_width=tmp.max_width;
        cwin->size_hints.max_height=tmp.max_height;
        cwin->size_hints.flags|=PMaxSize;
    }
    
    if(cwin->flags&CLIENTWIN_PROP_I_MINSIZE){
        cwin->size_hints.flags&=~PMinSize;
    }else if(cwin->flags&CLIENTWIN_PROP_MINSIZE){
        cwin->size_hints.min_width=tmp.min_width;
        cwin->size_hints.min_height=tmp.min_height;
        cwin->size_hints.flags|=PMinSize;
    }
    
    if(cwin->flags&CLIENTWIN_PROP_I_ASPECT){
        cwin->size_hints.flags&=~PAspect;
    }else if(cwin->flags&CLIENTWIN_PROP_ASPECT){
        cwin->size_hints.min_aspect=tmp.min_aspect;
        cwin->size_hints.max_aspect=tmp.max_aspect;
        cwin->size_hints.flags|=PAspect;
    }
    
    if(cwin->flags&CLIENTWIN_PROP_I_RSZINC){
        cwin->size_hints.flags&=~PResizeInc;
    }else if(cwin->flags&CLIENTWIN_PROP_RSZINC){
        cwin->size_hints.width_inc=tmp.width_inc;
        cwin->size_hints.height_inc=tmp.height_inc;
        cwin->size_hints.flags|=PResizeInc;
    }
}


void clientwin_get_set_name(WClientWin *cwin)
{
    char **list=NULL;
    int n=0;
    
    if(ioncore_g.use_mb)
        list=netwm_get_name(cwin);

    if(list==NULL){
        list=xwindow_get_text_property(cwin->win, XA_WM_NAME, &n);
    }else{
        cwin->flags|=CLIENTWIN_USE_NET_WM_NAME;
    }

    if(list==NULL){
        /* Special condition kludge: property exists, but couldn't
         * be converted to a string list.
         */
        clientwin_set_name(cwin, (n==-1 ? "???" : NULL));
    }else{
        clientwin_set_name(cwin, *list);
        XFreeStringList(list);
    }
}


/* Some standard winprops */


bool clientwin_get_switchto(const WClientWin *cwin)
{
    bool b;
    
    if(ioncore_g.opmode==IONCORE_OPMODE_INIT)
        return FALSE;
    
    if(extl_table_gets_b(cwin->proptab, "switchto", &b))
        return b;
    
    return ioncore_g.switchto_new;
}


int clientwin_get_transient_mode(const WClientWin *cwin)
{
    char *s;
    int mode=TRANSIENT_MODE_NORMAL;
    
    if(extl_table_gets_s(cwin->proptab, "transient_mode", &s)){
        if(strcmp(s, "current")==0)
            mode=TRANSIENT_MODE_CURRENT;
        else if(strcmp(s, "off")==0)
            mode=TRANSIENT_MODE_OFF;
        free(s);
    }
    return mode;
}


/*}}}*/


/*{{{ Manage/create */


static void configure_cwin_bw(Window win, int bw)
{
    XWindowChanges wc;
    ulong wcmask=CWBorderWidth;
    
    wc.border_width=bw;
    XConfigureWindow(ioncore_g.dpy, win, wcmask, &wc);
}


static void set_sane_gravity(Window win)
{
    XSetWindowAttributes attr;
    
    attr.win_gravity=NorthWestGravity;
    
    XChangeWindowAttributes(ioncore_g.dpy, win,
                            CWWinGravity, &attr);
}


static bool clientwin_init(WClientWin *cwin, WWindow *par, Window win,
                           XWindowAttributes *attr)
{
    WFitParams fp;

    cwin->flags=0;
    cwin->win=win;
    cwin->state=WithdrawnState;
    
    fp.g.x=attr->x;
    fp.g.y=attr->y;
    fp.g.w=attr->width;
    fp.g.h=attr->height;
    fp.mode=REGION_FIT_EXACT;
    
    /* The idiot who invented special server-supported window borders that
     * are not accounted for in the window size should be "taken behind a
     * sauna".
     */
    cwin->orig_bw=attr->border_width;
    configure_cwin_bw(cwin->win, 0);
    if(cwin->orig_bw!=0 && cwin->size_hints.flags&PWinGravity){
        fp.g.x+=xgravity_deltax(cwin->size_hints.win_gravity, 
                               -cwin->orig_bw, -cwin->orig_bw);
        fp.g.y+=xgravity_deltay(cwin->size_hints.win_gravity, 
                               -cwin->orig_bw, -cwin->orig_bw);
    }
    
    set_sane_gravity(cwin->win);

    cwin->n_cmapwins=0;
    cwin->cmap=attr->colormap;
    cwin->cmaps=NULL;
    cwin->cmapwins=NULL;
    cwin->n_cmapwins=0;
    cwin->event_mask=IONCORE_EVENTMASK_CLIENTWIN;

    region_init(&(cwin->region), par, &fp);

    cwin->region.flags|=REGION_GRAB_ON_PARENT;
    region_add_bindmap(&cwin->region, ioncore_clientwin_bindmap);
        
    XSelectInput(ioncore_g.dpy, win, cwin->event_mask);

    clientwin_register(cwin);
    clientwin_get_set_name(cwin);
    clientwin_get_colormaps(cwin);
    clientwin_get_protocols(cwin);
    clientwin_get_winprops(cwin);
    clientwin_get_size_hints(cwin);

    netwm_update_allowed_actions(cwin);
    
    XSaveContext(ioncore_g.dpy, win, ioncore_g.win_context, (XPointer)cwin);
    XAddToSaveSet(ioncore_g.dpy, win);

    return TRUE;
}


static WClientWin *create_clientwin(WWindow *par, Window win,
                                    XWindowAttributes *attr)
{
    CREATEOBJ_IMPL(WClientWin, clientwin, (p, par, win, attr));
}



WClientWin *clientwin_get_transient_for(const WClientWin *cwin)
{
    Window tforwin;
    WClientWin *tfor=NULL;
    
    if(clientwin_get_transient_mode(cwin)!=TRANSIENT_MODE_NORMAL)
        return NULL;

    if(!XGetTransientForHint(ioncore_g.dpy, cwin->win, &tforwin))
        return NULL;
    
    if(tforwin==None)
        return NULL;
    
    tfor=XWINDOW_REGION_OF_T(tforwin, WClientWin);
    
    if(tfor==cwin){
        warn(TR("The transient_for hint for \"%s\" points to itself."),
             region_name((WRegion*)cwin));
    }else if(tfor==NULL){
        if(xwindow_region_of(tforwin)!=NULL){
            warn(TR("Client window \"%s\" has broken transient_for hint. "
                    "(\"Extended WM hints\" multi-parent brain damage?)"),
                 region_name((WRegion*)cwin));
        }
    }else if(!region_same_rootwin((WRegion*)cwin, (WRegion*)tfor)){
        warn(TR("The transient_for window for \"%s\" is not on the same "
                "screen."), region_name((WRegion*)cwin));
    }else{
        return tfor;
    }
    
    return NULL;
}


static bool postmanage_check(WClientWin *cwin, XWindowAttributes *attr)
{
    /* Check that the window exists. The previous check and selectinput
     * do not seem to catch all cases of window destroyal.
     */
    XSync(ioncore_g.dpy, False);
    
    if(XGetWindowAttributes(ioncore_g.dpy, cwin->win, attr))
        return TRUE;
    
    warn(TR("Window %#x disappeared."), cwin->win);
    
    return FALSE;
}


static bool do_manage_mrsh(bool (*fn)(WClientWin *cwin, WManageParams *pm),
                           void **p)
{
    return fn((WClientWin*)p[0], (WManageParams*)p[1]);
}



static bool do_manage_mrsh_extl(ExtlFn fn, void **p)
{
    WClientWin *cwin=(WClientWin*)p[0];
    WManageParams *mp=(WManageParams*)p[1];
    ExtlTab t=manageparams_to_table(mp);
    bool ret=FALSE;
    
    extl_call(fn, "ot", "b", cwin, t, &ret);
    
    extl_unref_table(t);
    
    return (ret && REGION_MANAGER(cwin)!=NULL);
}


/* This is called when a window is mapped on the root window.
 * We want to check if we should manage the window and how and
 * act appropriately.
 */
WClientWin* ioncore_manage_clientwin(Window win, bool maprq)
{
    WRootWin *rootwin;
    WClientWin *cwin=NULL;
    XWindowAttributes attr;
    XWMHints *hints;
    int init_state=NormalState;
    WManageParams param=MANAGEPARAMS_INIT;
    void *mrshpm[2];

    param.dockapp=FALSE;
    
again:
    /* Is the window already being managed? */
    cwin=XWINDOW_REGION_OF_T(win, WClientWin);
    if(cwin!=NULL)
        return cwin;
    
    /* Select for UnmapNotify and DestroyNotify as the
     * window might get destroyed or unmapped in the meanwhile.
     */
    xwindow_unmanaged_selectinput(win, StructureNotifyMask);

    
    /* Is it a dockapp?
     */
    hints=XGetWMHints(ioncore_g.dpy, win);

    if(hints!=NULL && hints->flags&StateHint)
        init_state=hints->initial_state;
    
    if(!param.dockapp && init_state==WithdrawnState && 
       hints->flags&IconWindowHint && hints->icon_window!=None){
        /* The dockapp might be displaying its "main" window if no
         * wm that understands dockapps has been managing it.
         */
        if(!maprq)
            XUnmapWindow(ioncore_g.dpy, win);
        
        xwindow_unmanaged_selectinput(win, 0);
        
        /* Copy WM_CLASS as _ION_DOCKAPP_HACK */
        {
            char **p=NULL;
            int n=0;
            
            p=xwindow_get_text_property(win, XA_WM_CLASS, &n);
            
            if(p!=NULL){
                xwindow_set_text_property(hints->icon_window, 
                                          ioncore_g.atom_dockapp_hack,
                                          (const char **)p, n);
                XFreeStringList(p);
            }else{
                const char *pdummy[2]={"unknowndockapp", "UnknownDockapp"};
                xwindow_set_text_property(hints->icon_window, 
                                          ioncore_g.atom_dockapp_hack,
                                          pdummy, 2);
            }
        }
        
        /* It is a dockapp, do everything again from the beginning, now
         * with the icon window.
         */
        win=hints->icon_window;
        param.dockapp=TRUE;
        goto again;
    }
    
    if(hints!=NULL)
        XFree((void*)hints);

    if(!XGetWindowAttributes(ioncore_g.dpy, win, &attr)){
        if(maprq)
            warn(TR("Window %#x disappeared."), win);
        goto fail2;
    }
    
    attr.width=maxof(attr.width, 1);
    attr.height=maxof(attr.height, 1);

    /* Do we really want to manage it? */
    if(!param.dockapp && (attr.override_redirect || 
        (!maprq && attr.map_state!=IsViewable))){
        goto fail2;
    }

    /* Find root window */
    FOR_ALL_ROOTWINS(rootwin){
        if(WROOTWIN_ROOT(rootwin)==attr.root)
            break;
    }

    if(rootwin==NULL){
        warn(TR("Unable to find a matching root window!"));
        goto fail2;
    }

    /* Allocate and initialize */
    cwin=create_clientwin((WWindow*)rootwin, win, &attr);
    
    if(cwin==NULL){
        warn_err();
        goto fail2;
    }

    param.geom=REGION_GEOM(cwin);
    param.maprq=maprq;
    param.switchto=(init_state!=IconicState && clientwin_get_switchto(cwin));
    param.jumpto=extl_table_is_bool_set(cwin->proptab, "jumpto");
    param.gravity=(cwin->size_hints.flags&PWinGravity
                   ? cwin->size_hints.win_gravity
                   : ForgetGravity);
    param.tfor=clientwin_get_transient_for(cwin);
    
    if(!extl_table_gets_b(cwin->proptab, "userpos", &param.userpos))
        param.userpos=(cwin->size_hints.flags&USPosition);
    
    if(cwin->flags&SIZEHINT_PROPS){
        /* If size hints have been messed with, readjust requested geometry
         * here. If programs themselves give incompatible geometries and
         * things don't look good then, it's their fault.
         */
        region_size_hints_correct((WRegion*)cwin, &param.geom.w, &param.geom.h,
                                  FALSE);
    }

    mrshpm[0]=cwin;
    mrshpm[1]=&param;
        
    if(!hook_call_alt(clientwin_do_manage_alt, &mrshpm, 
                      (WHookMarshall*)do_manage_mrsh,
                      (WHookMarshallExtl*)do_manage_mrsh_extl)){
        warn(TR("Unable to manage client window %#x."), win);
        goto failure;
    }
    
    if(ioncore_g.opmode==IONCORE_OPMODE_NORMAL &&
       !region_is_fully_mapped((WRegion*)cwin) && 
       !region_skip_focus((WRegion*)cwin)){
        region_set_activity((WRegion*)cwin, SETPARAM_SET);
    }
    
    if(postmanage_check(cwin, &attr)){
        if(param.jumpto && ioncore_g.focus_next==NULL)
            region_goto((WRegion*)cwin);
        hook_call_o(clientwin_mapped_hook, (Obj*)cwin);
        return cwin;
    }

failure:
    clientwin_destroyed(cwin);
    return NULL;

fail2:
    xwindow_unmanaged_selectinput(win, 0);
    return NULL;
}


void clientwin_tfor_changed(WClientWin *cwin)
{
#if 0
    WManageParams param=MANAGEPARAMS_INIT;
    bool succeeded=FALSE;
    param.tfor=clientwin_get_transient_for(cwin);
    if(param.tfor==NULL)
        return;
    
    region_rootpos((WRegion*)cwin, &(param.geom.x), &(param.geom.y));
    param.geom.w=REGION_GEOM(cwin).w;
    param.geom.h=REGION_GEOM(cwin).h;
    param.maprq=FALSE;
    param.userpos=FALSE;
    param.switchto=region_may_control_focus((WRegion*)cwin);
    param.jumpto=extl_table_is_bool_set(cwin->proptab, "jumpto");
    param.gravity=ForgetGravity;
    
    CALL_ALT_B(succeeded, clientwin_do_manage_alt, (cwin, &param));
    warn("WM_TRANSIENT_FOR changed for \"%s\".",
         region_name((WRegion*)cwin));
#else
    warn(TR("Changes is WM_TRANSIENT_FOR property are unsupported."));
#endif        
}


/*}}}*/


/*{{{ Unmanage/destroy */


static bool reparent_root(WClientWin *cwin)
{
    XWindowAttributes attr;
    WWindow *par;
    Window dummy;
    int x=0, y=0;
    
    if(!XGetWindowAttributes(ioncore_g.dpy, cwin->win, &attr))
        return FALSE;
    
    par=REGION_PARENT(cwin);
    
    if(par==NULL){
        x=REGION_GEOM(cwin).x;
        y=REGION_GEOM(cwin).y;
    }else{
        int dr=REGION_GEOM(par).w-REGION_GEOM(cwin).w-REGION_GEOM(cwin).x;
        int db=REGION_GEOM(par).h-REGION_GEOM(cwin).h-REGION_GEOM(cwin).y;
        dr=maxof(dr, 0);
        db=maxof(db, 0);
        
        XTranslateCoordinates(ioncore_g.dpy, par->win, attr.root, 0, 0, 
                              &x, &y, &dummy);

        x-=xgravity_deltax(cwin->size_hints.win_gravity, 
                           maxof(0, REGION_GEOM(cwin).x), dr);
        y-=xgravity_deltay(cwin->size_hints.win_gravity, 
                           maxof(0, REGION_GEOM(cwin).y), db);
    }
    
    XReparentWindow(ioncore_g.dpy, cwin->win, attr.root, x, y);
    
    return TRUE;
}


void clientwin_deinit(WClientWin *cwin)
{
    WRegion *reg;
    
    if(cwin->win!=None){
        region_pointer_focus_hack(&cwin->region);

        xwindow_unmanaged_selectinput(cwin->win, 0);
        XUnmapWindow(ioncore_g.dpy, cwin->win);
        
        if(cwin->orig_bw!=0)
            configure_cwin_bw(cwin->win, cwin->orig_bw);
        
        if(reparent_root(cwin)){
            if(ioncore_g.opmode==IONCORE_OPMODE_DEINIT){
                XMapWindow(ioncore_g.dpy, cwin->win);
                /* Make sure the topmost window has focus; it doesn't really
                 * matter which one has as long as some has.
                 */
                xwindow_do_set_focus(cwin->win);
            }else{
                set_clientwin_state(cwin, WithdrawnState);
                netwm_delete_state(cwin);
            }
        }
        
        XRemoveFromSaveSet(ioncore_g.dpy, cwin->win);
        XDeleteContext(ioncore_g.dpy, cwin->win, ioncore_g.win_context);
    }
    
    clientwin_clear_colormaps(cwin);
    
    region_deinit((WRegion*)cwin);
}



static bool mrsh_u_c(WHookDummy *fn, void *param)
{
    fn(*(Window*)param);
    return TRUE;
}

static bool mrsh_u_extl(ExtlFn fn, void *param)
{
    double d=*(Window*)param;
    extl_call(fn, "d", NULL, d);
    return TRUE;
}

static void clientwin_do_unmapped(WClientWin *cwin, Window win)
{
    cwin->flags|=CLIENTWIN_UNMAP_RQ;
    
    /* First try a graceful chain-dispose */
    if(!region_rqdispose((WRegion*)cwin)){
        /* But force dispose anyway */
        region_dispose((WRegion*)cwin);
    }
    
    hook_call(clientwin_unmapped_hook, &win, mrsh_u_c, mrsh_u_extl);
}

/* Used when the window was unmapped */
void clientwin_unmapped(WClientWin *cwin)
{
    clientwin_do_unmapped(cwin, cwin->win);
}


/* Used when the window was deastroyed */
void clientwin_destroyed(WClientWin *cwin)
{
    Window win=cwin->win;
    XRemoveFromSaveSet(ioncore_g.dpy, cwin->win);
    XDeleteContext(ioncore_g.dpy, cwin->win, ioncore_g.win_context);
    xwindow_unmanaged_selectinput(cwin->win, 0);
    cwin->win=None;
    clientwin_do_unmapped(cwin, win);
}


/*}}}*/


/*{{{ Kill/close */


static bool send_clientmsg(Window win, Atom a, Time stmp)
{
    XClientMessageEvent ev;
    
    ev.type=ClientMessage;
    ev.window=win;
    ev.message_type=ioncore_g.atom_wm_protocols;
    ev.format=32;
    ev.data.l[0]=a;
    ev.data.l[1]=stmp;
    
    return (XSendEvent(ioncore_g.dpy, win, False, 0L, (XEvent*)&ev)!=0);
}


/*EXTL_DOC
 * Attempt to kill (with XKillWindow) the client that owns the X
 * window correspoding to \var{cwin}.
 */
EXTL_EXPORT_MEMBER
void clientwin_kill(WClientWin *cwin)
{
    XKillClient(ioncore_g.dpy, cwin->win);
}


void clientwin_rqclose(WClientWin *cwin, bool relocate_ignored)
{
    /* Ignore relocate parameter -- client windows can always be 
     * destroyed by the application in any case, so way may just as
     * well assume relocate is always set.
     */
    
    if(cwin->flags&CLIENTWIN_P_WM_DELETE){
        send_clientmsg(cwin->win, ioncore_g.atom_wm_delete, 
                       ioncore_get_timestamp());
    }else{
        warn(TR("Client does not support the WM_DELETE protocol."));
    }
}


/*}}}*/


/*{{{ State (hide/show) */


static void set_clientwin_state(WClientWin *cwin, int state)
{
    if(cwin->state!=state){
        cwin->state=state;
        xwindow_set_state_property(cwin->win, state);
    }
}


static void hide_clientwin(WClientWin *cwin)
{
    region_pointer_focus_hack(&cwin->region);

    if(cwin->flags&CLIENTWIN_PROP_ACROBATIC){
        XMoveWindow(ioncore_g.dpy, cwin->win,
                    -2*REGION_GEOM(cwin).w, -2*REGION_GEOM(cwin).h);
        return;
    }
    
    set_clientwin_state(cwin, IconicState);
    XSelectInput(ioncore_g.dpy, cwin->win,
                 cwin->event_mask&~(StructureNotifyMask|EnterWindowMask));
    XUnmapWindow(ioncore_g.dpy, cwin->win);
    XSelectInput(ioncore_g.dpy, cwin->win, cwin->event_mask);
}


static void show_clientwin(WClientWin *cwin)
{
    if(cwin->flags&CLIENTWIN_PROP_ACROBATIC){
        XMoveWindow(ioncore_g.dpy, cwin->win,
                    REGION_GEOM(cwin).x, REGION_GEOM(cwin).y);
        if(cwin->state==NormalState)
            return;
    }
    
    XSelectInput(ioncore_g.dpy, cwin->win,
                 cwin->event_mask&~(StructureNotifyMask|EnterWindowMask));
    XMapWindow(ioncore_g.dpy, cwin->win);
    XSelectInput(ioncore_g.dpy, cwin->win, cwin->event_mask);
    set_clientwin_state(cwin, NormalState);
}


/*}}}*/


/*{{{ Resize/reparent/reconf helpers */


void clientwin_notify_rootpos(WClientWin *cwin, int rootx, int rooty)
{
    XEvent ce;
    Window win;
    
    if(cwin==NULL)
        return;
    
    win=cwin->win;
    
    ce.xconfigure.type=ConfigureNotify;
    ce.xconfigure.event=win;
    ce.xconfigure.window=win;
    ce.xconfigure.x=rootx-cwin->orig_bw;
    ce.xconfigure.y=rooty-cwin->orig_bw;
    ce.xconfigure.width=REGION_GEOM(cwin).w;
    ce.xconfigure.height=REGION_GEOM(cwin).h;
    ce.xconfigure.border_width=cwin->orig_bw;
    ce.xconfigure.above=None;
    ce.xconfigure.override_redirect=False;
    
    XSelectInput(ioncore_g.dpy, win, cwin->event_mask&~StructureNotifyMask);
    XSendEvent(ioncore_g.dpy, win, False, StructureNotifyMask, &ce);
    XSelectInput(ioncore_g.dpy, win, cwin->event_mask);
}


static void sendconfig_clientwin(WClientWin *cwin)
{
    int rootx, rooty;
    
    region_rootpos(&cwin->region, &rootx, &rooty);
    clientwin_notify_rootpos(cwin, rootx, rooty);
}


static void do_reparent_clientwin(WClientWin *cwin, Window win, int x, int y)
{
    XSelectInput(ioncore_g.dpy, cwin->win,
                 cwin->event_mask&~StructureNotifyMask);
    XReparentWindow(ioncore_g.dpy, cwin->win, win, x, y);
    XSelectInput(ioncore_g.dpy, cwin->win, cwin->event_mask);
}


static void convert_geom(const WFitParams *fp, 
                         WClientWin *cwin, WRectangle *geom)
{
    WFitParams fptmp=*fp;
    WSizePolicy szplcy=SIZEPOLICY_FULL_EXACT;
    
    /*if(cwin->szplcy!=SIZEPOLICY_DEFAULT)
        szplcy=cwin->szplcy;*/
    
    sizepolicy(&szplcy, (WRegion*)cwin, NULL, REGION_RQGEOM_WEAK_ALL, &fptmp);
    
    *geom=fptmp.g;
}


/*}}}*/


/*{{{ Region dynfuns */


static bool postpone_resize(WClientWin *cwin)
{
    return cwin->state == IconicState && cwin->flags&CLIENTWIN_PROP_LAZY_RESIZE;
}


static bool clientwin_fitrep(WClientWin *cwin, WWindow *np, 
                             const WFitParams *fp)
{
    WRectangle geom;
    bool changes;
    int w, h;

    if(np!=NULL && !region_same_rootwin((WRegion*)cwin, (WRegion*)np))
        return FALSE;
    
    if(fp->mode&REGION_FIT_WHATEVER){
        geom.x=fp->g.x;
        geom.y=fp->g.y;
        geom.w=REGION_GEOM(cwin).w;
        geom.h=REGION_GEOM(cwin).h;
    }else{
        geom=fp->g;
    }
    
    changes=(REGION_GEOM(cwin).x!=geom.x ||
             REGION_GEOM(cwin).y!=geom.y ||
             REGION_GEOM(cwin).w!=geom.w ||
             REGION_GEOM(cwin).h!=geom.h);
    
    if(np==NULL && !changes)
        return TRUE;
    
    if(np!=NULL){
        region_unset_parent((WRegion*)cwin);

        /**
         * update netwm properties before mapping, because some apps check the
         * netwm state directly when mapped.
         *
         * also, update netwm properties after setting the parent, because
         * the new state of _NET_WM_STATE_FULLSCREEN is determined based on
         * the parent of the cwin.
         */                                                                                     
        region_set_parent((WRegion*)cwin, np);
        netwm_update_state(cwin);

        do_reparent_clientwin(cwin, np->win, geom.x, geom.y);
        sendconfig_clientwin(cwin);
        
        if(!REGION_IS_FULLSCREEN(cwin))
            cwin->flags&=~CLIENTWIN_FS_RQ;
    }
    
    if (postpone_resize(cwin))
        return TRUE;
    
    REGION_GEOM(cwin)=geom;
    
    w=maxof(1, geom.w);
    h=maxof(1, geom.h);
    
    if(cwin->flags&CLIENTWIN_PROP_ACROBATIC && !REGION_IS_MAPPED(cwin)){
        XMoveResizeWindow(ioncore_g.dpy, cwin->win,
                          -2*REGION_GEOM(cwin).w, -2*REGION_GEOM(cwin).h, 
                          w, h);
    }else{
        XMoveResizeWindow(ioncore_g.dpy, cwin->win, geom.x, geom.y, w, h);
    }
    
    cwin->flags&=~CLIENTWIN_NEED_CFGNTFY;
    
    return TRUE;
}


static void clientwin_map(WClientWin *cwin)
{
    show_clientwin(cwin);
    REGION_MARK_MAPPED(cwin);
}


static void clientwin_unmap(WClientWin *cwin)
{
    hide_clientwin(cwin);
    REGION_MARK_UNMAPPED(cwin);
}


static void clientwin_do_set_focus(WClientWin *cwin, bool warp)
{
    if(cwin->flags&CLIENTWIN_P_WM_TAKE_FOCUS){
        Time stmp=ioncore_get_timestamp();
        send_clientmsg(cwin->win, ioncore_g.atom_wm_take_focus, stmp);
    }

    region_finalise_focusing((WRegion*)cwin, cwin->win, warp);
    
    XSync(ioncore_g.dpy, 0);
}


void clientwin_restack(WClientWin *cwin, Window other, int mode)
{
    xwindow_restack(cwin->win, other, mode);
}
       

void clientwin_stacking(WClientWin *cwin, Window *bottomret, Window *topret)
{
    *bottomret=cwin->win;
    *topret=cwin->win;
}


static Window clientwin_x_window(WClientWin *cwin)
{
    return cwin->win;
}


static void clientwin_activated(WClientWin *cwin)
{
    clientwin_install_colormap(cwin);
}


static void clientwin_size_hints(WClientWin *cwin, WSizeHints *hints_ret)
{
    if(cwin->flags&CLIENTWIN_FS_RQ){
        /* Do not use size hints, when full screen mode has been
         * requested by the client window itself.
         */
        sizehints_clear(hints_ret);
    }else{
        xsizehints_to_sizehints(&cwin->size_hints, hints_ret);
    }
}


/*}}}*/


/*{{{ Identity & lookup */


/*EXTL_DOC
 * Returns a table containing the properties \code{WM_CLASS} (table entries
 * \var{instance} and \var{class}) and  \code{WM_WINDOW_ROLE} (\var{role})
 * properties for \var{cwin}. If a property is not set, the corresponding 
 * field(s) are unset in the  table.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
ExtlTab clientwin_get_ident(WClientWin *cwin)
{
    char **p=NULL, **p2=NULL, *wrole=NULL;
    int n=0, n2=0, n3=0, tmp=0;
    Window tforwin=None;
    ExtlTab tab;
    bool dockapp_hack=FALSE;
    
    p=xwindow_get_text_property(cwin->win, XA_WM_CLASS, &n);
    
    p2=xwindow_get_text_property(cwin->win, ioncore_g.atom_dockapp_hack, &n2);
    
    dockapp_hack=(n2>0);
    
    if(p==NULL){
        /* Some dockapps do actually have WM_CLASS, so use it. */
        p=p2;
        n=n2;
        p2=NULL;
    }
    
    wrole=xwindow_get_string_property(cwin->win, ioncore_g.atom_wm_window_role, 
                                      &n3);
    
    tab=extl_create_table();
    if(n>=2 && p[1]!=NULL)
        extl_table_sets_s(tab, "class", p[1]);
    if(n>=1 && p[0]!=NULL)
        extl_table_sets_s(tab, "instance", p[0]);
    if(wrole!=NULL)
        extl_table_sets_s(tab, "role", wrole);
    
    if(XGetTransientForHint(ioncore_g.dpy, cwin->win, &tforwin) 
       && tforwin!=None){
        extl_table_sets_b(tab, "is_transient", TRUE);
    }
    
    if(dockapp_hack)
        extl_table_sets_b(tab, "is_dockapp", TRUE);
    
    if(p!=NULL)
        XFreeStringList(p);
    if(p2!=NULL)
        XFreeStringList(p2);
    if(wrole!=NULL)
        free(wrole);
    
    return tab;
}


/*}}}*/


/*{{{ ConfigureRequest */


static bool check_fs_cfgrq(WClientWin *cwin, XConfigureRequestEvent *ev)
{
    /* check full screen request */
    if((ev->value_mask&(CWWidth|CWHeight))==(CWWidth|CWHeight)){
        WRegion *grp=region_groupleader_of((WRegion*)cwin);
        WScreen *scr=clientwin_fullscreen_chkrq(cwin, ev->width, ev->height);
        
        if(scr!=NULL && REGION_MANAGER(grp)!=(WRegion*)scr){
            bool sw=clientwin_fullscreen_may_switchto(cwin);
            
            cwin->flags|=CLIENTWIN_FS_RQ;
            
            if(!region_fullscreen_scr(grp, scr, sw))
                cwin->flags&=~CLIENTWIN_FS_RQ;
                
            return TRUE;
        }
    }

    return FALSE;
}

WMPlex* find_mplexer(WRegion *cwin)
{
    if (cwin == NULL)
        return NULL;
    if(obj_is((Obj*)cwin, &CLASSDESCR(WMPlex)))
        return (WMPlex*) cwin;
    return find_mplexer(cwin->manager);
}

/* Returns whether anything was actually changed. */
static bool check_normal_cfgrq(WClientWin *cwin, XConfigureRequestEvent *ev)
{
    bool result = FALSE;

    if(ev->value_mask&(CWX|CWY|CWWidth|CWHeight)){
        WRQGeomParams rq=RQGEOMPARAMS_INIT;
        int gdx=0, gdy=0;

        rq.flags=REGION_RQGEOM_WEAK_ALL|REGION_RQGEOM_ABSOLUTE;
        
        if(cwin->size_hints.flags&PWinGravity){
            rq.flags|=REGION_RQGEOM_GRAVITY;
            rq.gravity=cwin->size_hints.win_gravity;
        }
        
        /* Do I need to insert another disparaging comment on the person who
         * invented special server-supported window borders that are not 
         * accounted for in the window size? Keep it simple, stupid!
         */
        if(cwin->size_hints.flags&PWinGravity){
            gdx=xgravity_deltax(cwin->size_hints.win_gravity, 
                               -cwin->orig_bw, -cwin->orig_bw);
            gdy=xgravity_deltay(cwin->size_hints.win_gravity, 
                               -cwin->orig_bw, -cwin->orig_bw);
        }
        
        region_rootpos((WRegion*)cwin, &(rq.geom.x), &(rq.geom.y));
        rq.geom.w=REGION_GEOM(cwin).w;
        rq.geom.h=REGION_GEOM(cwin).h;
        
        if(ev->value_mask&CWWidth){
            /* If x was not changed, keep reference point where it was */
            if(cwin->size_hints.flags&PWinGravity){
                rq.geom.x+=xgravity_deltax(cwin->size_hints.win_gravity, 0,
                                           ev->width-rq.geom.w);
            }
            rq.geom.w=maxof(ev->width, 1);
            rq.flags&=~REGION_RQGEOM_WEAK_W;
        }
        if(ev->value_mask&CWHeight){
            /* If y was not changed, keep reference point where it was */
            if(cwin->size_hints.flags&PWinGravity){
                rq.geom.y+=xgravity_deltay(cwin->size_hints.win_gravity, 0,
                                           ev->height-rq.geom.h);
            }
            rq.geom.h=maxof(ev->height, 1);
            rq.flags&=~REGION_RQGEOM_WEAK_H;
        }
        if(ev->value_mask&CWX){
            rq.geom.x=ev->x+gdx;
            rq.flags&=~REGION_RQGEOM_WEAK_X;
            rq.flags&=~REGION_RQGEOM_WEAK_W;
        }
        if(ev->value_mask&CWY){
            rq.geom.y=ev->y+gdy;
            rq.flags&=~REGION_RQGEOM_WEAK_Y;
            rq.flags&=~REGION_RQGEOM_WEAK_H;
        }
        
        region_rqgeom((WRegion*)cwin, &rq, NULL);
        
        result = TRUE;
    }

    if(ev->value_mask&CWStackMode){
        switch(ev->detail){
        case Above:
            region_set_activity((WRegion*) cwin, SETPARAM_SET);
            /* TODO we should be more conservative here - but what does/should
             * region_set_activity return? */
            result = TRUE;
            break;
        case Below:
        case TopIf:
        case BottomIf:
        case Opposite:
            /* unimplemented */
            break;
        }
    }
    
    return result;
}


void clientwin_handle_configure_request(WClientWin *cwin,
                                        XConfigureRequestEvent *ev)
{
    if(ev->value_mask&CWBorderWidth)
        cwin->orig_bw=ev->border_width;
    
    cwin->flags|=CLIENTWIN_NEED_CFGNTFY;

    if(!(cwin->flags&CLIENTWIN_PROP_IGNORE_CFGRQ)){
        if(!check_fs_cfgrq(cwin, ev))
            check_normal_cfgrq(cwin, ev);
    }

    if(cwin->flags&CLIENTWIN_NEED_CFGNTFY){
        sendconfig_clientwin(cwin);
        cwin->flags&=~CLIENTWIN_NEED_CFGNTFY;
    }
}


/*}}}*/


/*{{{ Kludges */


/*EXTL_DOC
 * Attempts to fix window size problems with non-ICCCM compliant
 * programs.
 */
EXTL_EXPORT_MEMBER
void clientwin_nudge(WClientWin *cwin)
{
    XResizeWindow(ioncore_g.dpy, cwin->win, 
                  2*REGION_GEOM(cwin).w, 2*REGION_GEOM(cwin).h);
    XFlush(ioncore_g.dpy);
    XResizeWindow(ioncore_g.dpy, cwin->win, 
                  REGION_GEOM(cwin).w, REGION_GEOM(cwin).h);
}


/*}}}*/


/*{{{ Misc. */


/*EXTL_DOC
 * Return the X window id for the client window.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
double clientwin_xid(WClientWin *cwin)
{
    return cwin->win;
}


/*}}}*/


/*{{{ Save/load */


static int last_checkcode=1;


static ExtlTab clientwin_get_configuration(WClientWin *cwin)
{
    int chkc=0;
    ExtlTab tab;
    SMCfgCallback *cfg_cb;
    SMAddCallback *add_cb;
    
    tab=region_get_base_configuration((WRegion*)cwin);

    extl_table_sets_d(tab, "windowid", (double)(cwin->win));
    
    if(last_checkcode!=0){
        chkc=last_checkcode++;
        xwindow_set_integer_property(cwin->win, ioncore_g.atom_checkcode, 
                                     chkc);
        extl_table_sets_i(tab, "checkcode", chkc);
    }

    ioncore_get_sm_callbacks(&add_cb, &cfg_cb);
    
    if(cfg_cb!=NULL)
        cfg_cb(cwin, tab);
    
    return tab;
}


WRegion *clientwin_load(WWindow *par, const WFitParams *fp, ExtlTab tab)
{
    double wind=0;
    Window win=None;
    int chkc=0, real_chkc=0;
    WClientWin *cwin=NULL;
    XWindowAttributes attr;
    WRectangle rg;
    bool got_chkc=FALSE;
    
    if(!extl_table_gets_d(tab, "windowid", &wind) ||
       !extl_table_gets_i(tab, "checkcode", &chkc)){
        return NULL;
    }
    
    win=(Window)wind;

    if(XWINDOW_REGION_OF(win)!=NULL){
        warn("Client window %x already managed.", win);
        return NULL;
    }

    got_chkc=xwindow_get_integer_property(win, ioncore_g.atom_checkcode, 
                                          &real_chkc);
    
    if(!got_chkc || real_chkc!=chkc){
        ioncore_clientwin_load_missing();
        return NULL;
    }

    /* Found it! */
    
    if(!XGetWindowAttributes(ioncore_g.dpy, win, &attr)){
        warn(TR("Window %#x disappeared."), win);
        return NULL;
    }
    
    if(attr.override_redirect || 
       (ioncore_g.opmode==IONCORE_OPMODE_INIT && attr.map_state!=IsViewable)){
        warn(TR("Saved client window does not want to be managed."));
        return NULL;
    }
    
    cwin=create_clientwin(par, win, &attr);
    
    if(cwin==NULL)
        return FALSE;
    
    /* Reparent and resize taking limits set by size hints into account */
    convert_geom(fp, cwin, &rg);
    REGION_GEOM(cwin)=rg;
    do_reparent_clientwin(cwin, par->win, rg.x, rg.y);
    XResizeWindow(ioncore_g.dpy, win, maxof(1, rg.w), maxof(1, rg.h));
    
    if(!postmanage_check(cwin, &attr)){
        clientwin_destroyed(cwin);
        return NULL;
    }
    
    return (WRegion*)cwin;
}


/*}}}*/


/*{{{ Dynfuntab and class info */


static DynFunTab clientwin_dynfuntab[]={
    {(DynFun*)region_fitrep,
     (DynFun*)clientwin_fitrep},

    {region_map,
     clientwin_map},
    
    {region_unmap,
     clientwin_unmap},
    
    {region_do_set_focus, 
     clientwin_do_set_focus},
    
    {region_notify_rootpos, 
     clientwin_notify_rootpos},
    
    {region_restack, 
     clientwin_restack},

    {region_stacking, 
     clientwin_stacking},
    
    {(DynFun*)region_xwindow, 
     (DynFun*)clientwin_x_window},
    
    {region_activated, 
     clientwin_activated},
    
    {region_size_hints, 
     clientwin_size_hints},
    
    {(DynFun*)region_rqclose, 
     (DynFun*)clientwin_rqclose},
    
    {(DynFun*)region_get_configuration,
     (DynFun*)clientwin_get_configuration},

    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WClientWin, WRegion, clientwin_deinit, clientwin_dynfuntab);


/*}}}*/
