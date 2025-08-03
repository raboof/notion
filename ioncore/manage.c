/*
 * ion/ioncore/manage.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#include <libtu/objp.h>
#include <libextl/extl.h>
#include "global.h"
#include "common.h"
#include "region.h"
#include "manage.h"
#include "names.h"
#include "fullscreen.h"
#include "pointer.h"
#include "netwm.h"
#include "extlconv.h"
#include "return.h"
#include "conf.h"
#include "detach.h"
#include "group-ws.h"


/*{{{ Add */


WScreen *clientwin_find_suitable_screen(WClientWin *cwin,
                                        const WManageParams *param)
{
    WScreen *scr=NULL, *found=NULL;
    bool respectpos=(param->tfor!=NULL || param->userpos);

    FOR_ALL_SCREENS(scr){
        if(!region_same_rootwin((WRegion*)scr, (WRegion*)cwin))
            continue;
        if(REGION_IS_ACTIVE(scr)){
            found=scr;
            if(!respectpos)
                break;
        }

        if(rectangle_contains(&REGION_GEOM(scr),
                    param->geom.x, param->geom.y)){
            found=scr;
            if(respectpos)
                break;
        }

        if(found==NULL)
            found=scr;
    }

    return found;
}


/*extern WRegion *ioncore_newly_created;*/


static WPHolder *try_target(WClientWin *cwin, const WManageParams *param,
                            const char *target)
{
    WRegion *r=ioncore_lookup_region(target, NULL);

    if(r==NULL)
        return NULL;

    return region_prepare_manage(r, cwin, param, MANAGE_PRIORITY_NONE);
}


static bool handle_target_winprops(WClientWin *cwin, const WManageParams *param,
                                   WScreen *scr, WPHolder **ph_ret)
{
    char *layout=NULL, *target=NULL;
    WPHolder *ph=NULL;
    bool mgd=FALSE;

    if(extl_table_gets_s(cwin->proptab, "target", &target))
        ph=try_target(cwin, param, target);

    if(ph==NULL && extl_table_gets_s(cwin->proptab, "new_group", &layout)){
        ExtlTab lo=ioncore_get_layout(layout);

        free(layout);

        if(lo!=extl_table_none()){
            WMPlexAttachParams par=MPLEXATTACHPARAMS_INIT;
            int mask=MPLEX_ATTACH_SWITCHTO;
            WRegion *reg;

            if(param->switchto)
                par.flags|=MPLEX_ATTACH_SWITCHTO;

            /*ioncore_newly_created=(WRegion*)cwin;*/

            reg=mplex_attach_new_(&scr->mplex, &par, mask, lo);

            extl_unref_table(lo);

            /*ioncore_newly_created=NULL;*/

            mgd=(region_manager((WRegion*)cwin)!=NULL);

            if(reg!=NULL && !mgd){
                if(target!=NULL)
                    ph=try_target(cwin, param, target);

                if(ph==NULL){
                    ph=region_prepare_manage(reg, cwin, param,
                                             MANAGE_PRIORITY_NONE);

                    if(ph==NULL)
                        destroy_obj((Obj*)reg);
                }
            }
        }
    }

    if(target!=NULL)
        free(target);

    *ph_ret=ph;

    return mgd;
}


static bool try_fullscreen(WClientWin *cwin, WScreen *dflt,
                           const WManageParams *param)
{
    WScreen *fs_scr=NULL;
    bool fs=FALSE, tmp;

    /* Check fullscreen mode. (This is intentionally not done
     * for transients and windows with target winprops.)
     */
    if(extl_table_gets_b(cwin->proptab, "fullscreen", &tmp)){
        if(!tmp)
            return FALSE;
        fs_scr=dflt;
    }

    if(fs_scr==NULL && netwm_check_initial_fullscreen(cwin))
        fs_scr=dflt;

    if(fs_scr==NULL)
        fs_scr=clientwin_fullscreen_chkrq(cwin, param->geom.w, param->geom.h);

    if(fs_scr!=NULL){
        WPHolder *fs_ph=region_make_return_pholder((WRegion*)cwin);

        if(fs_ph!=NULL){
            int swf=(param->switchto ? PHOLDER_ATTACH_SWITCHTO : 0);

            cwin->flags|=CLIENTWIN_FS_RQ;

            fs=pholder_attach(fs_ph, swf, (WRegion*)cwin);

            if(!fs)
                cwin->flags&=~CLIENTWIN_FS_RQ;

            destroy_obj((Obj*)fs_ph);
        }
    }

    return fs;
}


