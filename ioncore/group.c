/*
 * ion/ioncore/group.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#include <string.h>

#include <libtu/minmax.h>
#include <libtu/objp.h>
#include <libmainloop/defer.h>

#include "common.h"
#include "rootwin.h"
#include "focus.h"
#include "global.h"
#include "region.h"
#include "manage.h"
#include "screen.h"
#include "names.h"
#include "saveload.h"
#include "attach.h"
#include "regbind.h"
#include "extlconv.h"
#include "xwindow.h"
#include "resize.h"
#include "stacking.h"
#include "sizepolicy.h"
#include "bindmaps.h"
#include "navi.h"
#include "sizehint.h"
#include "llist.h"
#include "mplex.h"
#include "group.h"
#include "grouppholder.h"
#include "frame.h"
#include "float-placement.h"
#include "return.h"


static void group_place_stdisp(WGroup *ws, WWindow *parent,
                                 int pos, WRegion *stdisp);

static void group_remanage_stdisp(WGroup *ws);


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
    WStacking *unweaved=NULL;
    int xdiff=0, ydiff=0;
    WStacking *st;
    WWindow *oldpar;
    WRectangle g;
    
    oldpar=REGION_PARENT(ws);
    
    if(par==NULL){
        if(fp->mode&REGION_FIT_WHATEVER)
            return TRUE;
        REGION_GEOM(ws)=fp->g;
    }else{
        if(!region_same_rootwin((WRegion*)ws, (WRegion*)par))
            return FALSE;
        
        if(ws->managed_stdisp!=NULL && ws->managed_stdisp->reg!=NULL)
            region_detach_manager(ws->managed_stdisp->reg);
        
        assert(ws->managed_stdisp==NULL);
        
        xdiff=fp->g.x-REGION_GEOM(ws).x;
        ydiff=fp->g.y-REGION_GEOM(ws).y;
    
        region_unset_parent((WRegion*)ws);
        XReparentWindow(ioncore_g.dpy, ws->dummywin, par->win, -1, -1);
        region_set_parent((WRegion*)ws, par);

        REGION_GEOM(ws).x=fp->g.x;
        REGION_GEOM(ws).y=fp->g.y;
        if(!(fp->mode&REGION_FIT_WHATEVER)){
            REGION_GEOM(ws).w=fp->g.w;
            REGION_GEOM(ws).h=fp->g.h;
        }
        
        if(oldpar!=NULL)
            unweaved=stacking_unweave(&oldpar->stacking, wsfilt, (void*)ws);
    }

    FOR_ALL_NODES_IN_GROUP(ws, st, tmp){
        WFitParams fp2=*fp;
        
        if(st->reg==NULL)
            continue;
        
        g=REGION_GEOM(st->reg);
        g.x+=xdiff;
        g.y+=ydiff;

        if(fp->mode&REGION_FIT_WHATEVER){
            fp2.g=g;
        }else{
            fp2.g=REGION_GEOM(ws);
            sizepolicy(&st->szplcy, st->reg, &g, REGION_RQGEOM_WEAK_ALL, &fp2);
        }
        
        if(!region_fitrep(st->reg, par, &fp2)){
            warn(TR("Error reparenting %s."), region_name(st->reg));
            region_detach_manager(st->reg);
        }
    }
    
    if(unweaved!=NULL)
        stacking_weave(&par->stacking, &unweaved, FALSE);

    return TRUE;
}


static void group_map(WGroup *ws)
{
    WRegion *reg;
    WGroupIterTmp tmp;

    REGION_MARK_MAPPED(ws);
    XMapWindow(ioncore_g.dpy, ws->dummywin);
    
    FOR_ALL_MANAGED_BY_GROUP(ws, reg, tmp){
        region_map(reg);
    }
}


static void group_unmap(WGroup *ws)
{
    WRegion *reg;
    WGroupIterTmp tmp;

    REGION_MARK_UNMAPPED(ws);
    XUnmapWindow(ioncore_g.dpy, ws->dummywin);

    FOR_ALL_MANAGED_BY_GROUP(ws, reg, tmp){
        region_unmap(reg);
    }
}


static WStacking *find_to_focus(WGroup *ws, WStacking *st, bool group_only)
{
    WStacking *stacking=group_get_stacking(ws);
    
    if(stacking==NULL)
        return st;
                                
    return stacking_find_to_focus_mapped(stacking, st, 
                                         (group_only ? (WRegion*)ws : NULL));
}


static bool group_refocus_(WGroup *ws, WStacking *st)
{
    if(st!=ws->current_managed && st->reg!=NULL){
        if(region_may_control_focus((WRegion*)ws))
            region_set_focus(st->reg);
        else
            ws->current_managed=st;
        return TRUE;
    }
    
    return FALSE;
}


static void group_do_set_focus(WGroup *ws, bool warp)
{
    WStacking *st=ws->current_managed;
    
    if(st==NULL || st->reg==NULL)
        st=find_to_focus(ws, NULL, TRUE);
    
    if(st!=NULL && st->reg!=NULL)
        region_do_set_focus(st->reg, warp);
    else
        region_finalise_focusing((WRegion*)ws, ws->dummywin, warp);
}


static bool group_managed_prepare_focus(WGroup *ws, WRegion *reg, 
                                        int flags, WPrepareFocusResult *res)
{
    WMPlex *mplex=OBJ_CAST(REGION_MANAGER(ws), WMPlex);
    WStacking *st=group_find_stacking(ws, reg);    
    
    if(st==NULL)
        return FALSE;

    if(mplex!=NULL){
        WStacking *node=mplex_find_stacking(mplex, (WRegion*)ws);
        
        if(node==NULL)
            return FALSE;
        
        return mplex_do_prepare_focus(mplex, node, st,
                                      flags, res);
    }else{
        if(!region_prepare_focus((WRegion*)ws, flags, res))
            return FALSE;

        st=find_to_focus(ws, st, FALSE);
        
        if(st==NULL)
            return FALSE;
        
        if(ioncore_g.autoraise && 
           !(flags&REGION_GOTO_ENTERWINDOW) &&
           st->level>STACKING_LEVEL_BOTTOM){
            WStacking **stackingp=group_get_stackingp(ws);
            stacking_restack(stackingp, st, None, NULL, NULL, FALSE);
        }
        
        res->reg=st->reg;
        res->flags=flags;
        
        return (res->reg==reg);
    }
}


void group_managed_remove(WGroup *ws, WRegion *reg)
{
    bool mcf=region_may_control_focus((WRegion*)ws);
    WStacking *st, *next_st=NULL;
    bool was_stdisp=FALSE, was_bottom=FALSE;
    bool was_current=FALSE;
    
    st=group_find_stacking(ws, reg);

    if(st!=NULL){
        next_st=stacking_unstack(REGION_PARENT(ws), st);
        
        UNLINK_ITEM(ws->managed_list, st, mgr_next, mgr_prev);
        
        if(st==ws->managed_stdisp){
            ws->managed_stdisp=NULL;
            was_stdisp=TRUE;
        }
        
        if(st==ws->bottom){
            ws->bottom=NULL;
            was_bottom=TRUE;
        }
            
        if(st==ws->current_managed){
            ws->current_managed=NULL;
            was_current=TRUE;
        }
        
        stacking_unassoc(st);
        stacking_free(st);
    }
    
    region_unset_manager(reg, (WRegion*)ws);
    
    if(!OBJ_IS_BEING_DESTROYED(ws)){
        if(was_bottom && !was_stdisp && ws->managed_stdisp==NULL){
            /* We should probably be managing any stdisp, that 'bottom' 
             * was managing.
             */
            group_remanage_stdisp(ws);
        }
        
        if(was_current){
            /* This may still potentially cause problems when focus
             * change is pending. Perhaps we should use region_await_focus,
             * if it is pointing to our child (and region_may_control_focus 
             * fail if it is pointing somewhere else).
             */
            WStacking *stf=find_to_focus(ws, next_st, TRUE);
            if(stf!=NULL && mcf){
                region_maybewarp_now(stf->reg, FALSE);
            }else{
                ws->current_managed=stf;
            }
        }
    }
}


