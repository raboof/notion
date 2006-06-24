/*
 * ion/mod_floatws/placement.c
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

#include "main.h"
#include "floatws.h"
#include "floatframe.h"
#include "floatwsrescueph.h"

#define FOR_ALL_MANAGED_BY_FLOATWS(A, B, C) \
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


static WRegion* is_occupied(WFloatWS *ws, const WRectangle *r)
{
    WRegion *reg;
    WRectangle p;
    WGroupIterTmp tmp;
    
    FOR_ALL_MANAGED_BY_FLOATWS(ws, reg, tmp){
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


static int next_least_x(WFloatWS *ws, int x)
{
    WRegion *reg;
    WRectangle p;
    int retx=REGION_GEOM(ws).x+REGION_GEOM(ws).w;
    WGroupIterTmp tmp;
    
    FOR_ALL_MANAGED_BY_FLOATWS(ws, reg, tmp){
        ggeom(reg, &p);
        
        if(p.x+p.w>x && p.x+p.w<retx)
            retx=p.x+p.w;
    }
    
    return retx+1;
}


static int next_lowest_y(WFloatWS *ws, int y)
{
    WRegion *reg;
    WRectangle p;
    int rety=REGION_GEOM(ws).y+REGION_GEOM(ws).h;
    WGroupIterTmp tmp;
    
    FOR_ALL_MANAGED_BY_FLOATWS(ws, reg, tmp){
        ggeom(reg, &p);
        
        if(p.y+p.h>y && p.y+p.h<rety)
            rety=p.y+p.h;
    }
    
    return rety+1;
}


enum{
    PLACEMENT_LRUD, PLACEMENT_UDLR, PLACEMENT_RANDOM
} placement_method=PLACEMENT_LRUD;


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
EXTL_EXPORT
void mod_floatws_set(ExtlTab tab)
{
    char *method=NULL;
    
    if(extl_table_gets_s(tab, "placement_method", &method)){
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
}

/*EXTL_DOC
 * Get module basic settings. See \fnref{mod_floatws.set} for more 
 * information.
 */
EXTL_SAFE
EXTL_EXPORT
ExtlTab mod_floatws_get()
{
    ExtlTab t=extl_create_table();
    extl_table_sets_s(t, "placement_method", 
                      (placement_method==PLACEMENT_UDLR
                       ? "udlr" 
                       : (placement_method==PLACEMENT_LRUD
                          ? "lrud" 
                          : "random")));
    return t;
}

static bool tiling_placement(WFloatWS *ws, WRectangle *g)
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


void floatws_calc_placement(WFloatWS *ws, WRectangle *geom)
{
    if(placement_method!=PLACEMENT_RANDOM){
        if(tiling_placement(ws, geom))
            return;
    }
    random_placement(REGION_GEOM(ws), geom);
}


/*}}}*/


/*{{{ Attach w/ frame */

WFloatFrame *floatws_create_frame(WFloatWS *ws, const WRectangle *geom, 
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
        floatws_calc_placement(ws, &fp.g);

    /* Set proper geometry */
    region_fit((WRegion*)frame, &fp.g, REGION_FIT_EXACT);

    if(!group_do_add_managed(&ws->grp, (WRegion*)frame, 1, 
                               SIZEPOLICY_UNCONSTRAINED)){
        destroy_obj((Obj*)frame);
        return NULL;
    }

    return frame;
}


