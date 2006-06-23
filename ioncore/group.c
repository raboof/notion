/*
 * ion/ioncore/group.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <libtu/minmax.h>
#include <libtu/objp.h>
#include <libmainloop/defer.h>

#include <ioncore/common.h>
#include <ioncore/rootwin.h>
#include <ioncore/focus.h>
#include <ioncore/global.h>
#include <ioncore/region.h>
#include <ioncore/manage.h>
#include <ioncore/screen.h>
#include <ioncore/names.h>
#include <ioncore/saveload.h>
#include <ioncore/attach.h>
#include <ioncore/regbind.h>
#include <ioncore/frame-pointer.h>
#include <ioncore/extlconv.h>
#include <ioncore/xwindow.h>
#include <ioncore/resize.h>
#include <ioncore/stacking.h>
#include <ioncore/sizepolicy.h>
#include <ioncore/bindmaps.h>

#include "group.h"
#include "grouppholder.h"


static void group_place_stdisp(WGroup *ws, WWindow *parent,
                                 int pos, WRegion *stdisp);
static void group_do_raise(WGroup *ws, WRegion *reg, bool initial);



/*{{{ Stacking list stuff */


WStacking *group_get_stacking(WGroup *ws)
{
    WWindow *par=REGION_PARENT(ws);
    
    return (par==NULL 
            ? NULL
            : window_get_stacking(par));
}


WStacking **group_get_stackingp(WGroup *ws)
{
    WWindow *par=REGION_PARENT(ws);
    
    return (par==NULL 
            ? NULL
            : window_get_stackingp(par));
}


static bool wsfilt(WStacking *st, void *ws)
{
    return (st->reg!=NULL && REGION_MANAGER(st->reg)==(WRegion*)ws);
}


static bool wsfilt_nostdisp(WStacking *st, void *ws)
{
    return (wsfilt(st, ws) && ((WGroup*)ws)->managed_stdisp!=st);
}


void group_iter_init(WGroupIterTmp *tmp, WGroup *ws)
{
    stacking_iter_mgr_init(tmp, ws->managed_list, NULL, ws);
}


void group_iter_init_nostdisp(WGroupIterTmp *tmp, WGroup *ws)
{
    stacking_iter_mgr_init(tmp, ws->managed_list, wsfilt_nostdisp, ws);
}


WRegion *group_iter(WGroupIterTmp *tmp)
{
    return stacking_iter_mgr(tmp);
}


WStacking *group_iter_nodes(WGroupIterTmp *tmp)
{
    return stacking_iter_mgr_nodes(tmp);
}


WGroupIterTmp group_iter_default_tmp;


/*}}}*/


/*{{{ region dynfun implementations */


static void group_fit(WGroup *ws, const WRectangle *geom)
{
    REGION_GEOM(ws)=*geom;
}


bool group_fitrep(WGroup *ws, WWindow *par, const WFitParams *fp)
{
    WGroupIterTmp tmp;
    WStacking *unweaved;
    int xdiff=0, ydiff=0;
    WStacking *st;
    WWindow *oldpar;
    WRectangle g;
    
    oldpar=REGION_PARENT(ws);
    
    if(par==NULL){
        REGION_GEOM(ws)=fp->g;
    }else if(oldpar!=NULL){
        if(!region_same_rootwin((WRegion*)ws, (WRegion*)par))
            return FALSE;
        
        if(ws->managed_stdisp!=NULL && ws->managed_stdisp->reg!=NULL)
            region_detach_manager(ws->managed_stdisp->reg);
        
        assert(ws->managed_stdisp==NULL);
        
        xdiff=fp->g.x-REGION_GEOM(ws).x;
        ydiff=fp->g.y-REGION_GEOM(ws).y;
    
        genws_do_reparent(&(ws->genws), par, fp);
        REGION_GEOM(ws)=fp->g;
    
        unweaved=stacking_unweave(&oldpar->stacking, wsfilt, (void*)ws);
        stacking_weave(&par->stacking, &unweaved, FALSE);
    }

    g=fp->g;
    g.x+=xdiff;
    g.y+=ydiff;
    
    FOR_ALL_NODES_IN_GROUP(ws, st, tmp){
        WFitParams fp2;
        
        if(st->reg==NULL)
            continue;
        
        fp2=*fp;
        fp2.g.x+=xdiff;
        fp2.g.y+=ydiff;

        g=REGION_GEOM(st->reg);
        g.x+=xdiff;
        g.y+=ydiff;
        
        sizepolicy(&st->szplcy, st->reg, &g, REGION_RQGEOM_WEAK_ALL, &fp2);

        if(!region_fitrep(st->reg, par, &fp2)){
            warn(TR("Error reparenting %s."), region_name(st->reg));
            region_detach_manager(st->reg);
        }
    }
    
    return TRUE;
}


