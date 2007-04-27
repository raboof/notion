/*
 * ion/mod_sm/sm.c
 *
 * Copyright (c) Tuomo Valkonen 2004-2007. 
 *
 * See the included file LICENSE for details.
 */

#include <stdlib.h>
#include <string.h>

#include <libtu/misc.h>
#include <libtu/parser.h>
#include <libtu/tokenizer.h>
#include <libtu/util.h>

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/clientwin.h>
#include <ioncore/saveload.h>
#include <ioncore/property.h>
#include <libextl/readconfig.h>
#include <ioncore/manage.h>
#include <ioncore/ioncore.h>
#include <ioncore/exec.h>
#include "sm_matchwin.h"
#include "sm_session.h"
#include "exports.h"


/*{{{ Module information */


#include "../version.h"

char mod_sm_ion_api_version[]=ION_API_VERSION;


/*}}}*/


/*{{{ Manage callback */


static bool sm_do_manage(WClientWin *cwin, const WManageParams *param)
{
    int transient_mode=TRANSIENT_MODE_OFF;
    WPHolder *ph;
    bool ret;
    
    if(param->tfor!=NULL)
        return FALSE;
    
    ph=mod_sm_match_cwin_to_saved(cwin);
    if(ph==NULL)
        return FALSE;
    
    ret=pholder_attach(ph, 0, (WRegion*)cwin);
    
    destroy_obj((Obj*)ph);
    
    return ret;
}


/*}}}*/


/*{{{ Init/deinit */


static void mod_sm_set_sessiondir()
{
    const char *smdir=NULL, *id=NULL;
    char *tmp;
    bool ok=FALSE;
    
    smdir=getenv("SM_SAVE_DIR");
    id=getenv("GNOME_DESKTOP_SESSION_ID");

    /* Running under SM, try to use a directory specific
     * to the session.
     */
    if(smdir!=NULL){
        tmp=scat3(smdir, "/", libtu_progbasename());
    }else if(id!=NULL){
        tmp=scat("gnome-session-", id);
        if(tmp!=NULL){
            char *p=tmp;
            while(1){
                p=strpbrk(p, "/ :?*");
                if(p==NULL)
                    break;
                *p='-';
                p++;
            }
        }
    }else{
        tmp=scopy("default-session-sm");
    }
        
    if(tmp!=NULL){
        ok=extl_set_sessiondir(tmp);
        free(tmp);
    }
    
    if(!ok)
        warn(TR("Failed to set session directory."));
}



void mod_sm_deinit()
{
    ioncore_set_smhook(NULL);

    hook_remove(clientwin_do_manage_alt, (WHookDummy*)sm_do_manage);

    ioncore_set_sm_callbacks(NULL, NULL);
    
    mod_sm_unregister_exports();
    
    mod_sm_close();
}


int mod_sm_init()
{
    if(ioncore_g.sm_client_id!=NULL)
        mod_sm_set_ion_id(ioncore_g.sm_client_id);
    
    if(!mod_sm_init_session())
        goto err;

    if(extl_sessiondir()==NULL)
        mod_sm_set_sessiondir();
    
    if(!mod_sm_register_exports())
        goto err;

    ioncore_set_sm_callbacks(mod_sm_add_match, mod_sm_get_configuration);
    
    hook_add(clientwin_do_manage_alt, (WHookDummy*)sm_do_manage);

    ioncore_set_smhook(mod_sm_smhook);
    
    return TRUE;
    
err:
    mod_sm_deinit();
    return FALSE;
}


/*}}}*/

