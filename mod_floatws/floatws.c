/*
 * ion/mod_floatws/floatws.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
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

#include "floatws.h"
#include "floatwspholder.h"
#include "floatwsrescueph.h"
#include "floatframe.h"
#include "placement.h"
#include "main.h"


static WFloatStacking *stacking=NULL;


static void floatws_place_stdisp(WFloatWS *ws, WWindow *parent,
                                 int corner, WRegion *stdisp);
static void floatws_do_raise(WFloatWS *ws, WRegion *reg, bool initial);


/*{{{ Stacking list iteration */


void floatws_iter_init(WFloatWSIterTmp *tmp, WFloatWS *ws)
{
    tmp->ws=ws;
    tmp->st=stacking;
}


WRegion *floatws_iter(WFloatWSIterTmp *tmp)
{
    WRegion *next=NULL;
    
    while(tmp->st!=NULL){
        next=tmp->st->reg;
        tmp->st=tmp->st->next;
        if(tmp->ws==NULL || REGION_MANAGER(next)==(WRegion*)tmp->ws)
            break;
    }
    
    return next;
}


WFloatWSIterTmp floatws_iter_default_tmp;


/*}}}*/


/*{{{ region dynfun implementations */


static void floatws_fit(WFloatWS *ws, const WRectangle *geom)
{
    REGION_GEOM(ws)=*geom;
}


bool floatws_fitrep(WFloatWS *ws, WWindow *par, const WFitParams *fp)
{
    WFloatStacking *st, *stnext, *end;
    int xdiff, ydiff;
    WRectangle g;
    bool rs;
    
    if(par==NULL){
        REGION_GEOM(ws)=fp->g;
        return TRUE;
    }

    if(!region_same_rootwin((WRegion*)ws, (WRegion*)par))
        return FALSE;

    if(ws->managed_stdisp!=NULL)
        region_detach_manager(ws->managed_stdisp);
    
    assert(ws->managed_stdisp==NULL);

    genws_do_reparent(&(ws->genws), par, fp);
    
    xdiff=fp->g.x-REGION_GEOM(ws).x;
    ydiff=fp->g.y-REGION_GEOM(ws).y;
    
    end=NULL;

    for(st=stacking; st!=end && st!=NULL; st=stnext){
        stnext=st->next;
        
        if(REGION_MANAGER(st->reg)==(WRegion*)ws){
            /* It doesn't matter in which order the frames for different
             * parents are, just that the frames with the same parent are
             * ordered properly.
             */
            UNLINK_ITEM(stacking, st, next, prev);
            LINK_ITEM(stacking, st, next, prev);
            
            if(end==NULL)
                end=st;
            
            g=REGION_GEOM(st->reg);
            g.x+=xdiff;
            g.y+=ydiff;
        
            if(!region_reparent(st->reg, par, &g, REGION_FIT_EXACT)){
                warn(TR("Error reparenting %s."), region_name(st->reg));
                region_detach_manager(st->reg);
            }
        }
    }
    
    return TRUE;
}


static bool is_l1(WFloatWS *ws)
{
    WMPlex *mplex=REGION_MANAGER_CHK(ws, WMPlex);
    return (mplex!=NULL && mplex_layer(mplex, (WRegion*)ws)==1);
}


static WFloatWS *same_stacking(WFloatWS *ws, WRegion *reg)
{
    WMPlex *mplex;
    WFloatWS *ws2;

    ws2=REGION_MANAGER_CHK(reg, WFloatWS);
    
    if(ws2==ws)
        return ws;
    
    if(ws2==NULL)
        return NULL;
    
    if(REGION_MANAGER(ws)==NULL){
        if(REGION_PARENT(ws)==REGION_PARENT(ws2) && is_l1(ws2))
            return ws2;
        return NULL;
    }

    if(REGION_MANAGER(ws2)==NULL){
        if(REGION_PARENT(ws)==REGION_PARENT(ws2) && is_l1(ws))
            return ws2;
        return NULL;
    }
    
    if(REGION_MANAGER(ws2)==REGION_MANAGER(ws) && is_l1(ws) && is_l1(ws2))
        return ws2;
    return NULL;
}


static void move_sticky(WFloatWS *ws)
{
    WFloatStacking *st;
    WFloatWS *ws2;

    for(st=stacking; st!=NULL; st=st->next){
        if(!st->sticky || REGION_MANAGER(st->reg)==(WRegion*)ws)
            continue;
        
        ws2=same_stacking(ws, st->reg);
        
        if(ws2==NULL)
            continue;

        if(ws2->current_managed==st->reg){
            ws2->current_managed=NULL;
            ws->current_managed=st->reg;
        }
        
        region_unset_manager(st->reg, (WRegion*)ws2);
        region_set_manager(st->reg, (WRegion*)ws);
    }
}