bool clientwin_do_manage_default(WClientWin *cwin,
                                 const WManageParams *param)
{
    WScreen *scr=NULL;
    WPHolder *ph=NULL;
    int swf=(param->switchto ? PHOLDER_ATTACH_SWITCHTO : 0);
    bool ok, uq=FALSE;

    /* Find a suitable screen */
    scr=clientwin_find_suitable_screen(cwin, param);
    if(scr==NULL){
        warn(TR("Unable to find a screen for a new client window."));
        return FALSE;
    }

    if(handle_target_winprops(cwin, param, scr, &ph))
        return TRUE;

    /* Check if param->tfor or any of its managers want to manage cwin. */
    if(ph==NULL && param->tfor!=NULL){
        assert(param->tfor!=cwin);
        ph=region_prepare_manage_transient((WRegion*)param->tfor, cwin,
                                           param, 0);
        uq=TRUE;
    }

    if(ph==NULL){
        /* Find a placeholder for non-fullscreen state */
        ph=region_prepare_manage((WRegion*)scr, cwin, param,
                                 MANAGE_PRIORITY_NONE);

        if(try_fullscreen(cwin, scr, param)){
            if(pholder_target(ph)!=(WRegion*)region_screen_of((WRegion*)cwin)){
                WRegion *grp=region_groupleader_of((WRegion*)cwin);
                if(region_do_set_return(grp, ph))
                    return TRUE;
            }
            destroy_obj((Obj*)ph);
            return TRUE;
        }

    }

    if(ph==NULL)
        return FALSE;

    /* Not in full-screen mode; use the placeholder to attach. */

    ok=pholder_attach(ph, swf, (WRegion*)cwin);

    destroy_obj((Obj*)ph);

    if(uq && ok)
        ioncore_unsqueeze((WRegion*)cwin, FALSE);

    return ok;
}


/*}}}*/


/*{{{ region_prepare_manage/region_manage_clientwin/etc. */


WPHolder *region_prepare_manage(WRegion *reg, const WClientWin *cwin,
                                const WManageParams *param, int priority)
{
    WPHolder *ret=NULL;
    CALL_DYN_RET(ret, WPHolder*, region_prepare_manage, reg,
                 (reg, cwin, param, priority));
    return ret;
}


WPHolder *region_prepare_manage_default(WRegion *reg, const WClientWin *cwin,
                                        const WManageParams *param, int priority)
{
    int cpriority=MANAGE_PRIORITY_SUB(priority, MANAGE_PRIORITY_NONE);
    WRegion *curr=region_current(reg);

    if(curr==NULL)
        return NULL;

    return region_prepare_manage(curr, cwin, param, cpriority);
}


WPHolder *region_prepare_manage_transient(WRegion *reg,
                                          const WClientWin *cwin,
                                          const WManageParams *param,
                                          int unused)
{
    WPHolder *ret=NULL;
    CALL_DYN_RET(ret, WPHolder*, region_prepare_manage_transient, reg,
                 (reg, cwin, param, unused));
    return ret;
}


WPHolder *region_prepare_manage_transient_default(WRegion *reg,
                                                  const WClientWin *cwin,
                                                  const WManageParams *param,
                                                  int unused)
{
    WRegion *mgr=REGION_MANAGER(reg);

    if(mgr!=NULL)
        return region_prepare_manage_transient(mgr, cwin, param, unused);
    else
        return NULL;
}


