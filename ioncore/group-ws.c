/*
 * ion/ioncore/placement.c
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

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/region.h>
#include <ioncore/focus.h>
#include <ioncore/group.h>
#include <ioncore/regbind.h>
#include <ioncore/bindmaps.h>

#include "group-ws.h"
#include "group-ws-rescueph.h"
#include "floatframe.h"

#define FOR_ALL_MANAGED_BY_GROUPWS(A, B, C) \
    FOR_ALL_MANAGED_BY_GROUP(&((A)->grp), B, C)


/*{{{ Placement policies */


static void random_placement(WRectangle box, WRectangle *g)
{
    box.w-=g->w;
    box.h-=g->h;
    g->x=box.x+(box.w<=0 ? 0 : rand()%box.w);
    g->y=box.y+(box.h<=0 ? 0 : rand()%box.h);
}


static void ggeom(WRegion *reg, WRectangle *geom)
{
    *geom=REGION_GEOM(reg);
}


static WRegion* is_occupied(WGroupWS *ws, const WRectangle *r)
{
    WRegion *reg;
    WRectangle p;
    WGroupIterTmp tmp;
    
    FOR_ALL_MANAGED_BY_GROUPWS(ws, reg, tmp){
        ggeom(reg, &p);
        
        if(r->x>=p.x+p.w)
            continue;
        if(r->y>=p.y+p.h)
            continue;
        if(r->x+r->w<=p.x)
            continue;
        if(r->y+r->h<=p.y)
            continue;
        return reg;
    }
    
    return NULL;
}


static int next_least_x(WGroupWS *ws, int x)
{
    WRegion *reg;
    WRectangle p;
    int retx=REGION_GEOM(ws).x+REGION_GEOM(ws).w;
    WGroupIterTmp tmp;
    
    FOR_ALL_MANAGED_BY_GROUPWS(ws, reg, tmp){
        ggeom(reg, &p);
        
        if(p.x+p.w>x && p.x+p.w<retx)
            retx=p.x+p.w;
    }
    
    return retx+1;
}


static int next_lowest_y(WGroupWS *ws, int y)
{
    WRegion *reg;
    WRectangle p;
    int rety=REGION_GEOM(ws).y+REGION_GEOM(ws).h;
    WGroupIterTmp tmp;
    
    FOR_ALL_MANAGED_BY_GROUPWS(ws, reg, tmp){
        ggeom(reg, &p);
        
        if(p.y+p.h>y && p.y+p.h<rety)
            rety=p.y+p.h;
    }
    
    return rety+1;
}


static enum{
    PLACEMENT_LRUD, PLACEMENT_UDLR, PLACEMENT_RANDOM
} placement_method=PLACEMENT_LRUD;

static bool default_ws_params_set=FALSE;
static ExtlTab default_ws_params;


/*EXTL_DOC
 * Set module basic settings. Currently only the \code{placement_method} 
 * parameter is supported.
 *
 * The method can be one  of ''udlr'', ''lrud'' (default) and ''random''. 
 * The ''udlr'' method looks for free space starting from top the top left
 * corner of the workspace moving first down keeping the x coordinate fixed.
 * If it find no free space, it start looking similarly at next x coordinate
 * unoccupied by other objects and so on. ''lrud' is the same but with the 
 * role of coordinates changed and both fall back to ''random'' placement 
 * if no free area was found.
 */

void ioncore_groupws_set(ExtlTab tab)
{
    char *method=NULL;
    ExtlTab t;
    
    if(extl_table_gets_s(tab, "float_placement_method", &method)){
        if(strcmp(method, "udlr")==0)
            placement_method=PLACEMENT_UDLR;
        else if(strcmp(method, "lrud")==0)
            placement_method=PLACEMENT_LRUD;
        else if(strcmp(method, "random")==0)
            placement_method=PLACEMENT_RANDOM;
        else
            warn(TR("Unknown placement method \"%s\"."), method);
        free(method);
    }
    
    if(extl_table_gets_t(tab, "default_ws_params", &t)){
        if(default_ws_params_set)
            extl_unref_table(default_ws_params);
        default_ws_params=t;
        default_ws_params_set=TRUE;
    }
}