static void floatws_map(WFloatWS *ws)
{
    WRegion *reg;
    WFloatWSIterTmp tmp;

    genws_do_map(&(ws->genws));
    
    move_sticky(ws);

    FOR_ALL_MANAGED_BY_FLOATWS(ws, reg, tmp){
        region_map(reg);
    }
    
    if(ws->managed_stdisp!=NULL)
        region_map(ws->managed_stdisp);
}


static void floatws_unmap(WFloatWS *ws)
{
    WRegion *reg;
    WFloatWSIterTmp tmp;

    genws_do_unmap(&(ws->genws));

    FOR_ALL_MANAGED_BY_FLOATWS(ws, reg, tmp){
        region_unmap(reg);
    }

    if(ws->managed_stdisp!=NULL)
        region_unmap(ws->managed_stdisp);
}


static void floatws_do_set_focus(WFloatWS *ws, bool warp)
{
    WRegion *r=ws->current_managed;
        
    if(r==NULL && stacking!=NULL){
        WFloatStacking *st=stacking->prev;
        while(1){
            if(REGION_MANAGER(st->reg)==(WRegion*)ws && 
               st->reg!=ws->managed_stdisp){
                r=st->reg;
                break;
            }
            if(st==stacking)
                break;
            st=st->prev;
        }
    }

    if(r!=NULL)
        region_do_set_focus(r, warp);
    else
        genws_fallback_focus(&(ws->genws), warp);
}


static bool floatws_managed_goto(WFloatWS *ws, WRegion *reg, int flags)
{
    if(!region_is_fully_mapped((WRegion*)ws))
       return FALSE;
    
    region_map(reg);
    
    if(flags&REGION_GOTO_FOCUS)
        region_maybewarp(reg, !(flags&REGION_GOTO_NOWARP));
    
    return TRUE;
}


static void floatws_managed_remove(WFloatWS *ws, WRegion *reg)
{
    bool mcf=region_may_control_focus((WRegion*)ws);
    bool ds=OBJ_IS_BEING_DESTROYED(ws);
    WRegion *next=NULL;
    WFloatStacking *st, *stnext;
    bool nextlocked=FALSE;
    
    for(st=stacking; st!=NULL; st=stnext){
        stnext=st->next;
        if(st->reg==reg){
            next=st->above;
            nextlocked=TRUE;
            UNLINK_ITEM(stacking, st, next, prev);
            free(st);
        }else if(st->above==reg){
            st->above=NULL;
            next=st->reg;
            nextlocked=TRUE;
        }else if(!nextlocked){
            next=st->reg;
        }
    }
    
    if(reg==ws->managed_stdisp)
        ws->managed_stdisp=NULL;
    
    region_unset_manager(reg, (WRegion*)ws);
    
    region_remove_bindmap_owned(reg, mod_floatws_floatws_bindmap,
                                (WRegion*)ws);
    
    if(ws->current_managed!=reg)
        return;
    
    ws->current_managed=NULL;
    
    if(mcf && !ds)
        region_do_set_focus(next!=NULL ? next : (WRegion*)ws, FALSE);
}


static void floatws_managed_activated(WFloatWS *ws, WRegion *reg)
{
    ws->current_managed=reg;
}


/*}}}*/


/*{{{ Create/destroy */


static bool floatws_init(WFloatWS *ws, WWindow *parent, const WFitParams *fp)
{
    ws->current_managed=NULL;
    ws->managed_stdisp=NULL;
    ws->stdisp_corner=MPLEX_STDISP_BL;

    if(!genws_init(&(ws->genws), parent, fp))
        return FALSE;

    region_add_bindmap((WRegion*)ws, mod_floatws_floatws_bindmap);
    
    return TRUE;
}


WFloatWS *create_floatws(WWindow *parent, const WFitParams *fp)
{
    CREATEOBJ_IMPL(WFloatWS, floatws, (p, parent, fp));
}


void floatws_deinit(WFloatWS *ws)
{
    WFloatWSIterTmp tmp;
    WRegion *reg;

    FOR_ALL_MANAGED_BY_FLOATWS(ws, reg, tmp){
        floatws_managed_remove(ws, reg);
    }

    FOR_ALL_MANAGED_BY_FLOATWS(ws, reg, tmp){
        assert(FALSE);
    }

    if(ws->managed_stdisp!=NULL)
        floatws_managed_remove(ws, ws->managed_stdisp);
        
    genws_deinit(&(ws->genws));
}