bool region_manage_clientwin(WRegion *reg, WClientWin *cwin,
                             const WManageParams *par, int priority)
{
    bool ret;
    WPHolder *ph=region_prepare_manage(reg, cwin, par, priority);
    int swf=(par->switchto ? PHOLDER_ATTACH_SWITCHTO : 0);

    if(ph==NULL)
        return FALSE;

    ret=pholder_attach(ph, swf, (WRegion*)cwin);

    destroy_obj((Obj*)ph);

    return ret;
}


/*}}}*/


/*{{{ Rescue */


DECLSTRUCT(WRescueInfo){
    WPHolder *ph;
    WRegion *get_rescue;
    /** set to TRUE when we fail to get a PHolder for this region. */
    bool failed_get;
    bool test;
};


static WRegion *iter_children(void *st)
{
    WRegion **nextp=(WRegion**)st;
    WRegion *next=*nextp;
    *nextp=(next==NULL ? NULL : next->p_next);
    return next;
}


bool region_rescue_child_clientwins(WRegion *reg, WRescueInfo *info)
{
    WRegion *next=reg->children;
    return region_rescue_some_clientwins(reg, info, iter_children, &next);
}


bool region_rescue_some_clientwins(WRegion *reg, WRescueInfo *info,
                                   WRegionIterator *iter, void *st)
{
    int fails=0;

    if(info->failed_get)
        return FALSE;

    reg->flags|=REGION_CWINS_BEING_RESCUED;

    while(TRUE){
        WRegion *tosave=iter(st);
        WClientWin *cwin;

        if(tosave==NULL)
            break;

        cwin=OBJ_CAST(tosave, WClientWin);

        if(cwin==NULL){
            if(!region_rescue_clientwins(tosave, info)){
                fails++;
                if(info->failed_get)
                    break;
            }
        }else if(info->test){
            fails++;
            break;
        }else if(!(cwin->flags&CLIENTWIN_UNMAP_RQ)){
            if(info->ph==NULL){
                info->ph=region_get_rescue_pholder(info->get_rescue);
                if(info->ph==NULL){
                    info->failed_get=TRUE;
                    break;
                }
            }

            WRegion *movereg = (WRegion*)cwin;

            /* If the client window is managed by a GroupCW, move the GroupCW instead. */
            WGroupCW *cwg = REGION_MANAGER_CHK(movereg, WGroupCW);
            if(cwg!=NULL){
                movereg = (WRegion*)cwg;
            }

            if(!pholder_attach(info->ph, 0, movereg))
                fails++;
        }
    }

    reg->flags&=~REGION_CWINS_BEING_RESCUED;

    return (fails==0 && !info->failed_get);
}


bool region_rescue_clientwins(WRegion *reg, WRescueInfo *info)
{
    bool ret=FALSE;
    CALL_DYN_RET(ret, bool, region_rescue_clientwins, reg, (reg, info));
    return ret;
}


bool region_rescue(WRegion *reg, WPHolder *ph_param)
{
    WRescueInfo info;
    bool ret;

    info.ph=ph_param;
    info.test=FALSE;
    info.get_rescue=reg;
    info.failed_get=FALSE;

    ret=region_rescue_clientwins(reg, &info);

    if(info.ph!=ph_param)
        destroy_obj((Obj*)info.ph);

    return ret;
}


bool region_rescue_needed(WRegion *reg)
{
    WRescueInfo info;

    info.ph=NULL;
    info.test=TRUE;
    info.get_rescue=reg;
    info.failed_get=FALSE;

    return !region_rescue_clientwins(reg, &info);
}


/*}}}*/


/*{{{ Misc. */


ExtlTab manageparams_to_table(const WManageParams *mp)
{
    ExtlTab t=extl_create_table();
    extl_table_sets_b(t, "switchto", mp->switchto);
    extl_table_sets_b(t, "jumpto", mp->jumpto);
    extl_table_sets_b(t, "userpos", mp->userpos);
    extl_table_sets_b(t, "dockapp", mp->dockapp);
    extl_table_sets_b(t, "maprq", mp->maprq);
    extl_table_sets_i(t, "gravity", mp->gravity);
    extl_table_sets_rectangle(t, "geom", &(mp->geom));
    extl_table_sets_o(t, "tfor", (Obj*)(mp->tfor));

    return t;
}


/*}}}*/