static WGroup *sticky_source(WGroup *ws, WRegion *reg)
{
    WMPlex *mplex=REGION_MANAGER_CHK(ws, WMPlex);
    WGroup *ws2;
    
    if(mplex==NULL || mplex_layer(mplex, (WRegion*)ws)!=1)
        return NULL;
    
    ws2=REGION_MANAGER_CHK(reg, WGroup);
    
    if(ws2==NULL || REGION_MANAGER(ws2)!=(WRegion*)mplex)
        return NULL;
    
    if(mplex_layer(mplex, (WRegion*)ws2)!=1)
        return NULL;
    
    return ws2;
}


static bool same_stacking_filt(WStacking *st, void *ws)
{
    WRegion *reg=st->reg;
    
    return (reg!=NULL && (REGION_MANAGER(reg)==(WRegion*)ws ||
                          (sticky_source((WGroup*)ws, reg)!=NULL)));
}


static void move_sticky(WGroup *ws)
{
    WStacking *st;
    WGroup *ws2;
    WStacking *stacking=group_get_stacking(ws);

    if(stacking==NULL)
        return;
    
    for(st=stacking; st!=NULL; st=st->next){
        if(!st->sticky || REGION_MANAGER(st->reg)==(WRegion*)ws)
            continue;
        
        ws2=sticky_source(ws, st->reg);
        
        if(ws2==NULL)
            continue;

        if(ws2->current_managed==st){
            ws2->current_managed=NULL;
            ws->current_managed=st;
        }
        
        region_unset_manager(st->reg, (WRegion*)ws2);
        region_set_manager(st->reg, (WRegion*)ws);
    }
}


static void group_map(WGroup *ws)
{
    WRegion *reg;
    WGroupIterTmp tmp;

    genws_do_map(&(ws->genws));
    
    move_sticky(ws);

    FOR_ALL_MANAGED_BY_GROUP(ws, reg, tmp){
        region_map(reg);
    }
}


static void group_unmap(WGroup *ws)
{
    WRegion *reg;
    WGroupIterTmp tmp;

    genws_do_unmap(&(ws->genws));

    FOR_ALL_MANAGED_BY_GROUP(ws, reg, tmp){
        region_unmap(reg);
    }
}


static void group_do_set_focus(WGroup *ws, bool warp)
{
    WStacking *st=ws->current_managed;
    WStacking *stacking=group_get_stacking(ws);
        
    if((st==NULL || st->reg==NULL) && stacking!=NULL){
        st=stacking->prev;
        while(1){
            if(REGION_MANAGER(st->reg)==(WRegion*)ws && 
               st!=ws->managed_stdisp){
                break;
            }
            if(st==stacking)
                break;
            st=st->prev;
        }
    }

    if(st!=NULL && st->reg!=NULL)
        region_do_set_focus(st->reg, warp);
    else
        genws_fallback_focus(&(ws->genws), warp);
}


static bool group_managed_goto(WGroup *ws, WRegion *reg, int flags)
{
    if(!region_is_fully_mapped((WRegion*)ws))
       return FALSE;
    
    region_map(reg);
    
    if(flags&REGION_GOTO_FOCUS)
        region_maybewarp(reg, !(flags&REGION_GOTO_NOWARP));
    
    return TRUE;
}