static WRegion *iter_just_cwins(WFloatWSIterTmp *tmp)
{
    WRegion *r;
    
    while(TRUE){
        r=(WRegion*)floatws_iter(tmp);
        if(r==NULL || OBJ_IS(r, WClientWin))
            break;
    }
    
    return r;
}


bool floatws_rescue_clientwins(WFloatWS *ws)
{
    WFloatWSIterTmp tmp;
    
    floatws_iter_init(&tmp, ws);
    
    return region_rescue_some_clientwins((WRegion*)ws, 
                                         (WRegionIterator*)iter_just_cwins, 
                                         &tmp);
}


/*EXTL_DOC
 * Destroys \var{ws} unless this would put the WM in a possibly unusable
 * state.
 */
EXTL_EXPORT_MEMBER
bool floatws_rqclose_relocate(WFloatWS *ws)
{
    if(!region_may_destroy((WRegion*)ws)){
        warn(TR("Workspace may not be destroyed."));
        return FALSE;
    }
    
    /* TODO: move frames to other workspaces */
    
    if(!region_rescue_clientwins((WRegion*)ws)){
        warn(TR("Failed to rescue some client windows!"));
        return FALSE;
    }
    
    mainloop_defer_destroy((Obj*)ws);
    return TRUE;
}


bool floatws_rqclose(WFloatWS *ws)
{
    WRegion *reg;
    
    FOR_ALL_MANAGED_BY_FLOATWS_UNSAFE(ws, reg){
        if(reg!=ws->managed_stdisp){
            warn(TR("Refusing to close non-empty workspace."));
            return FALSE;
        }
    }
    
    return floatws_rqclose_relocate(ws);
}


/*}}}*/


/*{{{ manage_clientwin/transient */


bool floatws_add_managed(WFloatWS *ws, WRegion *reg)
{
    WFloatStacking *st=ALLOC(WFloatStacking), *sttop=NULL;
    Window bottom=None, top=None;
    
    if(st==NULL)
        return FALSE;
    
    st->reg=reg;
    st->above=NULL;
    st->sticky=FALSE;

    region_set_manager(reg, (WRegion*)ws);
    
    region_add_bindmap_owned(reg, mod_floatws_floatws_bindmap, (WRegion*)ws);

    LINK_ITEM_FIRST(stacking, st, next, prev);
    floatws_do_raise(ws, reg, TRUE);

    if(region_is_fully_mapped((WRegion*)ws))
        region_map(reg);
    
    return TRUE;
}


#define REG_OK(R) OBJ_IS(R, WMPlex)


static WMPlex *find_existing(WFloatWS *ws)
{
    WRegion *r=ws->current_managed;
    
    if(r!=NULL && REG_OK(r))
        return (WMPlex*)r;
    
    FOR_ALL_MANAGED_BY_FLOATWS_UNSAFE(ws, r){
        if(REG_OK(r))
            return (WMPlex*)r;
    }
    
    return NULL;
}


WFloatFrame *floatws_create_frame(WFloatWS *ws, 
                                  const WRectangle *geom, int gravity, 
                                  bool inner_geom, bool respect_pos)
{
    WFloatFrame *frame=NULL;
    WFitParams fp;
    WWindow *par;

    par=REGION_PARENT(ws);
    assert(par!=NULL);

    /* Create frame with dummy geometry */
    fp.mode=REGION_FIT_EXACT;
    fp.g=*geom;
    
    frame=create_floatframe(par, &fp);

    if(frame==NULL){
        warn(TR("Failure to create a new frame."));
        return NULL;
    }

    if(inner_geom)
        floatframe_geom_from_initial_geom(frame, ws, &fp.g, gravity);
    
    /* If the requested geometry does not overlap the workspaces's geometry, 
     * position request is never honoured.
     */
    if((fp.g.x+fp.g.w<=REGION_GEOM(ws).x) ||
       (fp.g.y+fp.g.h<=REGION_GEOM(ws).y) ||
       (fp.g.x>=REGION_GEOM(ws).x+REGION_GEOM(ws).w) ||
       (fp.g.y>=REGION_GEOM(ws).y+REGION_GEOM(ws).h)){
        respect_pos=FALSE;
    }
    
    if(!respect_pos)
        floatws_calc_placement(ws, &fp.g);

    /* Set proper geometry */
    region_fit((WRegion*)frame, &fp.g, REGION_FIT_EXACT);

    floatws_add_managed(ws, (WRegion*)frame);

    return frame;
}