void group_managed_notify(WGroup *ws, WRegion *reg, WRegionNotify how)
{
    if(how==ioncore_g.notifies.activated || 
       how==ioncore_g.notifies.pseudoactivated){
        ws->current_managed=group_find_stacking(ws, reg);
    }
}


/*}}}*/


/*{{{ Create/destroy */


bool group_init(WGroup *ws, WWindow *par, const WFitParams *fp)
{
    ws->current_managed=NULL;
    ws->managed_stdisp=NULL;
    ws->bottom=NULL;
    ws->managed_list=NULL;
    
    ws->dummywin=XCreateWindow(ioncore_g.dpy, par->win,
                                fp->g.x, fp->g.y, 1, 1, 0,
                                CopyFromParent, InputOnly,
                                CopyFromParent, 0, NULL);
    if(ws->dummywin==None)
        return FALSE;

    region_init(&ws->reg, par, fp);
    region_register(&ws->reg);

    XSelectInput(ioncore_g.dpy, ws->dummywin,
                 FocusChangeMask|KeyPressMask|KeyReleaseMask|
                 ButtonPressMask|ButtonReleaseMask);
    XSaveContext(ioncore_g.dpy, ws->dummywin, ioncore_g.win_context,
                 (XPointer)ws);
    
    ((WRegion*)ws)->flags|=REGION_GRAB_ON_PARENT;
    
    region_add_bindmap((WRegion*)ws, ioncore_group_bindmap);
    
    return TRUE;
}