void group_managed_remove(WGroup *ws, WRegion *reg)
{
    bool mcf=region_may_control_focus((WRegion*)ws);
    bool ds=OBJ_IS_BEING_DESTROYED(ws);
    WRegion *next=NULL;
    WStacking *st;
    bool cur=FALSE;
    
    st=group_find_stacking(ws, reg);
    
    if(st!=NULL){
        WStacking *next_st=stacking_unstack(REGION_PARENT(ws), st);
        UNLINK_ITEM(ws->managed_list, st, mgr_next, mgr_prev);
        
        if(st==ws->managed_stdisp)
            ws->managed_stdisp=NULL;
        
        if(st==ws->bottom)
            ws->bottom=NULL;

        if(st==ws->current_managed){
            cur=TRUE;
            ws->current_managed=NULL;
        }
        
        if(next_st!=NULL)
            next=next_st->reg;
    }
    
    region_unset_manager(reg, (WRegion*)ws);
    
    region_remove_bindmap_owned(reg, ioncore_group_bindmap,
                                (WRegion*)ws);
    
    if(cur && mcf && !ds)
        region_do_set_focus(next!=NULL ? next : (WRegion*)ws, FALSE);
}


static void group_managed_activated(WGroup *ws, WRegion *reg)
{
    ws->current_managed=group_find_stacking(ws, reg);
}


/*}}}*/


/*{{{ Create/destroy */


bool group_init(WGroup *ws, WWindow *parent, const WFitParams *fp)
{
    ws->current_managed=NULL;
    ws->managed_stdisp=NULL;
    ws->bottom=NULL;
    ws->managed_list=NULL;

    if(!genws_init(&(ws->genws), parent, fp))
        return FALSE;

    region_add_bindmap((WRegion*)ws, ioncore_group_bindmap);
    
    return TRUE;
}


WGroup *create_group(WWindow *parent, const WFitParams *fp)
{
    CREATEOBJ_IMPL(WGroup, group, (p, parent, fp));
}


void group_deinit(WGroup *ws)
{
    WGroupIterTmp tmp;
    WRegion *reg;

    if(ws->managed_stdisp!=NULL && ws->managed_stdisp->reg!=NULL){
        group_managed_remove(ws, ws->managed_stdisp->reg);
        assert(ws->managed_stdisp==NULL);
    }

    FOR_ALL_MANAGED_BY_GROUP(ws, reg, tmp){
        destroy_obj((Obj*)reg);
    }

    assert(ws->managed_list==NULL);

    genws_deinit(&(ws->genws));
}


    
bool group_rescue_clientwins(WGroup *ws, WPHolder *ph)
{
    WGroupIterTmp tmp;
    
    group_iter_init_nostdisp(&tmp, ws);
    
    return region_rescue_some_clientwins((WRegion*)ws, ph,
                                         (WRegionIterator*)group_iter,
                                         &tmp);
}


bool group_may_destroy(WGroup *ws)
{
    WGroupIterTmp tmp;
    WStacking *st;
    
    FOR_ALL_NODES_IN_GROUP(ws, st, tmp){
        if(st!=ws->managed_stdisp){
            warn(TR("Workspace not empty - refusing to destroy."));
            return FALSE;
        }
    }
    
    return TRUE;
}


static bool group_managed_may_destroy(WGroup *ws, WRegion *reg)
{
    return TRUE;
}


/*}}}*/


/*{{{ attach */


WStacking *group_do_add_managed(WGroup *ws, WRegion *reg, int level,
                                WSizePolicy szplcy)
{
    WStacking *st=NULL;
    CALL_DYN_RET(st, WStacking*, group_do_add_managed, ws, 
                 (ws, reg, level, szplcy));
    return st;
}
    

