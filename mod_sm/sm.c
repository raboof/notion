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


/*{{{ Exports */


/*EXTL_DOC
 * Sets the restart hint property to restart immediately and exits.
 */
EXTL_EXPORT
void mod_sm_restart()
{
    sm_resign(2);
}


/*EXTL_DOC
 * Just exits.
 */
EXTL_EXPORT
void mod_sm_resign()
{
    sm_resign(0);
}


/*EXTL_DOC
 * Restart hint to restart anyway and exit Ion will be restarted next
 * session launch even if it wasn't running at shutdown.
 */
EXTL_EXPORT
void mod_sm_resign_tmp()
{
    sm_resign(1);
}


/*}}}*/


/*{{{ Manage callback */


static bool sm_do_manage(WClientWin *cwin, const WManageParams *param)
{
    Window t_hint;
    int transient_mode=TRANSIENT_MODE_OFF;
    WRegion *reg;
    
    if(param->tfor!=NULL)
        return FALSE;
    
    reg=sm_match_cwin_to_saved(cwin);
    if(reg==NULL)
        return FALSE;
    
    
    return region_manage_clientwin(reg, cwin, param, 
                                   MANAGE_REDIR_PREFER_NO);
}


/*}}}*/


/*{{{ Init/deinit */


extern bool mod_sm_register_exports();
extern void mod_sm_unregister_exports();


static void save_id()
{
    ExtlTab t=extl_create_table();
    extl_table_sets_s(t, "client_id", sm_get_ion_id());
    if(!ioncore_write_savefile("mod_sm_client_id", t))
        warn("Unable to save sm client id.");
    extl_unref_table(t);
}


static void load_id()
{
    ExtlTab t;
    char *id;
    
    if(!ioncore_read_savefile("mod_sm_client_id", &t))
        return;
    
    if(extl_table_gets_s(t, "client_id", &id)){
        sm_set_ion_id(id);
        free(id);
    }
    
    extl_unref_table(t);
}


void mod_sm_deinit()
{
    REMOVE_HOOK(clientwin_do_manage_alt, sm_do_manage);
    REMOVE_HOOK(ioncore_save_session_hook, save_id);

    ioncore_unset_sm_callbacks(mod_sm_add_match, mod_sm_get_configuration);
    
    mod_sm_unregister_exports();
    
    sm_close();
}


int mod_sm_init()
{
    load_id();
    
    if(!sm_init_session())
        goto err;

    if(!mod_sm_register_exports())
        goto err;

    if(!ioncore_set_sm_callbacks(mod_sm_add_match, mod_sm_get_configuration))
        goto err;
    
    ADD_HOOK(clientwin_do_manage_alt, sm_do_manage);
    ADD_HOOK(ioncore_save_session_hook, save_id);

    return TRUE;
    
err:
    mod_sm_deinit();
    return FALSE;
}


/*}}}*/