WGroup *create_group(WWindow *par, const WFitParams *fp)
{
    CREATEOBJ_IMPL(WGroup, group, (p, par, fp));
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

    XDeleteContext(ioncore_g.dpy, ws->dummywin, ioncore_g.win_context);
    XDestroyWindow(ioncore_g.dpy, ws->dummywin);
    ws->dummywin=None;

    region_deinit(&ws->reg);
}


    
bool group_rescue_clientwins(WGroup *ws, WRescueInfo *info)
{
    WGroupIterTmp tmp;
    
    group_iter_init_nostdisp(&tmp, ws);
    
    return region_rescue_some_clientwins((WRegion*)ws, info,
                                         (WRegionIterator*)group_iter,
                                         &tmp);
}


static bool group_empty_for_bottom_stdisp(WGroup *ws)
{
    WGroupIterTmp tmp;
    WStacking *st;
    
    FOR_ALL_NODES_IN_GROUP(ws, st, tmp){
        if(st!=ws->bottom && st!=ws->managed_stdisp)
            return FALSE;
    }
    
    return TRUE;
}


static WRegion *group_managed_disposeroot(WGroup *ws, WRegion *reg)
{
    if(group_bottom(ws)==reg){
        if(group_empty_for_bottom_stdisp(ws))
            return region_disposeroot((WRegion*)ws);
    }
    
    return reg;
}


/*}}}*/


/*{{{ Bottom */


static void group_do_set_bottom(WGroup *grp, WStacking *st)
{
    WStacking *was=grp->bottom;
    
    grp->bottom=st;
    
    if(st!=was){
        if(st==NULL || HAS_DYN(st->reg, region_manage_stdisp))
            group_remanage_stdisp(grp);
        
    }
}


/*EXTL_DOC
 * Sets the `bottom' of \var{ws}. The region \var{reg} must already
 * be managed by \var{ws}, unless \code{nil}.
 */
EXTL_EXPORT_MEMBER
bool group_set_bottom(WGroup *ws, WRegion *reg)
{
    WStacking *st=NULL;
    
    if(reg!=NULL){
        st=group_find_stacking(ws, reg);
        
        if(st==NULL)
            return FALSE;
    }
        
    group_do_set_bottom(ws, st);
    
    return TRUE;
}


/*EXTL_DOC
 * Returns the `bottom' of \var{ws}.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
WRegion *group_bottom(WGroup *ws)
{
    return (ws->bottom!=NULL ? ws->bottom->reg : NULL);
}


/*}}}*/