static bool floatws_do_manage_clientwin(WFloatWS *ws, WClientWin *cwin,
                                        const WManageParams *param, 
                                        int redir, bool respect_pos)
{
    WFloatFrame *frame=NULL;
    WFitParams fp;
    int swf;
    
    if(redir==MANAGE_REDIR_PREFER_YES){
        WMPlex *m=find_existing(ws);
        if(m!=NULL){
            if(region_manage_clientwin((WRegion*)m, cwin, param,
                                       MANAGE_REDIR_STRICT_YES))
                return TRUE;
        }
    }
    
    if(redir==MANAGE_REDIR_STRICT_YES)
        return FALSE;

    if(param->maprq && ioncore_g.opmode!=IONCORE_OPMODE_INIT){
        /* When the window is mapped by application request, position
         * request is only honoured if the position was given by the user
         * and in case of a transient (the app may know better where to 
         * place them) or if we're initialising.
         */
        respect_pos=(param->tfor!=NULL || param->userpos);
    }

    frame=floatws_create_frame(ws, &(param->geom), param->gravity,
                               TRUE, respect_pos);
    
    if(frame==NULL)
        return FALSE;
    
    assert(region_same_rootwin((WRegion*)frame, (WRegion*)cwin));

    swf=(param->switchto ? MPLEX_ATTACH_SWITCHTO : 0);
    if(!mplex_attach_simple((WMPlex*)frame, (WRegion*)cwin, swf)){
        destroy_obj((Obj*)frame);
        return FALSE;
    }

    /* Don't warp, it is annoying in this case */
    if(param->switchto && region_may_control_focus((WRegion*)ws)){
        ioncore_set_previous_of((WRegion*)frame);
        region_set_focus((WRegion*)frame);
    }
    
    return TRUE;
}


bool floatws_manage_clientwin(WFloatWS *ws, WClientWin *cwin,
                              const WManageParams *param,
                              int redir)
{
    return floatws_do_manage_clientwin(ws, cwin, param, redir, TRUE);
}


static bool floatws_handle_drop(WFloatWS *ws, int x, int y,
                                WRegion *dropped)
{
    WFloatFrame *frame;
    WRectangle g=REGION_GEOM(dropped);
    
    g.x=x;
    g.y=y;

    frame=floatws_create_frame(ws, &g, NorthWestGravity, TRUE, TRUE);
    
    if(frame==NULL)
        return FALSE;
    
    if(!mplex_attach_simple((WMPlex*)frame, dropped, MPLEX_ATTACH_SWITCHTO)){
        destroy_obj((Obj*)frame);
        return FALSE;
    }
    
    floatws_add_managed(ws, (WRegion*)frame);

    return TRUE;
}


/*EXTL_DOC
 * Attach client window \var{cwin} on \var{ws}.
 * At least the following fields in \var{p} are supported:
 * 
 * \begin{tabularx}{\linewidth}{lX}
 *  \tabhead{Field & Description}
 *  \var{switchto} & Should the region be switched to (boolean)? Optional. \\
 *  \var{geom} & Geometry; \var{x} and \var{y}, if set, indicates top-left of 
 *   the frame to be created while \var{width} and \var{height}, if set, indicate
 *   the size of the client window within that frame. Optional.
 * \end{tabularx}
 */
EXTL_EXPORT_MEMBER
bool floatws_attach(WFloatWS *ws, WClientWin *cwin, ExtlTab p)
{
    int posok=0;
    WManageParams param=MANAGEPARAMS_INIT;
    ExtlTab g;
    
    if(cwin==NULL)
        return FALSE;
    
    param.gravity=ForgetGravity;
    param.geom.x=0;
    param.geom.y=0;
    param.geom.w=REGION_GEOM(cwin).w;
    param.geom.h=REGION_GEOM(cwin).h;
    
    extl_table_gets_b(p, "switchto", &(param.switchto));
    
    if(extl_table_gets_t(p, "geom", &g)){
        if(extl_table_gets_i(g, "x", &(param.geom.x)))
            posok++;
        if(extl_table_gets_i(g, "y", &(param.geom.y)))
            posok++;
    
        extl_table_gets_i(p, "w", &(param.geom.w));
        extl_table_gets_i(p, "h", &(param.geom.h));
        
        extl_unref_table(g);
    }
    
    param.geom.w=maxof(0, param.geom.w);
    param.geom.h=maxof(0, param.geom.h);

    /*if(posok==2)
        param.userpos=TRUE;*/
    
    return floatws_do_manage_clientwin(ws, cwin, &param, 
                                       MANAGE_REDIR_STRICT_NO,
                                       posok==2);
}


/* A handler for clientwin_do_manage_alt hook to handle transients for windows
 * on WFloatWS:s differently from the normal behaviour.
 */