void ioncore_groupws_get(ExtlTab t)
{
    extl_table_sets_s(t, "float_placement_method", 
                      (placement_method==PLACEMENT_UDLR
                       ? "udlr" 
                       : (placement_method==PLACEMENT_LRUD
                          ? "lrud" 
                          : "random")));
    
    if(default_ws_params_set)
        extl_table_sets_t(t, "default_ws_params", default_ws_params);
}


static bool tiling_placement(WGroupWS *ws, WRectangle *g)
{
    WRegion *p;
    WRectangle r, r2;
    int maxx, maxy;
    
    r=REGION_GEOM(ws);
    r.w=g->w;
    r.h=g->h;

    maxx=REGION_GEOM(ws).x+REGION_GEOM(ws).w;
    maxy=REGION_GEOM(ws).y+REGION_GEOM(ws).h;
    
    if(placement_method==PLACEMENT_UDLR){
        while(r.x<maxx){
            p=is_occupied(ws, &r);
            while(p!=NULL && r.y+r.h<maxy){
                ggeom(p, &r2);
                r.y=r2.y+r2.h+1;
                p=is_occupied(ws, &r);
            }
            if(r.y+r.h<maxy && r.x+r.w<maxx){
                g->x=r.x;
                g->y=r.y;
                return TRUE;
            }else{
                r.x=next_least_x(ws, r.x);
                r.y=0;
            }
        }
    }else{
        while(r.y<maxy){
            p=is_occupied(ws, &r);
            while(p!=NULL && r.x+r.w<maxx){
                ggeom(p, &r2);
                r.x=r2.x+r2.w+1;
                p=is_occupied(ws, &r);
            }
            if(r.y+r.h<maxy && r.x+r.w<maxx){
                g->x=r.x;
                g->y=r.y;
                return TRUE;
            }else{
                r.y=next_lowest_y(ws, r.y);
                r.x=0;
            }
        }
    }

    return FALSE;

}


void groupws_calc_placement(WGroupWS *ws, WRectangle *geom)
{
    if(placement_method!=PLACEMENT_RANDOM){
        if(tiling_placement(ws, geom))
            return;
    }
    random_placement(REGION_GEOM(ws), geom);
}


/*}}}*/


/*{{{ Attach w/ frame */

WFloatFrame *groupws_create_frame(WGroupWS *ws, const WRectangle *geom, 
                                  bool inner_geom, bool respect_pos, 
                                  int gravity)
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
        groupws_calc_placement(ws, &fp.g);

    /* Set proper geometry */
    region_fit((WRegion*)frame, &fp.g, REGION_FIT_EXACT);

    if(!group_do_add_managed(&ws->grp, (WRegion*)frame, 1, 
                               SIZEPOLICY_UNCONSTRAINED)){
        destroy_obj((Obj*)frame);
        return NULL;
    }

    return frame;
}


bool groupws_phattach(WGroupWS *ws, 
                      WRegionAttachHandler *hnd, void *hnd_param,
                      WGroupWSPHAttachParams *p)
{
    bool newframe=FALSE;
    WStacking *st, *stabove;
    WMPlexAttachParams par;
    WStacking *stacking=group_get_stacking(&ws->grp);

    par.flags=(p->aflags&PHOLDER_ATTACH_SWITCHTO ? MPLEX_ATTACH_SWITCHTO : 0);
    
    if(p->frame==NULL){
        p->frame=(WFrame*)groupws_create_frame(ws, &(p->geom), p->inner_geom,
                                               p->pos_ok, p->gravity);
        
        if(p->frame==NULL)
            return FALSE;
        
        newframe=TRUE;
        
        
        if(stacking!=NULL && p->stack_above!=NULL){
            /* TODO: should not need to scan for st, as we just 
             * created it
             */
            st=group_find_stacking(ws, (WRegion*)p->frame);
            stabove=group_find_stacking(ws, (WRegion*)p->stack_above);
            
            if(st!=NULL && stabove!=NULL){
                st->above=stabove;
                st->level=stabove->level;
            }
        }
    }
    
    if(mplex_do_attach((WMPlex*)p->frame, hnd, hnd_param, &par)==NULL){
        if(newframe){
            destroy_obj((Obj*)p->frame);
            p->frame=NULL;
        }
        return FALSE;
    }

    /* Don't warp, it is annoying in this case */
    if(newframe && p->aflags&PHOLDER_ATTACH_SWITCHTO
       && region_may_control_focus((WRegion*)ws)){
        region_set_focus((WRegion*)p->frame);
    }
    
    return TRUE;
}


