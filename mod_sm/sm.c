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


static void set_sdir()
{
    const char *smdir=NULL, *id=NULL;
    char *tmp;
    
    smdir=getenv("SM_SAVE_DIR");
    id=getenv("GNOME_DESKTOP_SESSION_ID");

    /* Running under SM, try to use a directory specific
     * to the session.
     */
    if(smdir!=NULL){
        tmp=scat(smdir, "/ion3"); /* TODO: pwm<=>ion! */
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
        
    if(tmp==NULL){
        warn_err();
    }else{
        ioncore_set_sessiondir(tmp);
        free(tmp);
    }
}



void mod_sm_deinit()
{
    hook_remove(clientwin_do_manage_alt, (WHookDummy*)sm_do_manage);

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

    if(ioncore_sessiondir()==NULL)
        set_sdir();
    
    if(!mod_sm_register_exports())
        goto err;

    if(!ioncore_set_sm_callbacks(mod_sm_add_match, mod_sm_get_configuration))
        goto err;
    
    hook_add(clientwin_do_manage_alt, (WHookDummy*)sm_do_manage);

    return TRUE;
    
err:
    mod_sm_deinit();
    return FALSE;
}


/*}}}*/