bool mod_floatws_clientwin_do_manage(WClientWin *cwin, 
                                     const WManageParams *param)
{
    WRegion *stack_above;
    WFloatStacking *st;
    WFloatWS *ws;
    WRegion *mgr;
    
    if(param->tfor==NULL)
        return FALSE;

    stack_above=OBJ_CAST(REGION_PARENT(param->tfor), WRegion);
    if(stack_above==NULL)
        return FALSE;
    
    ws=REGION_MANAGER_CHK(stack_above, WFloatWS);
    if(ws==NULL)
        return FALSE;
    
    if(!floatws_manage_clientwin(ws, cwin, param, MANAGE_REDIR_PREFER_NO))
        return FALSE;

    mgr=REGION_MANAGER(cwin);
    
    if(stacking!=NULL){
        st=stacking->prev;
        while(1){
            if(st->reg==mgr){
                st->above=stack_above;
                break;
            }
            if(st==stacking)
                break;
            st=st->prev;
        }
    }

    return TRUE;
}


/*}}}*/


/*{{{ Sticky status display support */


static void floatws_place_stdisp(WFloatWS *ws, WWindow *par,
                                 int corner, WRegion *stdisp)
{
    WFitParams fp;
    WRectangle *wg=&REGION_GEOM(ws);
    
    fp.g.w=minof(wg->w, maxof(CF_STDISP_MIN_SZ, region_min_w(stdisp)));
    fp.g.h=minof(wg->h, maxof(CF_STDISP_MIN_SZ, region_min_h(stdisp)));
    
    if(corner==MPLEX_STDISP_TL || corner==MPLEX_STDISP_BL)
        fp.g.x=wg->x;
    else
        fp.g.x=wg->x+wg->w-fp.g.w;

    if(corner==MPLEX_STDISP_TL || corner==MPLEX_STDISP_TR)
        fp.g.y=wg->y;
    else
        fp.g.y=wg->y+wg->h-fp.g.h;
        
    fp.mode=REGION_FIT_EXACT;
    
    region_fitrep(stdisp, par, &fp);
}


void floatws_manage_stdisp(WFloatWS *ws, WRegion *stdisp, int corner)
{
    if(REGION_MANAGER(stdisp)==(WRegion*)ws)
        return;
    
    region_detach_manager(stdisp);
    
    floatws_add_managed(ws, stdisp);

    ws->stdisp_corner=corner;
    ws->managed_stdisp=stdisp;
    
    floatws_place_stdisp(ws, NULL, corner, stdisp);
}


/*}}}*/


/*{{{ Circulate */


/*EXTL_DOC
 * Activate next object in stacking order on \var{ws}.
 */