bool groupws_attach_framed(WGroupWS *ws, WRegion *reg,
                           WGroupWSPHAttachParams *p)
{
    return (region__attach_reparent((WRegion*)ws, reg,
                                    (WRegionDoAttachFn*)groupws_phattach, p)
            !=NULL);

}


bool groupws_handle_drop(WGroupWS *ws, int x, int y,
                         WRegion *dropped)
{
    WGroupWSPHAttachParams p;
    
    p.frame=NULL;
    p.geom.x=x;
    p.geom.y=y;
    p.geom.w=REGION_GEOM(dropped).w;
    p.geom.h=REGION_GEOM(dropped).h;
    p.inner_geom=TRUE;
    p.pos_ok=TRUE;
    p.gravity=NorthWestGravity;
    p.aflags=PHOLDER_ATTACH_SWITCHTO;
    p.stack_above=NULL;
    
    return groupws_attach_framed(ws, dropped, &p);
}


/*EXTL_DOC
 * Attach client window \var{cwin} on \var{ws}.
 * At least the following fields in \var{t} are supported:
 * 
 * \begin{tabularx}{\linewidth}{lX}
 *  \tabhead{Field & Description}
 *  \var{switchto} & Should the region be switched to (boolean)? Optional. \\
 *  \var{geom} & Geometry; \var{x} and \var{y}, if set, indicates top-left of 
 *   the frame to be created while \var{width} and \var{height}, if set, indicate
 *   the size of the client window within that frame. Optional.
 * \end{tabularx}
 */
EXTL_EXPORT_AS(WGroupWS, attach_framed)
bool groupws_attach_framed_extl(WGroupWS *ws, WClientWin *cwin, ExtlTab t)
{
    int posok=0;
    ExtlTab gt;
    WGroupWSPHAttachParams p;
    
    if(cwin==NULL)
        return FALSE;
    
    p.frame=NULL;
    p.geom.x=0;
    p.geom.y=0;
    p.geom.w=REGION_GEOM(cwin).w;
    p.geom.h=REGION_GEOM(cwin).h;
    p.inner_geom=TRUE;
    p.gravity=ForgetGravity;
    p.aflags=0;
    p.stack_above=NULL;

    if(extl_table_is_bool_set(t, "switchto"))
        p.aflags|=PHOLDER_ATTACH_SWITCHTO;
    
    if(extl_table_gets_t(t, "geom", &gt)){
        if(extl_table_gets_i(gt, "x", &(p.geom.x)))
            posok++;
        if(extl_table_gets_i(gt, "y", &(p.geom.y)))
            posok++;
    
        extl_table_gets_i(gt, "w", &(p.geom.w));
        extl_table_gets_i(gt, "h", &(p.geom.h));
        
        extl_unref_table(gt);
    }
    
    p.geom.w=maxof(0, p.geom.w);
    p.geom.h=maxof(0, p.geom.h);
    p.pos_ok=(posok==2);
    
    return groupws_attach_framed(ws, (WRegion*)cwin, &p);
}


/*}}}*/


/*{{{ groupws_prepare_manage */


#define REG_OK(R) OBJ_IS(R, WMPlex)


static WMPlex *find_existing(WGroupWS *ws)
{
    WGroupIterTmp tmp;
    WRegion *r=(ws->grp.current_managed!=NULL 
                ? ws->grp.current_managed->reg 
                : NULL);
    
    if(r!=NULL && REG_OK(r))
        return (WMPlex*)r;
    
    FOR_ALL_MANAGED_BY_GROUPWS(ws, r, tmp){
        if(REG_OK(r))
            return (WMPlex*)r;
    }
    
    return NULL;
}


static WGroupWSRescuePH *groupws_prepare_manage_in_frame(WGroupWS *ws, 
                                                         const WClientWin *cwin,
                                                         const WManageParams *param, 
                                                         bool respect_pos)
{
    if(param->maprq && ioncore_g.opmode!=IONCORE_OPMODE_INIT){
        /* When the window is mapped by application request, position
         * request is only honoured if the position was given by the user
         * and in case of a transient (the app may know better where to 
         * place them) or if we're initialising.
         */
        respect_pos=(param->tfor!=NULL || param->userpos);
    }

    return create_groupwsrescueph(ws, &(param->geom), respect_pos, 
                                  TRUE, param->gravity);
}