WStacking *group_do_add_managed_default(WGroup *ws, WRegion *reg, int level,
                                        WSizePolicy szplcy)
{
    WStacking *st=NULL, *sttop=NULL;
    Window bottom=None, top=None;
    WStacking **stackingp=group_get_stackingp(ws);
    
    if(stackingp==NULL)
        return FALSE;
    
    st=ALLOC(WStacking);
    
    if(st==NULL)
        return FALSE;
    
    st->reg=reg;
    st->above=NULL;
    st->sticky=FALSE;
    st->level=level;
    st->szplcy=szplcy;

    LINK_ITEM(ws->managed_list, st, mgr_next, mgr_prev);
    region_set_manager(reg, (WRegion*)ws);
    
    /* TODO: stacking fscked up for new regions. */
    
    region_add_bindmap_owned(reg, ioncore_group_bindmap, (WRegion*)ws);
    
    LINK_ITEM_FIRST(*stackingp, st, next, prev);
    group_do_raise(ws, reg, TRUE);

    if(region_is_fully_mapped((WRegion*)ws))
        region_map(reg);

    return st;
}

    
WRegion *group_do_attach(WGroup *ws, 
                         WRegionAttachHandler *fn, void *fnparams, 
                         const WGroupAttachParams *param)
{
    WWindow *par;
    WRegion *reg;
    WFitParams fp;
    WSizePolicy szplcy;
    uint level;
    WRectangle g;
    WStacking *st;
    bool sw;

    if(ws->bottom!=NULL && param->bottom){
        warn(TR("'bottom' already set."));
        return NULL;
    }
    
    par=REGION_PARENT(ws);
    assert(par!=NULL);
    
    if(param->geom_set){
        fp.g.x=param->geom.x+REGION_GEOM(ws).x;
        fp.g.y=param->geom.y+REGION_GEOM(ws).y;
        fp.g.w=maxof(param->geom.w, 1);
        fp.g.h=maxof(param->geom.h, 1);
        fp.mode=REGION_FIT_EXACT;
    }else{
        fp.g=REGION_GEOM(ws);
        fp.mode=REGION_FIT_WHATEVER;
    }
    
    reg=fn(par, &fp, fnparams);
    
    if(reg==NULL)
        return NULL;
    
    szplcy=(param->szplcy_set
            ? param->szplcy
            : SIZEPOLICY_UNCONSTRAINED);
        
    if(szplcy!=SIZEPOLICY_UNCONSTRAINED){
        fp.g=REGION_GEOM(ws);

        sizepolicy(&szplcy, reg, NULL, REGION_RQGEOM_WEAK_ALL, &fp);
        
        region_fitrep(reg, NULL, &fp);
    }
    
    level=(param->level_set ? param->level : 1);

    st=group_do_add_managed(ws, reg, level, szplcy);
    
    if(st==NULL){
        /* TODO: ? If the region was created by fn, it could be destroyed... */
        return NULL;
    }
    
    st->sticky=param->sticky;
    /* TODO: st->modal=param->modal; */
    
    if(param->bottom)
        ws->bottom=st;
    
    if(param->switchto_set)
        sw=param->switchto;
    else
        sw=ioncore_g.switchto_new;
    
    if(sw){
        if(region_may_control_focus((WRegion*)ws))
            region_set_focus(reg);
        else
            ws->current_managed=st;
    }
    
    return reg;
}


static void get_params(WGroup *ws, ExtlTab tab, WGroupAttachParams *par)
{
    int tmp;
    char *tmps;
    ExtlTab g;
    
    par->switchto_set=0;
    par->level_set=0;
    par->szplcy_set=0;
    par->geom_set=0;
    par->modal=0;
    par->sticky=0;
    par->bottom=0;
    
    if(extl_table_gets_i(tab, "level", &tmp)){
        if(tmp>=0){
            par->level_set=1;
            par->level=tmp;
        }
    }
    
    if(extl_table_is_bool_set(tab, "switchto"))
        par->switchto=1;
    
    if(extl_table_is_bool_set(tab, "sticky"))
        par->sticky=1;
    
    if(extl_table_is_bool_set(tab, "bottom"))
        par->bottom=1;

    if(extl_table_is_bool_set(tab, "modal"))
        par->modal=1;
    
    if(extl_table_gets_i(tab, "sizepolicy", &tmp)){
        par->szplcy_set=1;
        par->szplcy=tmp;
    }else if(extl_table_gets_s(tab, "sizepolicy", &tmps)){
        if(string2sizepolicy(tmps, &par->szplcy))
            par->szplcy_set=1;
        free(tmps);
    }
    
    if(extl_table_gets_t(tab, "geom", &g)){
        int n=0;
        
        if(extl_table_gets_i(g, "x", &(par->geom.x)))
            n++;
        if(extl_table_gets_i(g, "y", &(par->geom.y)))
            n++;
        if(extl_table_gets_i(g, "w", &(par->geom.w)))
            n++;
        if(extl_table_gets_i(g, "h", &(par->geom.h)))
            n++;
        
        if(n==4)
            par->geom_set=1;
        
        extl_unref_table(g);
    }
}