EXTL_EXPORT_MEMBER
WRegion *floatws_circulate(WFloatWS *ws)
{
    WFloatStacking *st=NULL, *ststart;
    
    if(stacking==NULL)
        return NULL;
    
    if(ws->current_managed!=NULL){
        st=mod_floatws_find_stacking(ws->current_managed);
        if(st!=NULL)
            st=st->next;
    }
    
    if(st==NULL)
        st=stacking;
    ststart=st;
    
    while(1){
        if(REGION_MANAGER(st->reg)==(WRegion*)ws
           && st->reg!=ws->managed_stdisp){
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
WRegion *floatws_backcirculate(WFloatWS *ws)
{
    WFloatStacking *st=NULL, *ststart;
    
    if(stacking==NULL)
        return NULL;
    
    if(ws->current_managed!=NULL){
        st=mod_floatws_find_stacking(ws->current_managed);
        if(st!=NULL)
            st=st->prev;
    }
    
    if(st==NULL)
        st=stacking->prev;
    ststart=st;
    
    while(1){
        if(REGION_MANAGER(st->reg)==(WRegion*)ws
           && st->reg!=ws->managed_stdisp){
            break;
        }
        st=st->next;
        if(st==ststart)
            return NULL;
    }
        
    if(region_may_control_focus((WRegion*)ws))
       region_goto(st->reg);
    
    return st->reg;
}


/*}}}*/


/*{{{ Stacking */


WFloatStacking *mod_floatws_find_stacking(WRegion *r)
{
    WFloatStacking *st;
    
    for(st=stacking; st!=NULL; st=st->next){
        if(st->reg==r)
            return st;
    }
    
    return NULL;
}


void floatws_stacking(WFloatWS *ws, Window *bottomret, Window *topret)
{
    WFloatStacking *st;
    
    /* Ignore dummywin if we manage anything in order to not confuse 
     * the global stacking list 
     */
    
    *topret=None;
    *bottomret=None;
    
    if(stacking!=NULL){
        st=stacking->prev;
        
        while(1){
            Window bottom=None, top=None;
            if(REGION_MANAGER(st->reg)==(WRegion*)ws){
                region_stacking(st->reg, &bottom, &top);
                if(top!=None){
                    *topret=top;
                    break;
                }
            }
            if(st==stacking)
                break;
            st=st->prev;
        }
        
        for(st=stacking; st!=NULL; st=st->next){
            Window bottom=None, top=None;
            if(REGION_MANAGER(st->reg)==(WRegion*)ws){
                region_stacking(st->reg, &bottom, &top);
                if(bottom!=None){
                    *bottomret=top;
                    break;
                }
            }
        }
    }
    
    if(*bottomret==None)
        *bottomret=ws->genws.dummywin;
    if(*topret==None)
        *topret=ws->genws.dummywin;
}


static WFloatStacking *link_lists(WFloatStacking *l1, WFloatStacking *l2)
{
    /* As everywhere, doubly-linked lists without the forward 
     * link in last item! 
     */
    WFloatStacking *tmp=l2->prev;
    l1->prev->next=l2;
    l2->prev=l1->prev;
    l1->prev=tmp;
    return l1;
}


static WFloatStacking *link_list_before(WFloatStacking *l1, 
                                        WFloatStacking *i1,
                                        WFloatStacking *l2)
{
    WFloatStacking *tmp;
    
    if(i1==l1)
        return link_lists(l2, l1);
    
    l2->prev->next=i1;
    i1->prev->next=l2;
    tmp=i1->prev;
    i1->prev=l2->prev;
    l2->prev=tmp;
    
    return l1;
}


static WFloatStacking *link_list_after(WFloatStacking *l1, 
                                        WFloatStacking *i1,
                                        WFloatStacking *l2)
{
    WFloatStacking *tmp;
    
    if(i1==l1->prev)
        return link_lists(l1, l2);
    
    i1->next->prev=l2->prev;
    l2->prev->next=i1->next;
    i1->next=l2;
    l2->prev=i1;
    
    return l1;
}


static WFloatStacking *find_stacking_if_not_on_ws(WFloatWS *ws, Window w)
{
    WRegion *r=xwindow_region_of(w);
    WFloatStacking *st=NULL;
    
    while(r!=NULL){
        if(REGION_PARENT(r)==REGION_PARENT(ws))
            break;
        if(REGION_MANAGER(r)==(WRegion*)ws)
            break;
        st=mod_floatws_find_stacking(r);
        if(st!=NULL)
            break;
        r=REGION_MANAGER(r);
    }
    
    return st;
}


void floatws_restack(WFloatWS *ws, Window other, int mode)
{
    WFloatStacking *st, *stnext, *chain=NULL;
    bool samepar=FALSE;
    Window ref=other;
    WMPlex *par=OBJ_CAST(REGION_PARENT(ws), WMPlex);

    assert(mode==Above || mode==Below);

    xwindow_restack(ws->genws.dummywin, ref, mode);
    ref=ws->genws.dummywin;
    mode=Above;
    
    if(stacking==NULL)
        return;
    
    for(st=stacking; st!=NULL; st=stnext){
        stnext=st->next;
        if(REGION_MANAGER(st->reg)==(WRegion*)ws){
            Window bottom=None, top=None;
            region_restack(st->reg, ref, mode);
            region_stacking(st->reg, &bottom, &top);
            if(top!=None)
                ref=top;
            
            UNLINK_ITEM(stacking, st, next, prev);
            LINK_ITEM(chain, st, next, prev);
        }else if(REGION_PARENT(st->reg)==REGION_PARENT(ws)){
            samepar=TRUE;
        }
    }
    
    if(chain==NULL)
        return;
    
    if(stacking==NULL){
        stacking=chain;
        return;
    }
    
    if(other==None || !samepar || par==NULL){
        WFloatStacking *tmp;
        if(mode==Above)
            stacking=link_lists(stacking, chain);
        else
            stacking=link_lists(chain, stacking);
    }else{
        /* Need to find the point on the list to insert to. */
        Window root=None, parent=None, *children=NULL;
        uint i, n=0;
        /* Use XQueryTree to get things in stacking order. */
        XQueryTree(ioncore_g.dpy, region_xwindow((WRegion*)par),
                   &root, &parent, &children, &n);
        if(mode==Above){
            WFloatStacking *below=NULL, *st;
            for(i=n; i>0; ){
                i--;
                if(children[i]==other)
                    break;
                st=find_stacking_if_not_on_ws(ws, children[i]);
                if(st!=NULL)
                    below=st;
            }
            if(below!=NULL)
                stacking=link_list_before(stacking, below, chain);
            else
                stacking=link_lists(stacking, chain);
        }else{
            WFloatStacking *above=NULL, *st;
            for(i=0; i<n; i++){
                if(children[i]==other)
                    break;
                st=find_stacking_if_not_on_ws(ws, children[i]);
                if(st!=NULL)
                    above=st;
            }
            if(above!=NULL)
                stacking=link_list_after(stacking, above, chain);
            else
                stacking=link_lists(chain, stacking);
        }
        XFree(children);
    }
}


static void floatws_do_raise(WFloatWS *ws, WRegion *reg, bool initial)
{
    WFloatStacking *st, *sttop=NULL, *stabove, *stnext;
    Window bottom=None, top=None, other=None;

    if(reg==NULL || stacking==NULL)
        return;

    if(REGION_MANAGER(reg)!=(WRegion*)ws){
        warn(TR("Region not managed by the workspace."));
        return;
    }
    
    st=stacking->prev;
    while(1){
        if(st->reg==reg)
            break;
        if(st->above!=reg && sttop==NULL && same_stacking(ws, st->reg)){
            region_stacking(st->reg, &bottom, &top);
            if(top!=None){
                other=top;
                sttop=st;
            }
        }
        if(st==stacking) /* reg not found */
            return;
        st=st->prev;
    }
    
    if(sttop!=NULL){
        UNLINK_ITEM(stacking, st, next, prev);
        region_restack(reg, other, Above);
        LINK_ITEM_AFTER(stacking, sttop, st, next, prev);
    }else if(initial){
        region_restack(reg, ws->genws.dummywin, Above);
    }
    
    if(initial)
        return;
    
    region_stacking(reg, &bottom, &top);
    if(top==None)
        return;
    other=top;
    sttop=st;

    for(stabove=stacking; stabove!=NULL && stabove!=st; stabove=stnext){
        stnext=stabove->next;
        
        if(stabove->above==reg){
            UNLINK_ITEM(stacking, stabove, next, prev);
            region_restack(stabove->reg, other, Above);
            LINK_ITEM_AFTER(stacking, sttop, stabove, next, prev);
            region_stacking(stabove->reg, &bottom, &top);
            if(top!=None)
                other=top;
            sttop=stabove;
        }
    }
}


/*EXTL_DOC
 * Raise \var{reg} that must be managed by \var{ws}.
 * If \var{reg} is \code{nil}, this function silently fails.
 */
EXTL_EXPORT_MEMBER
void floatws_raise(WFloatWS *ws, WRegion *reg)
{
    floatws_do_raise(ws, reg, FALSE);
}


/*EXTL_DOC
 * Lower \var{reg} that must be managed by \var{ws}.
 * If \var{reg} is \code{nil}, this function silently fails.
 */
EXTL_EXPORT_MEMBER
void floatws_lower(WFloatWS *ws, WRegion *reg)
{
    WFloatStacking *st, *stbottom=NULL, *stabove, *stnext;
    Window bottom=None, top=None, other=None;

    if(reg==NULL || stacking==NULL)
        return;

    if(REGION_MANAGER(reg)!=(WRegion*)ws){
        warn(TR("Region not managed by the workspace."));
        return;
    }
    
    for(st=stacking; st!=NULL; st=st->next){
        if(st->reg==reg)
            break;
        if(stbottom==NULL && same_stacking(ws, st->reg)){
            region_stacking(st->reg, &bottom, &top);
            if(bottom!=None){
                other=bottom;
                stbottom=st;
            }
        }
    }
    
    if(st!=NULL){
        if(stbottom==NULL){
            region_restack(reg, ws->genws.dummywin, Above);
        }else{
            UNLINK_ITEM(stacking, st, next, prev);
            region_restack(reg, other, Below);
            LINK_ITEM_BEFORE(stacking, stbottom, st, next, prev);
        }
    }
}


/*}}}*/


/*{{{ Misc. */


/*EXTL_DOC
 * Returns a list of regions managed by the workspace (frames, mostly).
 */
EXTL_EXPORT_MEMBER
ExtlTab floatws_managed_list(WFloatWS *ws)
{
    WFloatWSIterTmp tmp;
    floatws_iter_init(&tmp, ws);
    
    return extl_obj_iterable_to_table((ObjIterator*)floatws_iter, &tmp);
}


/*EXTL_DOC
 * Returns the object that currently has or previously had focus on \var{ws}
 * (if no other object on the workspace currently has focus).
 */
EXTL_EXPORT_MEMBER
WRegion* floatws_current(WFloatWS *ws)
{
    return ws->current_managed;
}


/*}}}*/


/*{{{ Save/load */


static ExtlTab floatws_get_configuration(WFloatWS *ws)
{
    ExtlTab tab, mgds, subtab, g;
    WFloatStacking *st;
    WFloatWSIterTmp tmp;
    WRegion *mgd;
    WMPlex *par;
    int n=0;
    
    tab=region_get_base_configuration((WRegion*)ws);
    
    mgds=extl_create_table();
    
    extl_table_sets_t(tab, "managed", mgds);
    
    FOR_ALL_MANAGED_BY_FLOATWS(ws, mgd, tmp){
        subtab=region_get_configuration(mgd);

        g=extl_table_from_rectangle(&REGION_GEOM(mgd));
        extl_table_sets_t(subtab, "geom", g);
        extl_unref_table(g);
        
        st=mod_floatws_find_stacking(mgd);
        if(st!=NULL && st->sticky)
            extl_table_sets_b(subtab, "sticky", TRUE);
        
        extl_table_seti_t(mgds, ++n, subtab);
        extl_unref_table(subtab);
    }
    
    extl_unref_table(mgds);
    
    return tab;
}


static WRegion *floatws_do_attach(WFloatWS *ws, WRegionAttachHandler *fn,
                                  void *fnparams, const WFitParams *fp)
{
    WWindow *par;
    WRegion *reg;

    par=REGION_PARENT(ws);
    assert(par!=NULL);
    
    reg=fn(par, fp, fnparams);

    if(reg!=NULL)
        floatws_add_managed(ws, reg);
    
    return reg;
}



static WRegion *floatws_attach_load(WFloatWS *ws, ExtlTab param)
{
    WRectangle geom;
    WRegion *reg;
    
    if(!extl_table_gets_rectangle(param, "geom", &geom)){
        warn(TR("No geometry specified."));
        return NULL;
    }

    geom.w=maxof(geom.w, 0);
    geom.h=maxof(geom.h, 0);
    
    reg=region__attach_load((WRegion*)ws, param, 
                            (WRegionDoAttachFn*)floatws_do_attach,
                            &geom);
    
    if(reg!=NULL && extl_table_is_bool_set(param, "sticky")){
        WFloatStacking *st=mod_floatws_find_stacking(reg);
        if(st!=NULL)
            st->sticky=TRUE;
    }
    
    return reg;
}


WRegion *floatws_load(WWindow *par, const WFitParams *fp, ExtlTab tab)
{
    WFloatWS *ws;
    ExtlTab substab, subtab;
    int i, n;
    
    ws=create_floatws(par, fp);
    
    if(ws==NULL)
        return NULL;
        
    if(!extl_table_gets_t(tab, "managed", &substab))
        return (WRegion*)ws;

    n=extl_table_get_n(substab);
    for(i=1; i<=n; i++){
        if(extl_table_geti_t(substab, i, &subtab)){
            floatws_attach_load(ws, subtab);
            extl_unref_table(subtab);
        }
    }
    
    extl_unref_table(substab);

    return (WRegion*)ws;
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab floatws_dynfuntab[]={
    {(DynFun*)region_fitrep,
     (DynFun*)floatws_fitrep},

    {region_map, 
     floatws_map},
    {region_unmap, 
     floatws_unmap},
    {(DynFun*)region_managed_goto, 
     (DynFun*)floatws_managed_goto},

    {region_do_set_focus, 
     floatws_do_set_focus},
    {region_managed_activated, 
     floatws_managed_activated},
    
    {(DynFun*)region_manage_clientwin, 
     (DynFun*)floatws_manage_clientwin},
    {(DynFun*)region_handle_drop,
     (DynFun*)floatws_handle_drop},
    {region_managed_remove,
     floatws_managed_remove},
    
    {(DynFun*)region_get_configuration, 
     (DynFun*)floatws_get_configuration},

    {(DynFun*)region_rqclose,
     (DynFun*)floatws_rqclose},

    {(DynFun*)region_current,
     (DynFun*)floatws_current},
    
    {(DynFun*)region_rescue_clientwins,
     (DynFun*)floatws_rescue_clientwins},
    
    {genws_manage_stdisp,
     floatws_manage_stdisp},
    
    {region_restack,
     floatws_restack},

    {region_stacking,
     floatws_stacking},

    {(DynFun*)region_managed_get_pholder,
     (DynFun*)floatws_managed_get_pholder},

    {(DynFun*)region_get_rescue_pholder_for,
     (DynFun*)floatws_get_rescue_pholder_for},
    
    END_DYNFUNTAB
};


IMPLCLASS(WFloatWS, WGenWS, floatws_deinit, floatws_dynfuntab);


/*}}}*/

