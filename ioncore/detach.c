/*
 * ion/ioncore/detach.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#include <libtu/objp.h>
#include <libtu/setparam.h>
#include <libtu/minmax.h>

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/mplex.h>
#include <ioncore/focus.h>
#include <ioncore/group.h>
#include <ioncore/group-ws.h>
#include <ioncore/framedpholder.h>
#include <ioncore/return.h>
#include <ioncore/sizehint.h>
#include <ioncore/resize.h>


static void get_relative_geom(WRectangle *g, WRegion *reg, WRegion *mgr)
{
    WWindow *rel=REGION_PARENT(mgr), *w;

    *g=REGION_GEOM(reg);

    for(w=REGION_PARENT(reg);
        w!=rel && (WRegion*)w!=mgr;
        w=REGION_PARENT(w)){

        g->x+=REGION_GEOM(w).x;
        g->y+=REGION_GEOM(w).y;
    }
}


static bool ioncore_do_detach(WRegion *reg, WGroup *grp, WFrameMode framemode,
                              uint framelevel)
{
    WGroupAttachParams ap=GROUPATTACHPARAMS_INIT;
    WRegionAttachData data;
    WPHolder *ph;
    bool newph=FALSE;
    bool ret;

    ap.switchto_set=TRUE;
    ap.switchto=region_may_control_focus(reg);

    data.type=REGION_ATTACH_REPARENT;
    data.u.reg=reg;

    ph=region_unset_get_return(reg);

    if(ph==NULL){
        ph=region_make_return_pholder(reg);
        newph=TRUE;
    }

    if(framemode!=FRAME_MODE_UNKNOWN){
        WFramedParam fpa=FRAMEDPARAM_INIT;

        fpa.mode=framemode;
        fpa.inner_geom_gravity_set=TRUE;
        fpa.gravity=ForgetGravity;

        ap.geom_weak_set=TRUE;
        ap.geom_weak=0;

        ap.level_set=TRUE;
        ap.level=framelevel+1;

        get_relative_geom(&fpa.inner_geom, reg, (WRegion*)grp);

        ret=(region_attach_framed((WRegion*)grp, &fpa,
                                  (WRegionAttachFn*)group_do_attach,
                                  &ap, &data)!=NULL);
    }else{
        WStacking *st=ioncore_find_stacking(reg);

        if(st!=NULL){
            ap.szplcy=st->szplcy;
            ap.szplcy_set=TRUE;

            ap.level_set=TRUE;
            ap.level=MAXOF(st->level, STACKING_LEVEL_NORMAL);
        }

        ap.geom_set=TRUE;
        get_relative_geom(&ap.geom, reg, (WRegion*)grp);

        ret=(group_do_attach(grp, &ap, &data)!=NULL);
    }

    if(!ret && newph)
        destroy_obj((Obj*)ph);
    else if(!region_do_set_return(reg, ph))
        destroy_obj((Obj*)ph);

    return ret;
}


static WRegion *check_mplex(WRegion *reg, WFrameMode *mode, uint *level)
{
    WMPlex *mplex=REGION_MANAGER_CHK(reg, WMPlex);

    if(OBJ_IS(reg, WWindow) || mplex==NULL){
        *mode=FRAME_MODE_UNKNOWN;
        return reg;
    }

    *mode=FRAME_MODE_FLOATING;

    if(OBJ_IS(mplex, WFrame)
       && frame_mode((WFrame*)mplex)==FRAME_MODE_TRANSIENT){
        WStacking *st=ioncore_find_stacking((WRegion*)mplex);
        if(st!=NULL)
            *level=st->level;
        *mode=FRAME_MODE_TRANSIENT;

    }

    return (WRegion*)mplex;
}


static WGroup *find_group(WRegion *reg)
{
    WRegion *mgr=REGION_MANAGER(reg);

    while(mgr!=NULL){
        mgr=REGION_MANAGER(mgr);
        if(OBJ_IS(mgr, WGroup))
            break;
    }

    return (WGroup*)mgr;
}


bool ioncore_detach(WRegion *reg, int sp)
{
    WFrameMode mode;
    WGroup *grp;
    bool set, nset;
    uint level=STACKING_LEVEL_NORMAL;

    reg=region_groupleader_of(reg);

    grp=find_group(check_mplex(reg, &mode, &level));

    /* reg is only considered detached if there's no higher-level group
     * to attach to, thus causing 'toggle' to cycle.
     */
    set=(grp==NULL);
    nset=libtu_do_setparam(sp, set);

    if(!XOR(nset, set))
        return set;

    if(!set){
        return ioncore_do_detach(reg, grp, mode, level);
    }else{
        WPHolder *ph=region_get_return(reg);

        if(ph!=NULL){
            if(!pholder_attach_mcfgoto(ph, PHOLDER_ATTACH_SWITCHTO, reg)){
                warn(TR("Failed to reattach."));
                return TRUE;
            }
            region_unset_return(reg);
        }

        return FALSE;
    }
}


/*EXTL_DOC
 * Detach or reattach \var{reg}, depending on whether \var{how}
 * is 'set'/'unset'/'toggle'. (Detaching means making \var{reg}
 * managed by its nearest ancestor \type{WGroup}, framed if \var{reg} is
 * not itself \type{WFrame}. Reattaching means making it managed where
 * it used to be managed, if a return-placeholder exists.)
 * If \var{reg} is the 'bottom' of some group, the whole group is
 * detached. If \var{reg} is a \type{WWindow}, it is put into a
 * frame.
 */
EXTL_EXPORT_AS(ioncore, detach)
bool ioncore_detach_extl(WRegion *reg, const char *how)
{
    if(how==NULL)
        how="set";

    return ioncore_detach(reg, libtu_string_to_setparam(how));
}


void do_unsqueeze(WRegion *reg)
{
    WSizeHints h;
    WRegion *mgr=REGION_MANAGER(reg);

    if(OBJ_IS(reg, WScreen))
        return;

    region_size_hints(reg, &h);

    if(!h.min_set)
        return;

    if((h.base_set ? h.base_width : 0)+h.min_width<=REGION_GEOM(reg).w &&
       (h.base_set ? h.base_height : 0)+h.min_height<=REGION_GEOM(reg).h){
        return;
    }

    ioncore_detach(reg, SETPARAM_SET);

    if(REGION_MANAGER(reg)==mgr)
        return;

    do_unsqueeze(reg);
}


/*EXTL_DOC
 * Try to detach \var{reg} if it fits poorly in its
 * current location. This function does not do anything,
 * unless \var{override} is set or the \var{unsqueeze} option
 * of \fnref{ioncore.set} is set.
 */
EXTL_EXPORT
void ioncore_unsqueeze(WRegion *reg, bool override)
{
    if(ioncore_g.unsqueeze_enabled || override)
        do_unsqueeze(region_groupleader_of(reg));
}