/*EXTL_DOC
 * Attach and reparent existing region \var{reg} to \var{ws}.
 * The table \var{param} may contain the fields \var{index} and
 * \var{switchto} that are interpreted as for \fnref{WMPlex.attach_new}.
 */
EXTL_EXPORT_MEMBER
WRegion *group_attach(WGroup *ws, WRegion *reg, ExtlTab param)
{
    WGroupAttachParams par;
    get_params(ws, param, &par);
    
    /* region__attach_reparent should do better checks. */
    if(reg==NULL || reg==(WRegion*)ws)
        return NULL;
    
    return region__attach_reparent((WRegion*)ws, reg,
                                   (WRegionDoAttachFn*)group_do_attach, 
                                   &par);
}


/*EXTL_DOC
 * Create a new region to be managed by \var{ws}. At least the following
 * fields in \var{param} are understood:
 * 
 * \begin{tabularx}{\linewidth}{lX}
 *  \tabhead{Field & Description}
 *  \var{type} & Class name (a string) of the object to be created. Mandatory. \\
 *  \var{name} & Name of the object to be created (a string). Optional. \\
 *  \var{switchto} & Should the region be switched to (boolean)? Optional. \\
 *  \var{level} & Stacking level; default is 1. \\
 *  \var{modal} & Make object modal. \\
 *  \var{sizepolicy} & Size policy. \\
 * \end{tabularx}
 * 
 * In addition parameters to the region to be created are passed in this 
 * same table.
 */
EXTL_EXPORT_MEMBER
WRegion *group_attach_new(WGroup *ws, ExtlTab param)
{
    WGroupAttachParams par;
    get_params(ws, param, &par);
    
    return region__attach_load((WRegion*)ws, param,
                               (WRegionDoAttachFn*)group_do_attach, 
                               &par);
}


/*}}}*/


/*{{{ Sticky status display support */


static int stdisp_szplcy(const WMPlexSTDispInfo *di, WRegion *stdisp)
{
    int pos=di->pos;
    
    if(di->fullsize){
        if(region_orientation(stdisp)==REGION_ORIENTATION_VERTICAL){
            if(pos==MPLEX_STDISP_TL || pos==MPLEX_STDISP_BL)
                return SIZEPOLICY_GRAVITY_WEST;
            else
                return SIZEPOLICY_GRAVITY_EAST;
        }else{
            if(pos==MPLEX_STDISP_TL || pos==MPLEX_STDISP_TR)
                return SIZEPOLICY_GRAVITY_NORTH;
            else
                return SIZEPOLICY_GRAVITY_SOUTH;
        }
    }else{
        if(pos==MPLEX_STDISP_TL)
            return SIZEPOLICY_GRAVITY_NORTHWEST;
        else if(pos==MPLEX_STDISP_BL)
            return SIZEPOLICY_GRAVITY_SOUTHWEST;
        else if(pos==MPLEX_STDISP_TR)
            return SIZEPOLICY_GRAVITY_NORTHEAST;
        else /*if(pos=MPLEX_STDISP_BR)*/
            return SIZEPOLICY_GRAVITY_SOUTHEAST;
    }
}


