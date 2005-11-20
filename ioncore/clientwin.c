/*
 * ion/ioncore/clientwin.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
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
#include "basicpholder.h"


static void set_clientwin_state(WClientWin *cwin, int state);
static bool send_clientmsg(Window win, Atom a, Time stmp);

static void do_gravity(const WRectangle *max_geom, int gravity,
                       WRectangle *geom);


WHook *clientwin_do_manage_alt=NULL;
WHook *clientwin_mapped_hook=NULL;
WHook *clientwin_unmapped_hook=NULL;
WHook *clientwin_property_change_hook=NULL;


#define LATEST_TRANSIENT(CWIN) PTRLIST_LAST(WRegion*, (CWIN)->transient_list)


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


static bool get_winprop_fn_set=FALSE;
static ExtlFn get_winprop_fn;

/*EXTL_DOC
 * Set function used to look up winprops.
 */
EXTL_EXPORT
void ioncore_set_get_winprop_fn(ExtlFn fn)
{
    if(get_winprop_fn_set)
        extl_unref_fn(get_winprop_fn);
    get_winprop_fn=extl_ref_fn(fn);
    get_winprop_fn_set=TRUE;
}


struct gravity_spec {
    const char *spec;    /* name of the gravity value       */
    int         gravity; /* X gravity value                 */
};

/* translation table for gravity specifications */
static struct gravity_spec gravity_specs[] = {
    {"center"        , CenterGravity    },
    {"west"          , WestGravity      },
    {"east"          , EastGravity      },
    {"north"         , NorthGravity     },
    {"northwest"     , NorthWestGravity },
    {"northeast"     , NorthEastGravity },
    {"south"         , SouthGravity     },
    {"southwest"     , SouthWestGravity },
    {"southeast"     , SouthEastGravity },
    { NULL           , 0                }   /* end marker */
};

static int get_gravity_winprop(WClientWin *cwin,const char *propname)
{
    char *gravity;
    char *s;
    const struct gravity_spec *sp;
    int value = ForgetGravity;

    if(extl_table_gets_s(cwin->proptab, propname, &gravity)){
	for(s=gravity; *s; ++s)
	    *s = tolower(*s);

	for(sp=gravity_specs; sp->spec; ++sp){
	    if(strcmp(gravity,sp->spec)==0){
		value=sp->gravity;
		break;
	    }
	}
	free(gravity);
    }
    return value;
}

static void clientwin_get_winprops(WClientWin *cwin)
{
    ExtlTab tab, tab2;
    int i1, i2;
    bool ret;
    
    if(!get_winprop_fn_set)
        return;
    
    extl_protect(NULL);
    ret=extl_call(get_winprop_fn, "o", "t", cwin, &tab);
    extl_unprotect(NULL);

    if(!ret)
        return;
    
    cwin->proptab=tab;
    
    if(tab==extl_table_none())
        return;

    if(extl_table_is_bool_set(tab, "transparent"))
        cwin->flags|=CLIENTWIN_PROP_TRANSPARENT;

    if(extl_table_is_bool_set(tab, "acrobatic"))
        cwin->flags|=CLIENTWIN_PROP_ACROBATIC;
    
    if(extl_table_gets_t(tab, "max_size", &tab2)){
        if(extl_table_gets_i(tab2, "w", &i1) &&
           extl_table_gets_i(tab2, "h", &i2)){
            cwin->size_hints.max_width=i1;
            cwin->size_hints.max_height=i2;
            cwin->size_hints.flags|=PMaxSize;
            cwin->flags|=CLIENTWIN_PROP_MAXSIZE;
        }
        extl_unref_table(tab2);
    }

    if(extl_table_gets_t(tab, "min_size", &tab2)){
        if(extl_table_gets_i(tab2, "w", &i1) &&
           extl_table_gets_i(tab2, "h", &i2)){
            cwin->size_hints.min_width=i1;
            cwin->size_hints.min_height=i2;
            cwin->size_hints.flags|=PMinSize;
            cwin->flags|=CLIENTWIN_PROP_MINSIZE;
        }
        extl_unref_table(tab2);
    }

    if(extl_table_gets_t(tab, "aspect", &tab2)){
        if(extl_table_gets_i(tab2, "w", &i1) &&
           extl_table_gets_i(tab2, "h", &i2)){
            cwin->size_hints.min_aspect.x=i1;
            cwin->size_hints.max_aspect.x=i1;
            cwin->size_hints.min_aspect.y=i2;
            cwin->size_hints.max_aspect.y=i2;
            cwin->size_hints.flags|=PAspect;
            cwin->flags|=CLIENTWIN_PROP_ASPECT;
        }
        extl_unref_table(tab2);
    }
    
    if(extl_table_is_bool_set(tab, "ignore_resizeinc"))
        cwin->flags|=CLIENTWIN_PROP_IGNORE_RSZINC;

    if(extl_table_is_bool_set(tab, "ignore_cfgrq"))
        cwin->flags|=CLIENTWIN_PROP_IGNORE_CFGRQ;

    if(extl_table_is_bool_set(tab, "transients_at_top"))
        cwin->transient_gravity=NorthGravity;

    cwin->gravity=get_gravity_winprop(cwin,"gravity");
    cwin->transient_gravity=get_gravity_winprop(cwin,"transient_gravity");
}


