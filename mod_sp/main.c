/*
 * ion/mod_sp/main.c
 *
 * Copyright (c) Tuomo Valkonen 2004-2008. 
 *
 * See the included file LICENSE for details.
 */

#include <string.h>

#include <libtu/map.h>
#include <libtu/minmax.h>
#include <libextl/readconfig.h>
#include <libmainloop/hooks.h>

#include <ioncore/saveload.h>
#include <ioncore/screen.h>
#include <ioncore/mplex.h>
#include <ioncore/stacking.h>
#include <ioncore/ioncore.h>
#include <ioncore/global.h>
#include <ioncore/framep.h>
#include <ioncore/frame.h>
#include <ioncore/names.h>
#include <ioncore/group.h>
#include <ioncore/group-ws.h>

#include "main.h"
#include "exports.h"


/*{{{ Module information */

#include "../version.h"

char mod_sp_ion_api_version[]=ION_API_VERSION;


/*}}}*/


/*{{{ Bindmaps, config, etc. */


#define SP_NAME  "*scratchpad*"
#define SPWS_NAME  "*scratchws*"


/*}}}*/


/*{{{ Exports */


static WRegion *create_frame_scratchpad(WWindow *parent, const WFitParams *fp,
                                        void *unused)
{
    return (WRegion*)create_frame(parent, fp, FRAME_MODE_UNKNOWN);
}


static WRegion *create_scratchws(WWindow *parent, const WFitParams *fp, 
                                 void *unused)
{
    WRegion *reg;
    WRegionAttachData data;
    WGroupAttachParams par=GROUPATTACHPARAMS_INIT;
    WGroupWS *ws;
    
    ws=create_groupws(parent, fp);
    
    if(ws==NULL)
        return NULL;
        
    region_set_name((WRegion*)ws, SPWS_NAME);
    
    data.type=REGION_ATTACH_NEW;
    data.u.n.fn=create_frame_scratchpad;
    data.u.n.param=NULL;
    
    par.szplcy_set=TRUE;
    par.szplcy=SIZEPOLICY_FREE_GLUE;
    
    par.geom_set=TRUE;
    par.geom.w=minof(fp->g.w, CF_SCRATCHPAD_DEFAULT_W);
    par.geom.h=minof(fp->g.h, CF_SCRATCHPAD_DEFAULT_H);
    par.geom.x=(fp->g.w-par.geom.w)/2;
    par.geom.y=(fp->g.h-par.geom.h)/2;
    
    par.level_set=TRUE;
    par.level=STACKING_LEVEL_MODAL1+1;
    
    par.bottom=TRUE;
    
    reg=group_do_attach(&ws->grp, &par, &data);

    if(reg==NULL){
        destroy_obj((Obj*)ws);
        return NULL;
    }
     
    region_set_name((WRegion*)reg, SP_NAME);
    
    return (WRegion*)ws;
}


static WRegion *create(WMPlex *mplex, int flags)
{
    WRegion *sp;
    WMPlexAttachParams par=MPLEXATTACHPARAMS_INIT;
    
    par.flags=(flags
               |MPLEX_ATTACH_UNNUMBERED
               |MPLEX_ATTACH_SIZEPOLICY
               |MPLEX_ATTACH_PSEUDOMODAL);
    par.szplcy=SIZEPOLICY_FULL_EXACT;

    sp=mplex_do_attach_new((WMPlex*)mplex, &par,
                           create_scratchws,
                           NULL);
    
    if(sp==NULL)
        warn(TR("Unable to create scratchpad."));
    
    return sp;
}


static bool is_scratchpad(WRegion *reg)
{
    char *nm=reg->ni.name;
    int inst_off=reg->ni.inst_off;
    
    if(nm==NULL)
        return FALSE;
    
    return (inst_off<0
            ? (strcmp(nm, SP_NAME)==0 || 
               strcmp(nm, SPWS_NAME)==0)
            : (strncmp(nm, SP_NAME, inst_off)==0 ||
               strncmp(nm, SPWS_NAME, inst_off)==0));
}


/*EXTL_DOC
 * Change displayed status of some scratchpad on \var{mplex} if one is 
 * found. The parameter \var{how} is one of 
 * \codestr{set}, \codestr{unset}, or \codestr{toggle}.
 * The resulting status is returned.
 */
EXTL_EXPORT
bool mod_sp_set_shown_on(WMPlex *mplex, const char *how)
{
    int setpar=libtu_setparam_invert(libtu_string_to_setparam(how));
    WMPlexIterTmp tmp;
    WRegion *reg;
    bool found=FALSE, res=FALSE;
    
    FOR_ALL_MANAGED_BY_MPLEX(mplex, reg, tmp){
        if(is_scratchpad(reg)){
            res=!mplex_set_hidden(mplex, reg, setpar);
            found=TRUE;
        }
    }

    if(!found){
        int sp=libtu_string_to_setparam(how);
        if(sp==SETPARAM_SET || sp==SETPARAM_TOGGLE)
            found=(create(mplex, 0)!=NULL);
            res=found;
    }
    
    return res;
}


/*EXTL_DOC
 * Toggle displayed status of \var{sp}.
 * The parameter \var{how} is one of 
 * \codestr{set}, \codestr{unset}, or \codestr{toggle}.
 * The resulting status is returned.
 */
EXTL_EXPORT
bool mod_sp_set_shown(WFrame *sp, const char *how)
{
    if(sp!=NULL){
        int setpar=libtu_setparam_invert(libtu_string_to_setparam(how));
        WMPlex *mplex=OBJ_CAST(REGION_MANAGER(sp), WMPlex);
        if(mplex!=NULL)
            return mplex_set_hidden(mplex, (WRegion*)sp, setpar);
    }
    
    return FALSE;
}


/*}}}*/


/*{{{ Init & deinit */


void mod_sp_deinit()
{
    mod_sp_unregister_exports();
}


static void check_and_create()
{
    WMPlexIterTmp tmp;
    WScreen *scr;
    WRegion *reg;
    
    /* No longer needed, free the memory the list uses. */
    hook_remove(ioncore_post_layout_setup_hook, check_and_create);
    
    FOR_ALL_SCREENS(scr){
        FOR_ALL_MANAGED_BY_MPLEX((WMPlex*)scr, reg, tmp){
            if(is_scratchpad(reg))
                return;
        }
        
        create(&scr->mplex, MPLEX_ATTACH_HIDDEN);
    }
}
    

bool mod_sp_init()
{
    if(!mod_sp_register_exports())
        return FALSE;

    extl_read_config("cfg_sp", NULL, FALSE);
    
    if(ioncore_g.opmode==IONCORE_OPMODE_INIT){
        hook_add(ioncore_post_layout_setup_hook, check_and_create);
    }else{
        check_and_create();
    }
    
    return TRUE;
}


/*}}}*/