void group_manage_stdisp(WGroup *ws, WRegion *stdisp, 
                           const WMPlexSTDispInfo *di)
{
    WFitParams fp;
    uint szplcy=stdisp_szplcy(di, stdisp)|SIZEPOLICY_SHRUNK;
    
    if(ws->managed_stdisp!=NULL && ws->managed_stdisp->reg==stdisp){
        if(ws->managed_stdisp->szplcy==szplcy)
            return;
        ws->managed_stdisp->szplcy=szplcy;
    }else{
        region_detach_manager(stdisp);
        ws->managed_stdisp=group_do_add_managed(ws, stdisp, 2, szplcy);
    }

    fp.g=REGION_GEOM(ws);
    sizepolicy(&ws->managed_stdisp->szplcy, stdisp, 
               &REGION_GEOM(stdisp), 0, &fp);
    
    region_fitrep(stdisp, NULL, &fp);
}


void group_managed_rqgeom(WGroup *ws, WRegion *reg,
                            int flags, const WRectangle *geom,
                            WRectangle *geomret)
{
    WFitParams fp;
    WStacking *st;
        
    st=group_find_stacking(ws, reg);

    if(st==NULL){
        fp.g=*geom;
        fp.mode=REGION_FIT_EXACT;
    }else{
        fp.g=REGION_GEOM(ws);
        sizepolicy(&st->szplcy, reg, geom, flags, &fp);
    }
    
    if(geomret!=NULL)
        *geomret=fp.g;
    
    if(!(flags&REGION_RQGEOM_TRYONLY))
        region_fitrep(reg, NULL, &fp);
}


/*}}}*/


/*{{{ Circulate */


/*EXTL_DOC
 * Activate next object in stacking order on \var{ws}.
 */
EXTL_EXPORT_MEMBER
WRegion *group_circulate(WGroup *ws)
{
    WStacking *st=NULL, *ststart;
    WStacking *stacking=group_get_stacking(ws);
    
    if(stacking==NULL)
        return NULL;
    
    if(ws->current_managed!=NULL)
        st=ws->current_managed->next;
    
    if(st==NULL)
        st=stacking;
    ststart=st;
    
    while(1){
        if(REGION_MANAGER(st->reg)==(WRegion*)ws
           && st!=ws->managed_stdisp){
            break;
        }
        st=st->next;
        if(st==NULL)
            st=stacking;
        if(st==ststart)
            return NULL;
    }
        
    if(region_may_control_focus((WRegion*)ws))
       region_goto(st->reg);
    
    return st->reg;
}


/*EXTL_DOC
 * Activate previous object in stacking order on \var{ws}.
 */
EXTL_EXPORT_MEMBER
WRegion *group_backcirculate(WGroup *ws)
{
    WStacking *st=NULL, *ststart;
    WStacking *stacking=group_get_stacking(ws);
    
    if(stacking==NULL)
        return NULL;
    
    if(ws->current_managed!=NULL)
        st=ws->current_managed->prev;
    
    if(st==NULL)
        st=stacking->prev;
    ststart=st;
    
    while(1){
        if(REGION_MANAGER(st->reg)==(WRegion*)ws
           && st!=ws->managed_stdisp){
            break;
        }
        st=st->prev;
        if(st==ststart)
            return NULL;
    }
        
    if(region_may_control_focus((WRegion*)ws))
       region_goto(st->reg);
    
    return st->reg;
}


/*}}}*/


/*{{{ Stacking */


void group_stacking(WGroup *ws, Window *bottomret, Window *topret)
{
    WStacking *stacking=group_get_stacking(ws);
    Window win;

    if(stacking!=NULL)
        stacking_stacking(stacking, bottomret, topret, wsfilt, ws);
    
    win=region_xwindow((WRegion*)ws);
    
    if(*bottomret==None)
        *bottomret=win;
    if(*topret==None)
        *topret=win;
}


WStacking *group_find_stacking(WGroup *ws, WRegion *r)
{
    return stacking_find_mgr(ws->managed_list, r);
}


