#include <stdlib.h>

#include <libtu/misc.h>
#include <libtu/parser.h>
#include <libtu/tokenizer.h>

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/clientwin.h>
#include <ioncore/property.h>
#include <ioncore/readconfig.h>
#include <ioncore/manage.h>
#include <ioncore/ioncore.h>
#include "sm_matchwin.h"
#include "sm_session.h"


/*{{{ Module information */


#include "../version.h"

char mod_sm_ion_api_version[]=ION_API_VERSION;


/*}}}*/


/*{{{ Manage callback */


static bool sm_do_manage(WClientWin *cwin, const WManageParams *param)
{
    Window t_hint;
    int transient_mode=TRANSIENT_MODE_OFF;
    WRegion *reg;
    
    if(param->tfor!=NULL)
        return FALSE;
    
    reg=mod_sm_match_cwin_to_saved(cwin);
    if(reg==NULL)
        return FALSE;
    
    
    return region_manage_clientwin(reg, cwin, param, 
                                   MANAGE_REDIR_PREFER_NO);
}


/*}}}*/


/*{{{ Init/deinit */


extern bool mod_sm_register_exports();
extern void mod_sm_unregister_exports();


void mod_sm_deinit()
{
    REMOVE_HOOK(clientwin_do_manage_alt, sm_do_manage);
    /*REMOVE_HOOK(ioncore_save_session_hook, save_id);*/

    ioncore_unset_sm_callbacks(mod_sm_add_match, mod_sm_get_configuration);
    
    mod_sm_unregister_exports();
    
    mod_sm_close();
}


int mod_sm_init()
{
    if(ioncore_g.sm_client_id!=NULL)
        mod_sm_set_ion_id(ioncore_g.sm_client_id);
    
    if(!mod_sm_init_session())
        goto err;

    if(!mod_sm_register_exports())
        goto err;

    if(!ioncore_set_sm_callbacks(mod_sm_add_match, mod_sm_get_configuration))
        goto err;
    
    ADD_HOOK(clientwin_do_manage_alt, sm_do_manage);
    /*ADD_HOOK(ioncore_save_session_hook, save_id);*/

    return TRUE;
    
err:
    mod_sm_deinit();
    return FALSE;
}


/*}}}*/