/*{{{ Attach */


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
    WStacking *st=NULL, *tmp=NULL;
    Window bottom=None, top=None;
    WStacking **stackingp=group_get_stackingp(ws);
    WFrame *frame;
    
    if(stackingp==NULL)
        return NULL;
    
    st=create_stacking();
    
    if(st==NULL)
        return NULL;
    
    if(!stacking_assoc(st, reg)){
        stacking_free(st);
        return  NULL;
    }
    
    frame=OBJ_CAST(reg, WFrame);
    if(frame!=NULL){
        WFrameMode m=frame_mode(frame);
        if(m==FRAME_MODE_TILED || m==FRAME_MODE_TILED_ALT)
            frame_set_mode(frame, FRAME_MODE_FLOATING);
    }

    st->level=level;
    st->szplcy=szplcy;

    LINK_ITEM_FIRST(tmp, st, next, prev);
    stacking_weave(stackingp, &tmp, FALSE);
    assert(tmp==NULL);

    LINK_ITEM(ws->managed_list, st, mgr_next, mgr_prev);
    region_set_manager(reg, (WRegion*)ws);

    if(region_is_fully_mapped((WRegion*)ws))
        region_map(reg);

    return st;
}


static void geom_group_to_parent(WGroup *ws, const WRectangle *g, 
                                 WRectangle *wg)
{
    wg->x=g->x+REGION_GEOM(ws).x;
    wg->y=g->y+REGION_GEOM(ws).y;
    wg->w=maxof(1, g->w);
    wg->h=maxof(1, g->h);
}


bool group_do_attach_final(WGroup *ws, 
                           WRegion *reg,
                           const WGroupAttachParams *param)
{
    WStacking *st, *stabove=NULL;
    WSizePolicy szplcy;
    WFitParams fp;
    WRectangle g;
    uint level;
    int weak;
    bool sw;
    
    /* Stacking  */
    if(param->stack_above!=NULL)
        stabove=group_find_stacking(ws, param->stack_above);

    level=(stabove!=NULL
           ? stabove->level
           : (param->level_set 
              ? param->level 
              : STACKING_LEVEL_NORMAL));
    
    /* Fit */
    szplcy=(param->szplcy_set
            ? param->szplcy
            : (param->bottom
               ? SIZEPOLICY_FULL_EXACT
               : SIZEPOLICY_UNCONSTRAINED));
    
    weak=(param->geom_weak_set
          ? param->geom_weak
          : (param->geom_set
             ? 0
             : REGION_RQGEOM_WEAK_ALL));
    
    if(param->geom_set)
        geom_group_to_parent(ws, &param->geom, &g);
    else
        g=REGION_GEOM(reg);
    
    /* If the requested geometry does not overlap the workspaces's geometry, 
     * position request is never honoured.
     */
    if((g.x+g.w<=REGION_GEOM(ws).x) ||
       (g.x>=REGION_GEOM(ws).x+REGION_GEOM(ws).w)){
        weak|=REGION_RQGEOM_WEAK_X;
    }
       
    if((g.y+g.h<=REGION_GEOM(ws).y) ||
       (g.y>=REGION_GEOM(ws).y+REGION_GEOM(ws).h)){
        weak|=REGION_RQGEOM_WEAK_Y;
    }

    if(weak&(REGION_RQGEOM_WEAK_X|REGION_RQGEOM_WEAK_Y) &&
        (szplcy==SIZEPOLICY_UNCONSTRAINED ||
         szplcy==SIZEPOLICY_FREE ||
         szplcy==SIZEPOLICY_FREE_GLUE /* without flags */)){
        /* TODO: use 'weak'? */
        group_calc_placement(ws, level, &g);
    }

    fp.g=REGION_GEOM(ws);
    fp.mode=REGION_FIT_EXACT;

    sizepolicy(&szplcy, reg, &g, weak, &fp);

    if(rectangle_compare(&fp.g, &REGION_GEOM(reg))!=RECTANGLE_SAME)
        region_fitrep(reg, NULL, &fp);
    
    /* Add */
    st=group_do_add_managed(ws, reg, level, szplcy);
    
    if(st==NULL)
        return FALSE;
    
    if(stabove!=NULL)
        st->above=stabove;

    if(param->bottom)
        group_do_set_bottom(ws, st);
    
    /* Focus */
    sw=(param->switchto_set ? param->switchto : ioncore_g.switchto_new);
    
    if(sw || st->level>=STACKING_LEVEL_MODAL1){
        WStacking *stf=find_to_focus(ws, st, FALSE);
        
        if(stf==st){
            /* Ok, the new region can be focused */
            group_refocus_(ws, stf);
        }
    }
    
    return TRUE;
}