static WStacking *find_stacking_if_not_on_ws(WGroup *ws, Window w)
{
    WRegion *r=xwindow_region_of(w);
    WStacking *st=NULL;
    
    while(r!=NULL){
        if(REGION_MANAGER(r)==(WRegion*)ws)
            break;
        st=group_find_stacking(ws, r);
        if(st!=NULL)
            break;
        r=REGION_MANAGER(r);
    }
    
    return st;
}


void group_restack(WGroup *ws, Window other, int mode)
{
    WStacking *other_on_list=NULL;
    WWindow *par=REGION_PARENT(ws);
    WStacking **stackingp=group_get_stackingp(ws);
    Window win;

    if(stackingp==NULL)
       return;
       
    assert(mode==Above || mode==Below);
    assert(par!=NULL);
    
    {
        /* Need to find the point on the list to insert to. */
        Window root=None, parent=None, *children=NULL;
        uint i, n=0;
        /* Use XQueryTree to get things in stacking order. */
        XQueryTree(ioncore_g.dpy, region_xwindow((WRegion*)par),
                   &root, &parent, &children, &n);
        if(mode==Above){
            WStacking *st;
            for(i=n; i>0; ){
                i--;
                if(children[i]==other)
                    break;
                st=find_stacking_if_not_on_ws(ws, children[i]);
                if(st!=NULL)
                    other_on_list=st;
            }
        }else{
            WStacking *st;
            for(i=0; i<n; i++){
                if(children[i]==other)
                    break;
                st=find_stacking_if_not_on_ws(ws, children[i]);
                if(st!=NULL)
                    other_on_list=st;
            }
        }
        XFree(children);
    }
    
    win=region_xwindow((WRegion*)ws);
    if(win!=None){
        xwindow_restack(win, other, mode);
        other=win;
        mode=Above;
    }
    
    if(*stackingp==NULL)
        return;
    
    stacking_restack(stackingp, other, mode, other_on_list, wsfilt, ws);
}


static void group_do_raise(WGroup *ws, WRegion *reg, bool initial)
{
    WStacking **stackingp=group_get_stackingp(ws);

    if(reg==NULL || stackingp==NULL || *stackingp==NULL)
        return;

    if(REGION_MANAGER(reg)!=(WRegion*)ws){
        warn(TR("Region not managed by the workspace."));
        return;
    }
    
    stacking_do_raise(stackingp, reg, initial, 
                      region_xwindow((WRegion*)ws),
                      same_stacking_filt, ws);
}
                      

/*EXTL_DOC
 * Raise \var{reg} that must be managed by \var{ws}.
 * If \var{reg} is \code{nil}, this function silently fails.
 */
EXTL_EXPORT_MEMBER
void group_raise(WGroup *ws, WRegion *reg)
{
    group_do_raise(ws, reg, FALSE);
}


/*EXTL_DOC
 * Lower \var{reg} that must be managed by \var{ws}.
 * If \var{reg} is \code{nil}, this function silently fails.
 */
EXTL_EXPORT_MEMBER
void group_lower(WGroup *ws, WRegion *reg)
{
    WStacking *st, *stbottom=NULL, *stabove, *stnext;
    Window bottom=None, top=None, other=None;
    WStacking **stackingp=group_get_stackingp(ws);

    if(reg==NULL || stackingp==NULL || *stackingp==NULL)
        return;

    if(REGION_MANAGER(reg)!=(WRegion*)ws){
        warn(TR("Region not managed by the workspace."));
        return;
    }
    
    stacking_do_lower(stackingp, reg, 
                      region_xwindow((WRegion*)ws),
                      same_stacking_filt, ws);
}


/*}}}*/


/*{{{ Bottom */


/*EXTL_DOC
 * Returns the 'bottom' of \var{ws}.
 */
EXTL_EXPORT_MEMBER
WRegion *group_bottom(WGroup *ws)
{
    return (ws->bottom!=NULL ? ws->bottom->reg : NULL);
}


/*}}}*/


/*{{{ Misc. */