bool floatws_phattach(WFloatWS *ws, 
                      WRegionAttachHandler *hnd, void *hnd_param,
                      WFloatWSPHAttachParams *p)
{
    bool newframe=FALSE;
    WStacking *st, *stabove;
    WMPlexAttachParams par;
    WStacking *stacking=group_get_stacking(&ws->grp);

    par.flags=(p->aflags&PHOLDER_ATTACH_SWITCHTO ? MPLEX_ATTACH_SWITCHTO : 0);
    
    if(p->frame==NULL){
        p->frame=(WFrame*)floatws_create_frame(ws, &(p->geom), p->inner_geom,
                                               p->pos_ok, p->gravity);
        
        if(p->frame==NULL)
            return FALSE;
        
        newframe=TRUE;
        
        
        if(stacking!=NULL && p->stack_above!=NULL){
            /* TODO: should not need to scan for st, as we just 
             * created it
             */
            st=stacking_find(stacking, (WRegion*)p->frame);
            stabove=stacking_find(stacking, (WRegion*)p->stack_above);
            
            if(st!=NULL){
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


bool floatws_attach_framed(WFloatWS *ws, WRegion *reg,
                           WFloatWSPHAttachParams *p)
{
    return (region__attach_reparent((WRegion*)ws, reg,
                                    (WRegionDoAttachFn*)floatws_phattach, p)
            !=NULL);

}


bool floatws_handle_drop(WFloatWS *ws, int x, int y,
                         WRegion *dropped)
{
    WFloatWSPHAttachParams p;
    
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
    
    return floatws_attach_framed(ws, dropped, &p);
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
EXTL_EXPORT_AS(WFloatWS, attach_framed)
bool floatws_attach_framed_extl(WFloatWS *ws, WClientWin *cwin, ExtlTab t)
{
    int posok=0;
    ExtlTab gt;
    WFloatWSPHAttachParams p;
    
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
    
    return floatws_attach_framed(ws, (WRegion*)cwin, &p);
}


/*}}}*/


/*{{{ floatws_prepare_manage */


#define REG_OK(R) OBJ_IS(R, WMPlex)


static WMPlex *find_existing(WFloatWS *ws)
{
    WGroupIterTmp tmp;
    WRegion *r=(ws->grp.current_managed!=NULL 
                ? ws->grp.current_managed->reg 
                : NULL);
    
    if(r!=NULL && REG_OK(r))
        return (WMPlex*)r;
    
    FOR_ALL_MANAGED_BY_FLOATWS(ws, r, tmp){
        if(REG_OK(r))
            return (WMPlex*)r;
    }
    
    return NULL;
}


static WFloatWSRescuePH *floatws_prepare_manage_in_frame(WFloatWS *ws, 
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

    return create_floatwsrescueph(ws, &(param->geom), respect_pos, 
                                  TRUE, param->gravity);
}


static WPHolder *floatws_do_prepare_manage(WFloatWS *ws, 
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

    return (WPHolder*) floatws_prepare_manage_in_frame(ws, cwin, param,
                                                       respect_pos);
}


WPHolder *floatws_prepare_manage(WFloatWS *ws, const WClientWin *cwin,
                                 const WManageParams *param,
                                 int redir)
{
    return floatws_do_prepare_manage(ws, cwin, param, redir, TRUE);
}


WPHolder *floatws_prepare_manage_transient(WFloatWS *ws, const WClientWin *cwin,
                                           const WManageParams *param,
                                           int unused)
{
    WFloatWSRescuePH *ph;
    WRegion *stack_above;
    
    stack_above=OBJ_CAST(REGION_PARENT(param->tfor), WRegion);
    if(stack_above==NULL)
        return NULL;
    ws=REGION_MANAGER_CHK(stack_above, WFloatWS);
    if(ws==NULL)
        return NULL;
    
    ph=floatws_prepare_manage_in_frame(ws, cwin, param, TRUE);
    
    if(ph!=NULL)
        watch_setup(&(ph->stack_above_watch), (Obj*)stack_above, NULL);

    return (WPHolder*)ph;
}


/*}}}*/


/*{{{ WFloatWS class */


static bool floatws_init(WFloatWS *ws, WWindow *parent, const WFitParams *fp)
{
    if(!group_init(&(ws->grp), parent, fp))
        return FALSE;

    ((WRegion*)ws)->flags|=REGION_GRAB_ON_PARENT;
    
    region_add_bindmap((WRegion*)ws, mod_floatws_floatws_bindmap);
    
    return TRUE;
}


WFloatWS *create_floatws(WWindow *parent, const WFitParams *fp)
{
    CREATEOBJ_IMPL(WFloatWS, floatws, (p, parent, fp));
}


void floatws_deinit(WFloatWS *ws)
{    
    group_deinit(&(ws->grp));
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
            group_attach_new(&ws->grp, subtab);
            extl_unref_table(subtab);
        }
    }
    
    extl_unref_table(substab);

    return (WRegion*)ws;
}


static DynFunTab floatws_dynfuntab[]={
    {(DynFun*)region_prepare_manage, 
     (DynFun*)floatws_prepare_manage},
    
    {(DynFun*)region_prepare_manage_transient,
     (DynFun*)floatws_prepare_manage_transient},
    
    {(DynFun*)region_handle_drop,
     (DynFun*)floatws_handle_drop},
    
    {(DynFun*)region_get_rescue_pholder_for,
     (DynFun*)floatws_get_rescue_pholder_for},
    
    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WFloatWS, WGroup, floatws_deinit, floatws_dynfuntab);


/*}}}*/