WRegion *group_do_attach(WGroup *ws, 
                         /*const*/ WGroupAttachParams *param,
                         WRegionAttachData *data)
{
    WFitParams fp;
    WWindow *par;
    WRegion *reg;
    
    if(ws->bottom!=NULL && param->bottom){
        warn(TR("'bottom' already set."));
        return NULL;
    }
    
    par=REGION_PARENT(ws);
    assert(par!=NULL);

    if(param->geom_set){
        geom_group_to_parent(ws, &param->geom, &fp.g);
        fp.mode=REGION_FIT_EXACT;
    }else{
        fp.g=REGION_GEOM(ws);
        fp.mode=REGION_FIT_BOUNDS|REGION_FIT_WHATEVER;
    }
    
    return region_attach_helper((WRegion*) ws, par, &fp, 
                                (WRegionDoAttachFn*)group_do_attach_final,
                                /*(const WRegionAttachParams*)*/param, data);
    /*                            ^^^^ doesn't seem to work. */
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
    par->bottom=0;
    
    if(extl_table_is_bool_set(tab, "bottom")){
        par->level=STACKING_LEVEL_BOTTOM;
        par->level_set=1;
        par->bottom=1;
    }
    
    if(extl_table_gets_i(tab, "level", &tmp)){
        if(tmp>=0){
            par->level_set=STACKING_LEVEL_NORMAL;
            par->level=tmp;
        }
    }
    
    if(!par->level_set && extl_table_is_bool_set(tab, "modal")){
        par->level=STACKING_LEVEL_MODAL1;
        par->level_set=1;
    }
    
    if(extl_table_is_bool_set(tab, "switchto"))
        par->switchto=1;

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
    WGroupAttachParams par=GROUPATTACHPARAMS_INIT;
    WRegionAttachData data;

    if(reg==NULL)
        return NULL;
    
    get_params(ws, param, &par);
    
    data.type=REGION_ATTACH_REPARENT;
    data.u.reg=reg;
    
    return group_do_attach(ws, &par, &data);
}


/*EXTL_DOC
 * Create a new region to be managed by \var{ws}. At least the following
 * fields in \var{param} are understood:
 * 
 * \begin{tabularx}{\linewidth}{lX}
 *  \tabhead{Field & Description}
 *  \var{type} & (string) Class of the object to be created. Mandatory. \\
 *  \var{name} & (string) Name of the object to be created. \\
 *  \var{switchto} & (boolean) Should the region be switched to? \\
 *  \var{level} & (integer) Stacking level; default is 1. \\
 *  \var{modal} & (boolean) Make object modal; ignored if level is set. \\
 *  \var{sizepolicy} & (string) Size policy; see Section \ref{sec:sizepolicies}. \\
 * \end{tabularx}
 * 
 * In addition parameters to the region to be created are passed in this 
 * same table.
 */
EXTL_EXPORT_MEMBER
WRegion *group_attach_new(WGroup *ws, ExtlTab param)
{
    WGroupAttachParams par=GROUPATTACHPARAMS_INIT;
    WRegionAttachData data;

    get_params(ws, param, &par);
    
    data.type=REGION_ATTACH_LOAD;
    data.u.tab=param;
    
    return group_do_attach(ws, &par, &data);
}


/*}}}*/


/*{{{ Status display support */