void clientwin_get_size_hints(WClientWin *cwin)
{
    XSizeHints tmp=cwin->size_hints;
    
    xwindow_get_sizehints(cwin->win, &(cwin->size_hints));
    
    if(cwin->flags&CLIENTWIN_PROP_MAXSIZE){
        cwin->size_hints.max_width=tmp.max_width;
        cwin->size_hints.max_height=tmp.max_height;
        cwin->size_hints.flags|=PMaxSize;
    }

    if(cwin->flags&CLIENTWIN_PROP_MINSIZE){
        cwin->size_hints.min_width=tmp.min_width;
        cwin->size_hints.min_height=tmp.min_height;
        cwin->size_hints.flags|=PMinSize;
    }
    
    if(cwin->flags&CLIENTWIN_PROP_ASPECT){
        cwin->size_hints.min_aspect=tmp.min_aspect;
        cwin->size_hints.max_aspect=tmp.max_aspect;
        cwin->size_hints.flags|=PAspect;
    }
    
    if(cwin->flags&CLIENTWIN_PROP_IGNORE_RSZINC)
        cwin->size_hints.flags&=~PResizeInc;
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


static bool clientwin_init(WClientWin *cwin, WWindow *par, Window win,
                           XWindowAttributes *attr)
{
    WRectangle geom;

    cwin->flags=0;
    cwin->win=win;
    cwin->state=WithdrawnState;
    
    geom.x=attr->x;
    geom.y=attr->y;
    geom.w=attr->width;
    geom.h=attr->height;
    
    /* The idiot who invented special server-supported window borders that
     * are not accounted for in the window size should be "taken behind a
     * sauna".
     */
    cwin->orig_bw=attr->border_width;
    configure_cwin_bw(cwin->win, 0);
    if(cwin->orig_bw!=0 && cwin->size_hints.flags&PWinGravity){
        geom.x+=xgravity_deltax(cwin->size_hints.win_gravity, 
                               -cwin->orig_bw, -cwin->orig_bw);
        geom.y+=xgravity_deltay(cwin->size_hints.win_gravity, 
                               -cwin->orig_bw, -cwin->orig_bw);
    }

    cwin->last_fp.g=geom;
    cwin->last_fp.mode=REGION_FIT_EXACT;

    cwin->transient_for=None;
    cwin->transient_list=NULL;
    
    cwin->n_cmapwins=0;
    cwin->cmap=attr->colormap;
    cwin->cmaps=NULL;
    cwin->cmapwins=NULL;
    cwin->n_cmapwins=0;
    cwin->event_mask=IONCORE_EVENTMASK_CLIENTWIN;

    cwin->fs_pholder=NULL;

    cwin->gravity=ForgetGravity;
    cwin->transient_gravity=ForgetGravity;
    
    region_init(&(cwin->region), par, &(cwin->last_fp));

    XSelectInput(ioncore_g.dpy, win, cwin->event_mask);

    clientwin_register(cwin);
    clientwin_get_set_name(cwin);
    clientwin_get_colormaps(cwin);
    clientwin_get_protocols(cwin);
    clientwin_get_winprops(cwin);
    clientwin_get_size_hints(cwin);
    
    XSaveContext(ioncore_g.dpy, win, ioncore_g.win_context, (XPointer)cwin);
    XAddToSaveSet(ioncore_g.dpy, win);

    return TRUE;
}


static WClientWin *create_clientwin(WWindow *par, Window win,
                                    XWindowAttributes *attr)
{
    CREATEOBJ_IMPL(WClientWin, clientwin, (p, par, win, attr));
}


static bool handle_target_props(WClientWin *cwin, const WManageParams *param)
{
    WRegion *r=NULL;
    char *target_name=NULL;
    
    if(extl_table_gets_s(cwin->proptab, "target", &target_name)){
        r=ioncore_lookup_region(target_name, NULL);
        
        free(target_name);
    
        if(r!=NULL){
            if(region_manage_clientwin(r, cwin, param, 
                                       MANAGE_REDIR_PREFER_NO))
                return TRUE;
        }
    }
    
    if(!extl_table_is_bool_set(cwin->proptab, "fullscreen"))
            return FALSE;
    
    return clientwin_enter_fullscreen(cwin, param->switchto);
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
        
        win=hints->icon_window;
        
        /* It is a dockapp, do everything again from the beginning, now
         * with the icon window.
         */
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
    param.userpos=(cwin->size_hints.flags&USPosition);
    param.switchto=(init_state!=IconicState && clientwin_get_switchto(cwin));
    param.jumpto=extl_table_is_bool_set(cwin->proptab, "jumpto");
    param.gravity=(cwin->size_hints.flags&PWinGravity
                   ? cwin->size_hints.win_gravity
                   : ForgetGravity);
    
    param.tfor=clientwin_get_transient_for(cwin);

    if(!handle_target_props(cwin, &param)){
        bool managed;
        void *mrshpm[2];
        
        mrshpm[0]=cwin;
        mrshpm[1]=&param;
        
        managed=hook_call_alt(clientwin_do_manage_alt, &mrshpm, 
                              (WHookMarshall*)do_manage_mrsh,
                              (WHookMarshallExtl*)do_manage_mrsh_extl);

        if(!managed){
            warn(TR("Unable to manage client window %#x."), win);
            goto failure;
        }
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


/*{{{ Add/remove managed */


static int clientwin_get_transients_gravity(WClientWin *cwin)
{
    if(cwin->transient_gravity!=ForgetGravity)
        return cwin->transient_gravity;
    if(cwin->last_fp.mode&REGION_FIT_GRAVITY)
        return cwin->last_fp.gravity;
    if(cwin->gravity!=ForgetGravity)
        return cwin->gravity;
    return SouthGravity;
}


static void correct_to_size_hints_of(int *w, int *h, WRegion *reg)
{
    XSizeHints hints;
    
    region_size_hints(reg, &hints);

    xsizehints_correct(&hints, w, h, FALSE);
}


static WRegion *clientwin_do_attach_transient(WClientWin *cwin, 
                                              WRegionAttachHandler *fn,
                                              void *fnparams,
                                              WRegion *thereg)
{
    WWindow *par=REGION_PARENT(cwin);
    Window bottom=None, top=None;
    WRegion *reg, *mreg;
    WFitParams fp;
    WRectangle mg;
    WFrame *frame=NULL;

    if(par==NULL)
        return NULL;

    fp.mode=REGION_FIT_BOUNDS|REGION_FIT_WHATEVER;
    fp.gravity=clientwin_get_transients_gravity(cwin);
    fp.g=cwin->last_fp.g;

    if(ioncore_g.framed_transients)
        frame=create_frame(par, &fp, "frame-transientcontainer");

    reg=fn(par, &fp, fnparams);
    
    if(reg==NULL){
        destroy_obj((Obj*)frame);
        return NULL;
    }

    if(frame!=NULL){
        frame->flags|=(FRAME_DEST_EMPTY|
                       FRAME_TAB_HIDE|
                       FRAME_SZH_USEMINMAX);
        mreg=(WRegion*)frame;
        mplex_managed_geom((WMPlex*)frame, &mg);
        
        /* border sizes */
        fp.g.w=REGION_GEOM(mreg).w-mg.w;
        fp.g.h=REGION_GEOM(mreg).h-mg.h;
        /* maximum inner size */
        mg.w=maxof(1, cwin->last_fp.g.w-fp.g.w);
        mg.h=maxof(1, minof(REGION_GEOM(reg).h, cwin->last_fp.g.h-fp.g.h));
        /* adjust it to size hints (can only shrink) */
        correct_to_size_hints_of(&(mg.w), &(mg.h), reg);
        /* final frame size */
        fp.g.w+=mg.w;
        fp.g.h+=mg.h;
        /* positioning */
        do_gravity(&(cwin->last_fp.g), fp.gravity, &(fp.g));
        
        fp.mode=REGION_FIT_EXACT;
    }else{
        mreg=reg;
        fp.g=cwin->last_fp.g;
        fp.mode=REGION_FIT_BOUNDS|REGION_FIT_GRAVITY;
        fp.gravity=clientwin_get_transients_gravity(cwin);
    }
    
    region_fitrep((WRegion*)mreg, NULL, &fp);
    
    if(frame!=NULL){
        if(!mplex_attach_simple((WMPlex*)frame, reg, 0)){
            destroy_obj((Obj*)frame);
            mreg=reg;
        }
    }

    region_stacking((WRegion*)cwin, &bottom, &top);
    region_restack(mreg, top, Above);
    
    if(!ptrlist_insert_last(&(cwin->transient_list), mreg)){
        if(frame!=NULL)
            destroy_obj((Obj*)frame);
        /* Not necessarily the right thing to do: */
        destroy_obj((Obj*)reg);
        return NULL;
    }
    region_set_manager(mreg, (WRegion*)cwin);
    
    if(REGION_IS_MAPPED((WRegion*)cwin))
        region_map(mreg);
    else
        region_unmap(mreg);
    
    if(region_may_control_focus((WRegion*)cwin))
        region_warp(mreg);
    
    return reg;
}


static WRegion *clientwin_do_attach_transient_simple(WClientWin *cwin, 
                                                     WRegionAttachHandler *fn,
                                                     void *fnparams)
{
    return clientwin_do_attach_transient(cwin, fn, fnparams, NULL);
}



/*EXTL_DOC
 * Manage \var{reg} as a transient of \var{cwin}.
 */
EXTL_EXPORT_MEMBER
bool clientwin_attach_transient(WClientWin *cwin, WRegion *reg)
{
    if(!reg)
        return FALSE;
    return (region__attach_reparent((WRegion*)cwin, reg,
                                    ((WRegionDoAttachFn*)
                                     clientwin_do_attach_transient), 
                                    reg)!=NULL);
}


static void clientwin_managed_remove(WClientWin *cwin, WRegion *transient)
{
    bool mcf=region_may_control_focus((WRegion*)cwin);
    
    ptrlist_remove(&(cwin->transient_list), transient);
    region_unset_manager(transient, (WRegion*)cwin);
    
    if(mcf){
        WRegion *reg=LATEST_TRANSIENT(cwin);
        if(reg==NULL)
            reg=&cwin->region;
        
        region_warp(reg);
    }
}


bool clientwin_rescue_clientwins(WClientWin *cwin, WPHolder *ph)
{
    PtrListIterTmp tmp;
    bool ret1, ret2;
    
    ptrlist_iter_init(&tmp, cwin->transient_list);
    
    ret1=region_rescue_some_clientwins((WRegion*)cwin, ph,
                                       (WRegionIterator*)ptrlist_iter, 
                                       &tmp);

    ret2=region_rescue_child_clientwins((WRegion*)cwin, ph);
    
    return (ret1 && ret2);
}


WPHolder *clientwin_prepare_manage(WClientWin *cwin, const WClientWin *cwin2,
                                   const WManageParams *param, int redir)
{
    if(redir==MANAGE_REDIR_STRICT_YES)
        return NULL;
    
    /* Only catch windows with transient mode set to current here. */
    if(clientwin_get_transient_mode(cwin2)!=TRANSIENT_MODE_CURRENT)
        return NULL;
    
    return (WPHolder*)create_basicpholder((WRegion*)cwin,
                                          ((WRegionDoAttachFnSimple*)
                                           clientwin_do_attach_transient_simple));
}


WBasicPHolder *clientwin_managed_get_pholder(WClientWin *cwin, WRegion *mgd)
{
    return create_basicpholder((WRegion*)cwin, 
                               ((WRegionDoAttachFnSimple*)
                                clientwin_do_attach_transient_simple));
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
    Obj *obj;
    PtrListIterTmp tmp;
    
    FOR_ALL_ON_PTRLIST(Obj*, obj, cwin->transient_list, tmp){
        destroy_obj(obj);
    }
    
    assert(PTRLIST_EMPTY(cwin->transient_list));
    
    if(cwin->win!=None){
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
    
    if(cwin->fs_pholder!=NULL){
        WPHolder *ph=cwin->fs_pholder;
        cwin->fs_pholder=NULL;
        destroy_obj((Obj*)ph);
    }
    
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
    bool cf=region_may_control_focus((WRegion*)cwin);
    WPHolder *ph=region_get_rescue_pholder((WRegion*)cwin);
    
    if(ph==NULL){
        warn(TR("Failed to rescue some client windows."));
    }else{
        if(!region_rescue_clientwins((WRegion*)cwin, ph))
            warn(TR("Failed to rescue some client windows."));
        destroy_obj((Obj*)ph);
    }
    
    if(cf && cwin->fs_pholder!=NULL)
        pholder_goto(cwin->fs_pholder);
    
    destroy_obj((Obj*)cwin);
    
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


bool clientwin_rqclose(WClientWin *cwin, bool relocate_ignored)
{
    /* Ignore relocate parameter -- client windows can always be 
     * destroyed by the application in any case, so way may just as
     * well assume relocate is always set.
     */
    
    if(cwin->flags&CLIENTWIN_P_WM_DELETE){
        send_clientmsg(cwin->win, ioncore_g.atom_wm_delete, 
                       ioncore_get_timestamp());
        return TRUE;
    }else{
        warn(TR("Client does not support the WM_DELETE protocol."));
        return FALSE;
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
    if(cwin->flags&CLIENTWIN_PROP_ACROBATIC){
        XMoveWindow(ioncore_g.dpy, cwin->win,
                    -2*cwin->last_fp.g.w, -2*cwin->last_fp.g.h);
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


static void do_gravity(const WRectangle *max_geom, int gravity,
                       WRectangle *geom)
{
    switch(gravity){
    case WestGravity:
    case NorthWestGravity:
    case SouthWestGravity:
        geom->x=max_geom->x;
        break;

    case EastGravity:
    case NorthEastGravity:
    case SouthEastGravity:
        geom->x=max_geom->x+max_geom->w-geom->w;
        break;

    default:
        geom->x=max_geom->x+max_geom->w/2-geom->w/2;
    }

    switch(gravity){
    case NorthGravity:
    case NorthWestGravity:
    case NorthEastGravity:
        geom->y=max_geom->y;
        break;

    case SouthGravity:
    case SouthWestGravity:
    case SouthEastGravity:
        geom->y=max_geom->y+max_geom->h-geom->h;
        break;

    default:
        geom->y=max_geom->y+max_geom->h/2-geom->h/2;
    }
    
    if(geom->h<=1)
        geom->h=1;
    if(geom->w<=1)
        geom->w=1;
}


static void do_convert_geom(int htry, int gravity,
                            const WRectangle *max_geom,
                            WRegion *reg, WRectangle *geom)
{
    geom->w=max_geom->w;
    geom->h=minof(htry, max_geom->h);
    
    correct_to_size_hints_of(&(geom->w), &(geom->h), reg);
    do_gravity(max_geom, gravity, geom);
}


static void convert_transient_geom(const WFitParams *fp, 
                                   WRegion *reg, WRectangle *geom)
{
    const WRectangle *max_geom=&(fp->g);
    int htry=max_geom->h;
    int gravity=ForgetGravity;
    
    if(fp->mode&REGION_FIT_BOUNDS){
	if(fp->mode&REGION_FIT_GRAVITY)
            gravity=fp->gravity;
        if(gravity==ForgetGravity)
            gravity=SouthGravity;
        
        /* TODO: old height after size hint adjustments might be
         *       zero, while htry result in positive size.
         */
        htry=REGION_GEOM(reg).h;
    }
    
    do_convert_geom(htry, gravity, max_geom, reg, geom);
}


static void convert_geom(const WFitParams *fp, 
                         WClientWin *cwin, WRectangle *geom)
{
    WFitParams fptmp=*fp;
    if(!(fptmp.mode&REGION_FIT_GRAVITY) || fptmp.gravity==ForgetGravity){
        fptmp.mode|=REGION_FIT_GRAVITY;
        fptmp.gravity=StaticGravity;
    }

    convert_transient_geom(&fptmp, (WRegion*)cwin, geom);
}


/*}}}*/


/*{{{ Region dynfuns */


static void clientwin_managed_rqgeom(WClientWin *cwin, WRegion *sub,
                                     int flags, const WRectangle *geom, 
                                     WRectangle *geomret)
{
    WRectangle g, *cg=&(cwin->last_fp.g);

    /* Constrain to cwin's maximum occupied area. */
    g.w=minof(geom->w, cg->w);
    g.h=minof(geom->h, cg->h);
    g.x=minof(maxof(cg->x, geom->x), cg->x+cg->w-g.w);
    g.y=minof(maxof(cg->y, geom->y), cg->y+cg->h-g.h);
    
    if(geomret!=NULL)
        *geomret=g;
    
    if(!(flags&REGION_RQGEOM_TRYONLY))
        region_fit(sub, &g, REGION_FIT_EXACT);
}



static bool clientwin_fitrep(WClientWin *cwin, WWindow *np, 
                             const WFitParams *fp)
{
    WRegion *transient, *next;
    WRectangle geom;
    PtrListIterTmp tmp;
    int diff;
    bool changes;
    int w, h;
    WFitParams fptmp;

    if(np!=NULL && !region_same_rootwin((WRegion*)cwin, (WRegion*)np))
        return FALSE;

    if(fp->mode&REGION_FIT_WHATEVER){
        /* fp->g is not final geometry; use current size for now. */
        geom.x=fp->g.x;
        geom.y=fp->g.y;
        geom.w=REGION_GEOM(cwin).w;
        geom.h=REGION_GEOM(cwin).h;
    }else if(cwin->flags&CLIENTWIN_FS_RQ
       && (OBJ_IS(np, WScreen)
           || (np==NULL && CLIENTWIN_IS_FULLSCREEN(cwin)))){
        /* Use all available space if NetWM full screen request. */
        geom=fp->g;
    }else{
        convert_geom(fp, cwin, &geom);
    }

    changes=(REGION_GEOM(cwin).x!=geom.x ||
             REGION_GEOM(cwin).y!=geom.y ||
             REGION_GEOM(cwin).w!=geom.w ||
             REGION_GEOM(cwin).h!=geom.h);
    
    cwin->last_fp=*fp;
    REGION_GEOM(cwin)=geom;
    
    if(np==NULL && !changes)
        return TRUE;
    
    if(np!=NULL){
        region_unset_parent((WRegion*)cwin);
        do_reparent_clientwin(cwin, np->win, geom.x, geom.y);
        region_set_parent((WRegion*)cwin, np);
        sendconfig_clientwin(cwin);

        if(!CLIENTWIN_IS_FULLSCREEN(cwin) && cwin->fs_pholder!=NULL){
            WPHolder *ph=cwin->fs_pholder;
            cwin->fs_pholder=NULL;
            cwin->flags&=~CLIENTWIN_FS_RQ;
            /* Can't destroy it yet - messes up mplex placeholder
             * reorganisation.
             */
            mainloop_defer_destroy((Obj*)ph);
        }
        
        netwm_update_state(cwin);
    }
    
    w=maxof(1, geom.w);
    h=maxof(1, geom.h);
    
    if(cwin->flags&CLIENTWIN_PROP_ACROBATIC && !REGION_IS_MAPPED(cwin)){
        XMoveResizeWindow(ioncore_g.dpy, cwin->win,
                          -2*cwin->last_fp.g.w, -2*cwin->last_fp.g.h, w, h);
    }else{
        XMoveResizeWindow(ioncore_g.dpy, cwin->win, geom.x, geom.y, w, h);
    }
    
    cwin->flags&=~CLIENTWIN_NEED_CFGNTFY;
    
    fptmp.g=fp->g;
    fptmp.mode=REGION_FIT_BOUNDS|REGION_FIT_GRAVITY;
    if(!(fp->mode&REGION_FIT_GRAVITY) || fp->gravity==ForgetGravity)
        fptmp.gravity=clientwin_get_transients_gravity(cwin);
    else
        fptmp.gravity=fp->gravity;
    
    FOR_ALL_ON_PTRLIST(WRegion*, transient, cwin->transient_list, tmp){
        WFitParams fp2;
        fp2.mode=REGION_FIT_EXACT;
        if(ioncore_g.framed_transients){
            convert_transient_geom(&(fptmp), transient, &(fp2.g));
        }else{
            /* Hack */
            fp2=fptmp;
        }

        if(!region_fitrep(transient, np, &fp2) && np!=NULL){
            warn(TR("Error reparenting %s."), region_name(transient));
            region_detach_manager(transient);
        }
    }
    
    return TRUE;
}


static void clientwin_map(WClientWin *cwin)
{
    WRegion *sub;
    PtrListIterTmp tmp;
    
    show_clientwin(cwin);
    REGION_MARK_MAPPED(cwin);
    
    FOR_ALL_ON_PTRLIST(WRegion*, sub, cwin->transient_list, tmp){
        region_map(sub);
    }
}


static void clientwin_unmap(WClientWin *cwin)
{
    WRegion *sub;
    PtrListIterTmp tmp;
    
    hide_clientwin(cwin);
    REGION_MARK_UNMAPPED(cwin);
    
    FOR_ALL_ON_PTRLIST(WRegion*, sub, cwin->transient_list, tmp){
        region_unmap(sub);
    }
}


static void clientwin_do_set_focus(WClientWin *cwin, bool warp)
{
    WRegion *trs=LATEST_TRANSIENT(cwin);
    
    if(trs!=NULL){
        region_do_set_focus(trs, warp);
        return;
    }

    if(warp)
        region_do_warp((WRegion*)cwin);

    if(cwin->flags&CLIENTWIN_P_WM_TAKE_FOCUS){
        Time stmp=ioncore_get_timestamp();
        send_clientmsg(cwin->win, ioncore_g.atom_wm_take_focus, stmp);
    }

    region_set_await_focus((WRegion*)cwin);
    xwindow_do_set_focus(cwin->win);

    XSync(ioncore_g.dpy, 0);
}


static void restack_transients(WClientWin *cwin)
{
    Window other=cwin->win;
    PtrListIterTmp tmp;
    int mode=Above;
    WRegion *reg;

    FOR_ALL_ON_PTRLIST(WRegion*, reg, cwin->transient_list, tmp){
        Window bottom=None, top=None;
        region_restack(reg, other, Above);
        region_stacking(reg, &bottom, &top);
        if(top!=None)
            other=top;
    }
}



static bool clientwin_managed_goto(WClientWin *cwin, WRegion *sub, int flags)
{
    if(flags&REGION_GOTO_ENTERWINDOW)
        return FALSE;
    
    if(LATEST_TRANSIENT(cwin)!=sub){
        ptrlist_reinsert_last(&(cwin->transient_list), sub);
        restack_transients(cwin);
    }
        
    if(!REGION_IS_MAPPED(cwin))
        return FALSE;
    
    if(!REGION_IS_MAPPED(sub))
        region_map(sub);
    
    if(flags&REGION_GOTO_FOCUS)
        region_maybewarp(sub, !(flags&REGION_GOTO_NOWARP));
    
    return TRUE;
}


void clientwin_restack(WClientWin *cwin, Window other, int mode)
{
    xwindow_restack(cwin->win, other, mode);
    restack_transients(cwin);
}
       

void clientwin_stacking(WClientWin *cwin, Window *bottomret, Window *topret)
{
    WRegion *reg;
    PtrListIterTmp tmp;
    
    *bottomret=cwin->win;
    *topret=cwin->win;
    
    FOR_ALL_ON_PTRLIST_REV(WRegion*, reg, cwin->transient_list, tmp){
        Window bottom=None, top=None;
        region_stacking(reg, &bottom, &top);
        if(top!=None){
            *topret=top;
            break;
        }
    }
}


static Window clientwin_x_window(WClientWin *cwin)
{
    return cwin->win;
}


static void clientwin_activated(WClientWin *cwin)
{
    clientwin_install_colormap(cwin);
}


static void clientwin_resize_hints(WClientWin *cwin, XSizeHints *hints_ret)
{
    WRegion *trs;
    PtrListIterTmp tmp;
    
    *hints_ret=cwin->size_hints;
    
    FOR_ALL_ON_PTRLIST(WRegion*, trs, cwin->transient_list, tmp){
        xsizehints_adjust_for(hints_ret, trs);
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
    char **p=NULL, *wrole=NULL;
    int n=0, n2=0, n3=0, tmp=0;
    ExtlTab tab;
    
    p=xwindow_get_text_property(cwin->win, XA_WM_CLASS, &n);
    wrole=xwindow_get_string_property(cwin->win, ioncore_g.atom_wm_window_role, &n2);
    
    tab=extl_create_table();
    if(n>=2 && p[1]!=NULL)
        extl_table_sets_s(tab, "class", p[1]);
    if(n>=1 && p[0]!=NULL)
        extl_table_sets_s(tab, "instance", p[0]);
    if(wrole!=NULL)
        extl_table_sets_s(tab, "role", wrole);
    
    if(p!=NULL)
        XFreeStringList(p);
    if(wrole!=NULL)
        free(wrole);
    
    return tab;
}


/*}}}*/


/*{{{ ConfigureRequest */


void clientwin_handle_configure_request(WClientWin *cwin,
                                        XConfigureRequestEvent *ev)
{
    if(ev->value_mask&CWBorderWidth)
        cwin->orig_bw=ev->border_width;
    
    if(cwin->flags&CLIENTWIN_PROP_IGNORE_CFGRQ){
        sendconfig_clientwin(cwin);
        return;
    }

    /* check full screen request */
    if((ev->value_mask&(CWWidth|CWHeight))==(CWWidth|CWHeight)){
        bool sw=clientwin_fullscreen_may_switchto(cwin);
        if(clientwin_check_fullscreen_request(cwin, ev->width, ev->height, sw))
            return;
    }

    cwin->flags|=CLIENTWIN_NEED_CFGNTFY;

    if(ev->value_mask&(CWX|CWY|CWWidth|CWHeight)){
        WRectangle geom;
        WRegion *mgr;
        int rqflags=REGION_RQGEOM_WEAK_ALL;
        int gdx=0, gdy=0;

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
        
        /* Rootpos is usually wrong here, but managers (frames) that respect
         * position at all, should define region_rqgeom_clientwin to
         * handle this how they see fit.
         */
        region_rootpos((WRegion*)cwin, &(geom.x), &(geom.y));
        geom.w=REGION_GEOM(cwin).w;
        geom.h=REGION_GEOM(cwin).h;
        
        if(ev->value_mask&CWWidth){
            /* If x was not changed, keep reference point where it was */
            if(cwin->size_hints.flags&PWinGravity){
                geom.x+=xgravity_deltax(cwin->size_hints.win_gravity, 0,
                                       ev->width-geom.w);
            }
            geom.w=maxof(ev->width, 1);
            rqflags&=~REGION_RQGEOM_WEAK_W;
        }
        if(ev->value_mask&CWHeight){
            /* If y was not changed, keep reference point where it was */
            if(cwin->size_hints.flags&PWinGravity){
                geom.y+=xgravity_deltay(cwin->size_hints.win_gravity, 0,
                                       ev->height-geom.h);
            }
            geom.h=maxof(ev->height, 1);
            rqflags&=~REGION_RQGEOM_WEAK_H;
        }
        if(ev->value_mask&CWX){
            geom.x=ev->x+gdx;
            rqflags&=~REGION_RQGEOM_WEAK_X;
        }
        if(ev->value_mask&CWY){
            geom.y=ev->y+gdy;
            rqflags&=~REGION_RQGEOM_WEAK_Y;
        }
        
        mgr=region_manager((WRegion*)cwin);
        if(mgr!=NULL && HAS_DYN(mgr, region_rqgeom_clientwin)){
            /* Manager gets to decide how to handle position request. */
            region_rqgeom_clientwin(mgr, cwin, rqflags, &geom);
        }else{
            /*region_convert_root_geom(region_parent((WRegion*)cwin),
                                     &geom);
            region_rqgeom((WRegion*)cwin, rqflags, &geom, NULL);*/
            /* Just use any known available space wanted or give up some */
            /* Temporary hack. */
            REGION_GEOM(cwin).w=geom.w;
            REGION_GEOM(cwin).h=geom.h;
            region_fitrep((WRegion*)cwin, NULL, &(cwin->last_fp));
        }
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
    XResizeWindow(ioncore_g.dpy, cwin->win, 2*cwin->last_fp.g.w,
                  2*cwin->last_fp.g.h);
    XFlush(ioncore_g.dpy);
    XResizeWindow(ioncore_g.dpy, cwin->win, REGION_GEOM(cwin).w,
                  REGION_GEOM(cwin).h);
}


/*}}}*/


/*{{{ Misc. */


/*EXTL_DOC
 * Returns a list of regions managed by the clientwin (transients, mostly).
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
ExtlTab clientwin_managed_list(WClientWin *cwin)
{
    PtrListIterTmp tmp;
    ptrlist_iter_init(&tmp, cwin->transient_list);
    
    return extl_obj_iterable_to_table((ObjIterator*)ptrlist_iter, &tmp);
}


/*EXTL_DOC
 * Toggle transients managed by \var{cwin} between top/bottom
 * of the window.
 */
EXTL_EXPORT_MEMBER
void clientwin_toggle_transients_pos(WClientWin *cwin)
{
    WRegion *transient;
    WFitParams fp;
    PtrListIterTmp tmp;
    
    if(cwin->transient_gravity==NorthGravity)
	cwin->transient_gravity=SouthGravity;
    else
	cwin->transient_gravity=NorthGravity;

    fp.mode=REGION_FIT_BOUNDS|REGION_FIT_GRAVITY;
    fp.g=cwin->last_fp.g;
    fp.gravity=clientwin_get_transients_gravity(cwin);
    
    FOR_ALL_ON_PTRLIST(WRegion*, transient, cwin->transient_list, tmp){
        region_fitrep(transient, NULL, &fp);
    }
}


static WRegion *clientwin_current(WClientWin *cwin)
{
    return LATEST_TRANSIENT(cwin);
}


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
    bool got_chkc;

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

    attr.x=fp->g.x;
    attr.y=fp->g.y;
    attr.width=fp->g.w;
    attr.height=fp->g.h;

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

    {(DynFun*)region_current,
     (DynFun*)clientwin_current},
    
    {region_map,
     clientwin_map},
    
    {region_unmap,
     clientwin_unmap},
    
    {region_do_set_focus, 
     clientwin_do_set_focus},
    
    {(DynFun*)region_managed_goto,
     (DynFun*)clientwin_managed_goto},
    
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
     clientwin_resize_hints},
    
    {region_managed_remove, 
     clientwin_managed_remove},
    
    {region_managed_rqgeom, 
     clientwin_managed_rqgeom},
    
    {(DynFun*)region_rqclose, 
     (DynFun*)clientwin_rqclose},
    
    {(DynFun*)region_get_configuration,
     (DynFun*)clientwin_get_configuration},
    
    {(DynFun*)region_rescue_clientwins,
     (DynFun*)clientwin_rescue_clientwins},
    
    {(DynFun*)region_prepare_manage,
     (DynFun*)clientwin_prepare_manage},

    {(DynFun*)region_managed_get_pholder,
     (DynFun*)clientwin_managed_get_pholder},

    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WClientWin, WRegion, clientwin_deinit, clientwin_dynfuntab);


/*}}}*/