/*EXTL_DOC
 * Returns a list of regions managed by the workspace (frames, mostly).
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
ExtlTab group_managed_list(WGroup *ws)
{
    WGroupIterTmp tmp;
    group_iter_init(&tmp, ws);
    
    return extl_obj_iterable_to_table((ObjIterator*)group_iter, &tmp);
}


WRegion* group_current(WGroup *ws)
{
    return (ws->current_managed!=NULL ? ws->current_managed->reg : NULL);
}


/*}}}*/


/*{{{ Save/load */


static ExtlTab group_get_configuration(WGroup *ws)
{
    ExtlTab tab, mgds, subtab, g;
    WStacking *st;
    WGroupIterTmp tmp;
    WMPlex *par;
    int n=0;
    WRectangle tmpg;
    
    tab=region_get_base_configuration((WRegion*)ws);
    
    mgds=extl_create_table();
    
    extl_table_sets_t(tab, "managed", mgds);
    
    /* TODO: stacking order messed up */
    
    FOR_ALL_NODES_IN_GROUP(ws, st, tmp){
        if(st->reg==NULL)
            continue;
        
        subtab=region_get_configuration(st->reg);

        extl_table_sets_i(subtab, "sizepolicy", st->szplcy);
        extl_table_sets_i(subtab, "level", st->level);
        
        tmpg=REGION_GEOM(st->reg);
        tmpg.x-=REGION_GEOM(ws).x;
        tmpg.y-=REGION_GEOM(ws).y;
        
        g=extl_table_from_rectangle(&tmpg);
        extl_table_sets_t(subtab, "geom", g);
        extl_unref_table(g);
        
        if(st->sticky)
            extl_table_sets_b(subtab, "sticky", TRUE);
        
        if(ws->bottom==st)
            extl_table_sets_b(subtab, "bottom", TRUE);
        
        extl_table_seti_t(mgds, ++n, subtab);
        extl_unref_table(subtab);
    }
    
    extl_unref_table(mgds);
    
    return tab;
}


WRegion *group_load(WWindow *par, const WFitParams *fp, ExtlTab tab)
{
    WGroup *ws;
    ExtlTab substab, subtab;
    int i, n;
    
    ws=create_group(par, fp);
    
    if(ws==NULL)
        return NULL;
        
    if(!extl_table_gets_t(tab, "managed", &substab))
        return (WRegion*)ws;

    n=extl_table_get_n(substab);
    for(i=1; i<=n; i++){
        if(extl_table_geti_t(substab, i, &subtab)){
            group_attach_new(ws, subtab);
            extl_unref_table(subtab);
        }
    }
    
    extl_unref_table(substab);

    return (WRegion*)ws;
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab group_dynfuntab[]={
    {(DynFun*)region_fitrep,
     (DynFun*)group_fitrep},

    {region_map, 
     group_map},
    {region_unmap, 
     group_unmap},
    {(DynFun*)region_managed_goto, 
     (DynFun*)group_managed_goto},

    {region_do_set_focus, 
     group_do_set_focus},
    {region_managed_activated, 
     group_managed_activated},
    
    {region_managed_remove,
     group_managed_remove},
    
    {(DynFun*)region_get_configuration, 
     (DynFun*)group_get_configuration},

    {(DynFun*)region_may_destroy,
     (DynFun*)group_may_destroy},

    {(DynFun*)region_managed_may_destroy,
     (DynFun*)group_managed_may_destroy},

    {(DynFun*)region_current,
     (DynFun*)group_current},
    
    {(DynFun*)region_rescue_clientwins,
     (DynFun*)group_rescue_clientwins},
    
    /*{genws_manage_stdisp,
     group_manage_stdisp},*/
    
    {region_restack,
     group_restack},

    {region_stacking,
     group_stacking},

    {(DynFun*)region_managed_get_pholder,
     (DynFun*)group_managed_get_pholder},

    {region_managed_rqgeom,
     group_managed_rqgeom},
    
    {(DynFun*)group_do_add_managed,
     (DynFun*)group_do_add_managed_default},
    
    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WGroup, WGenWS, group_deinit, group_dynfuntab);


/*}}}*/