static WPHolder *groupws_do_prepare_manage(WGroupWS *ws, 
                                           const WClientWin *cwin,
                                           const WManageParams *param, 
                                           int redir, bool respect_pos)
{
    WPHolder *ph;
        
    if(redir==MANAGE_REDIR_PREFER_YES){
        WMPlex *m=find_existing(ws);
        if(m!=NULL){
            ph=region_prepare_manage((WRegion*)m, cwin, param,
                                     MANAGE_REDIR_STRICT_YES);
            if(ph!=NULL)
                return ph;
        }
    }
    
    if(redir==MANAGE_REDIR_STRICT_YES)
        return NULL;

    return (WPHolder*) groupws_prepare_manage_in_frame(ws, cwin, param,
                                                       respect_pos);
}


WPHolder *groupws_prepare_manage(WGroupWS *ws, const WClientWin *cwin,
                                 const WManageParams *param,
                                 int redir)
{
    WRegion *b=(ws->grp.bottom!=NULL ? ws->grp.bottom->reg : NULL);
    
    if(b!=NULL && HAS_DYN(b, region_prepare_manage)){
        WPHolder *ph=region_prepare_manage(b, cwin, param, redir);
        if(ph!=NULL)
            return ph;
    }
    
    return groupws_do_prepare_manage(ws, cwin, param, redir, TRUE);
}


WPHolder *groupws_prepare_manage_transient(WGroupWS *ws, const WClientWin *cwin,
                                           const WManageParams *param,
                                           int unused)
{
    WGroupWSRescuePH *ph;
    WRegion *stack_above;
    
    stack_above=OBJ_CAST(REGION_PARENT(param->tfor), WRegion);
    if(stack_above==NULL)
        return NULL;
    ws=REGION_MANAGER_CHK(stack_above, WGroupWS);
    if(ws==NULL)
        return NULL;
    
    ph=groupws_prepare_manage_in_frame(ws, cwin, param, TRUE);
    
    if(ph!=NULL)
        watch_setup(&(ph->stack_above_watch), (Obj*)stack_above, NULL);

    return (WPHolder*)ph;
}


/*}}}*/


/*{{{ WGroupWS class */


bool groupws_init(WGroupWS *ws, WWindow *parent, const WFitParams *fp)
{
    if(!group_init(&(ws->grp), parent, fp))
        return FALSE;

    ((WRegion*)ws)->flags|=REGION_GRAB_ON_PARENT;
    
    region_add_bindmap((WRegion*)ws, ioncore_groupws_bindmap);
    
    return TRUE;
}


WGroupWS *create_groupws(WWindow *parent, const WFitParams *fp)
{
    CREATEOBJ_IMPL(WGroupWS, groupws, (p, parent, fp));
}


void groupws_deinit(WGroupWS *ws)
{    
    group_deinit(&(ws->grp));
}


WRegion *groupws_load(WWindow *par, const WFitParams *fp, 
                       ExtlTab tab)
{
    WGroupWS *ws;
    
    ws=create_groupws(par, fp);
    
    if(ws==NULL)
        return NULL;

    group_do_load(&ws->grp, tab);
    
    return (WRegion*)ws;
}


WRegion *groupws_load_default(WWindow *par, const WFitParams *fp)
{
    return groupws_load(par, fp, (default_ws_params_set
                                  ? default_ws_params
                                  : extl_table_none()));
}


static DynFunTab groupws_dynfuntab[]={
    {(DynFun*)region_prepare_manage, 
     (DynFun*)groupws_prepare_manage},
    
    {(DynFun*)region_prepare_manage_transient,
     (DynFun*)groupws_prepare_manage_transient},
    
    {(DynFun*)region_handle_drop,
     (DynFun*)groupws_handle_drop},
    
    {(DynFun*)region_get_rescue_pholder_for,
     (DynFun*)groupws_get_rescue_pholder_for},
    
    {region_manage_stdisp,
     group_manage_stdisp},
    
    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WGroupWS, WGroup, groupws_deinit, groupws_dynfuntab);


/*}}}*/