static int stdisp_szplcy(const WMPlexSTDispInfo *di, WRegion *stdisp)
{
    int pos=di->pos;
    
    if(di->fullsize){
        if(region_orientation(stdisp)==REGION_ORIENTATION_VERTICAL){
            if(pos==MPLEX_STDISP_TL || pos==MPLEX_STDISP_BL)
                return SIZEPOLICY_STRETCH_LEFT;
            else
                return SIZEPOLICY_STRETCH_RIGHT;
        }else{
            if(pos==MPLEX_STDISP_TL || pos==MPLEX_STDISP_TR)
                return SIZEPOLICY_STRETCH_TOP;
            else
                return SIZEPOLICY_STRETCH_BOTTOM;
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
    uint szplcy;
    WRegion *b=(ws->bottom==NULL ? NULL : ws->bottom->reg);
    
    /* Check if 'bottom' wants to manage the stdisp. */
    if(b!=NULL 
       && !OBJ_IS_BEING_DESTROYED(b)
       && HAS_DYN(b, region_manage_stdisp)){
        region_manage_stdisp(b, stdisp, di);
        if(REGION_MANAGER(stdisp)==b)
            return;
    }
    
    /* No. */
    
    szplcy=stdisp_szplcy(di, stdisp)|SIZEPOLICY_SHRUNK;
    
    if(ws->managed_stdisp!=NULL && ws->managed_stdisp->reg==stdisp){
        if(ws->managed_stdisp->szplcy==szplcy)
            return;
        ws->managed_stdisp->szplcy=szplcy;
    }else{
        region_detach_manager(stdisp);
        ws->managed_stdisp=group_do_add_managed(ws, stdisp, 
                                                STACKING_LEVEL_ON_TOP, 
                                                szplcy);
    }

    fp.g=REGION_GEOM(ws);
    sizepolicy(&ws->managed_stdisp->szplcy, stdisp, NULL, 0, &fp);

    region_fitrep(stdisp, NULL, &fp);
}


static void group_remanage_stdisp(WGroup *ws)
{
    WMPlex *mplex=OBJ_CAST(REGION_MANAGER(ws), WMPlex);
    
    if(mplex!=NULL && 
       mplex->mx_current!=NULL && 
       mplex->mx_current->st->reg==(WRegion*)ws){
        mplex_remanage_stdisp(mplex);
    }
}


/*}}}*/


/*{{{ Geometry requests */


void group_managed_rqgeom(WGroup *ws, WRegion *reg,
                          const WRQGeomParams *rq,
                          WRectangle *geomret)
{
    WFitParams fp;
    WStacking *st;
        
    st=group_find_stacking(ws, reg);

    if(st==NULL){
        fp.g=rq->geom;
        fp.mode=REGION_FIT_EXACT;
    }else{
        fp.g=REGION_GEOM(ws);
        sizepolicy(&st->szplcy, reg, &rq->geom, rq->flags, &fp);
    }
    
    if(geomret!=NULL)
        *geomret=fp.g;
    
    if(!(rq->flags&REGION_RQGEOM_TRYONLY))
        region_fitrep(reg, NULL, &fp);
}


void group_managed_rqgeom_absolute(WGroup *grp, WRegion *sub, 
                                   const WRQGeomParams *rq,
                                   WRectangle *geomret)
{
    if(grp->bottom!=NULL && grp->bottom->reg==sub){
        region_rqgeom((WRegion*)grp, rq, geomret);
        if(!(rq->flags&REGION_RQGEOM_TRYONLY) && geomret!=NULL)
            *geomret=REGION_GEOM(sub);
    }else{
        WRQGeomParams rq2=*rq;
        rq2.flags&=~REGION_RQGEOM_ABSOLUTE;

        region_managed_rqgeom((WRegion*)grp, sub, &rq2, geomret);
    }
}


/*}}}*/


/*{{{ Navigation */


static WStacking *nxt(WGroup *ws, WStacking *st, bool wrap)
{
    return (st->mgr_next!=NULL 
            ? st->mgr_next 
            : (wrap ? ws->managed_list : NULL));
}


static WStacking *prv(WGroup *ws, WStacking *st, bool wrap)
{
    return (st!=ws->managed_list 
            ? st->mgr_prev
            : (wrap ? st->mgr_prev : NULL));
}


typedef WStacking *NxtFn(WGroup *ws, WStacking *st, bool wrap);


static bool mapped_filt(WStacking *st, void *unused)
{
    return (st->reg!=NULL && REGION_IS_MAPPED(st->reg));
}


static bool focusable(WGroup *ws, WStacking *st, uint min_level)
{
    return (st->reg!=NULL
            && REGION_IS_MAPPED(st->reg)
            && !(st->reg->flags&REGION_SKIP_FOCUS)
            && st->level>=min_level);
}

       
static WStacking *do_get_next(WGroup *ws, WStacking *sti,
                              NxtFn *fn, bool wrap, bool sti_ok)
{
    WStacking *st, *stacking;
    uint min_level=0;
    
    stacking=group_get_stacking(ws);
    
    if(stacking!=NULL)
        min_level=stacking_min_level(stacking, mapped_filt, NULL);

    st=sti;
    while(1){
        st=fn(ws, st, wrap); 
        
        if(st==NULL || st==sti)
            break;
        
        if(focusable(ws, st, min_level))
            return st;
    }

    if(sti_ok && focusable(ws, sti, min_level))
        return sti;
    
    return NULL;
}


static WStacking *group_do_navi_first(WGroup *ws, WRegionNavi nh)
{
    WStacking *lst=ws->managed_list;
    
    if(lst==NULL)
        return NULL;

    if(nh==REGION_NAVI_ANY && 
       ws->current_managed!=NULL &&
       ws->current_managed->reg!=NULL){
        return ws->current_managed;
    }
    
    if(nh==REGION_NAVI_ANY || nh==REGION_NAVI_END || 
       nh==REGION_NAVI_BOTTOM || nh==REGION_NAVI_RIGHT){
        return do_get_next(ws, lst, prv, TRUE, TRUE);
    }else{
        return do_get_next(ws, lst->mgr_prev, nxt, TRUE, TRUE);
    }
}


static WRegion *group_navi_first(WGroup *ws, WRegionNavi nh,
                                 WRegionNaviData *data)
{
    WStacking *st=group_do_navi_first(ws, nh);
    
    return region_navi_cont(&ws->reg, (st!=NULL ? st->reg : NULL), data);
}


static WStacking *group_do_navi_next(WGroup *ws, WStacking *st, 
                                     WRegionNavi nh, bool wrap)
{
    if(st==NULL)
        return group_do_navi_first(ws, nh);
    
    if(nh==REGION_NAVI_ANY || nh==REGION_NAVI_END || 
       nh==REGION_NAVI_BOTTOM || nh==REGION_NAVI_RIGHT){
        return do_get_next(ws, st, nxt, wrap, FALSE);
    }else{
        return do_get_next(ws, st, prv, wrap, FALSE);
    }
}

static WRegion *group_navi_next(WGroup *ws, WRegion *reg, 
                                WRegionNavi nh, WRegionNaviData *data)
{
    WStacking *st=group_find_stacking(ws, reg);
    
    st=group_do_navi_next(ws, st, nh, FALSE);
    
    return region_navi_cont(&ws->reg, (st!=NULL ? st->reg : NULL), data);
}


/*}}}*/


/*{{{ Stacking */


/* 
 * Note: Managed objects are considered to be stacked separately from the
 * group, slightly violating expectations.
 */

void group_stacking(WGroup *ws, Window *bottomret, Window *topret)
{
    Window win=region_xwindow((WRegion*)ws);
    
    *bottomret=win;
    *topret=win;
}


void group_restack(WGroup *ws, Window other, int mode)
{
    Window win;
    
    win=region_xwindow((WRegion*)ws);
    if(win!=None){
        xwindow_restack(win, other, mode);
        other=win;
        mode=Above;
    }
}


WStacking *group_find_stacking(WGroup *ws, WRegion *r)
{
    if(r==NULL || REGION_MANAGER(r)!=(WRegion*)ws)
        return NULL;
    
    return ioncore_find_stacking(r);
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


bool group_managed_rqorder(WGroup *grp, WRegion *reg, WRegionOrder order)
{
    WStacking **stackingp=group_get_stackingp(grp);
    WStacking *st;

    if(stackingp==NULL || *stackingp==NULL)
        return FALSE;

    st=group_find_stacking(grp, reg);
    
    if(st==NULL)
        return FALSE;
    
    stacking_restack(stackingp, st, None, NULL, NULL,
                     (order!=REGION_ORDER_FRONT));
    
    return TRUE;
}


/*}}}*/


/*{{{ Misc. */


/*EXTL_DOC
 * Iterate over managed regions of \var{ws} until \var{iterfn} returns
 * \code{false}.
 * The function itself returns \code{true} if it reaches the end of list
 * without this happening.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
bool group_managed_i(WGroup *ws, ExtlFn iterfn)
{
    WGroupIterTmp tmp;
    group_iter_init(&tmp, ws);
    
    return extl_iter_objlist_(iterfn, (ObjIterator*)group_iter, &tmp);
}


WRegion* group_current(WGroup *ws)
{
    return (ws->current_managed!=NULL ? ws->current_managed->reg : NULL);
}


void group_size_hints(WGroup *ws, WSizeHints *hints_ret)
{
    if(ws->bottom==NULL || ws->bottom->reg==NULL){
        sizehints_clear(hints_ret);
    }else{
        region_size_hints(ws->bottom->reg, hints_ret);
        hints_ret->no_constrain=TRUE;
    }
}


Window group_xwindow(const WGroup *ws)
{
    return ws->dummywin;
}


/*EXTL_DOC
 * Returns the group of \var{reg}, if it is managed by one,
 * and \var{reg} itself otherwise.
 */
/*EXTL_EXPORT_MEMBER
WRegion *region_group_of(WRegion *reg)
{
    WRegion *mgr=REGION_MANAGER(reg);
    
    return (OBJ_IS(mgr, WGroup) ? mgr : reg);
}*/


/*EXTL_DOC
 * Returns the group of \var{reg}, if \var{reg} is its bottom,
 * and \var{reg} itself otherwise.
 */
EXTL_EXPORT_MEMBER
WRegion *region_groupleader_of(WRegion *reg)
{
    WGroup *grp=REGION_MANAGER_CHK(reg, WGroup);
    
    return ((grp!=NULL && group_bottom(grp)==reg)
            ? (WRegion*)grp
            : reg);
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

        if(subtab!=extl_table_none()){
            extl_table_sets_s(subtab, "sizepolicy", 
                              sizepolicy2string(st->szplcy));
            extl_table_sets_i(subtab, "level", st->level);
        
            tmpg=REGION_GEOM(st->reg);
            tmpg.x-=REGION_GEOM(ws).x;
            tmpg.y-=REGION_GEOM(ws).y;
            
            g=extl_table_from_rectangle(&tmpg);
            extl_table_sets_t(subtab, "geom", g);
            extl_unref_table(g);
            
            if(ws->bottom==st)
                extl_table_sets_b(subtab, "bottom", TRUE);
        
            extl_table_seti_t(mgds, ++n, subtab);
            extl_unref_table(subtab);
        }
    }
    
    extl_unref_table(mgds);
    
    return tab;
}


void group_do_load(WGroup *ws, ExtlTab tab)
{
    ExtlTab substab, subtab;
    int i, n;
    
    if(extl_table_gets_t(tab, "managed", &substab)){
        n=extl_table_get_n(substab);
        for(i=1; i<=n; i++){
            if(extl_table_geti_t(substab, i, &subtab)){
                group_attach_new(ws, subtab);
                extl_unref_table(subtab);
            }
        }
        
        extl_unref_table(substab);
    }
}


WRegion *group_load(WWindow *par, const WFitParams *fp, ExtlTab tab)
{
    WGroup *ws;
    
    ws=create_group(par, fp);
    
    if(ws==NULL)
        return NULL;
        
    group_do_load(ws, tab);
    
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
    
    {(DynFun*)region_managed_prepare_focus, 
     (DynFun*)group_managed_prepare_focus},

    {region_do_set_focus, 
     group_do_set_focus},
    
    {region_managed_notify, 
     group_managed_notify},
    
    {region_managed_remove,
     group_managed_remove},
    
    {(DynFun*)region_get_configuration, 
     (DynFun*)group_get_configuration},

    {(DynFun*)region_managed_disposeroot,
     (DynFun*)group_managed_disposeroot},

    {(DynFun*)region_current,
     (DynFun*)group_current},
    
    {(DynFun*)region_rescue_clientwins,
     (DynFun*)group_rescue_clientwins},
    
    {region_restack,
     group_restack},

    {region_stacking,
     group_stacking},

    {(DynFun*)region_managed_get_pholder,
     (DynFun*)group_managed_get_pholder},

    {region_managed_rqgeom,
     group_managed_rqgeom},
     
    {region_managed_rqgeom_absolute,
     group_managed_rqgeom_absolute},
    
    {(DynFun*)group_do_add_managed,
     (DynFun*)group_do_add_managed_default},
    
    {region_size_hints,
     group_size_hints},
    
    {(DynFun*)region_xwindow,
     (DynFun*)group_xwindow},
    
    {(DynFun*)region_navi_first,
     (DynFun*)group_navi_first},
    
    {(DynFun*)region_navi_next,
     (DynFun*)group_navi_next},
    
    {(DynFun*)region_managed_rqorder,
     (DynFun*)group_managed_rqorder},
    
    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WGroup, WRegion, group_deinit, group_dynfuntab);


/*}}}*/

