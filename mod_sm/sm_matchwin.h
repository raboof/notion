#ifndef ION_MOD_SM_WINMATCH_H
#define ION_MOD_SM_WINMATCH_H

#include <ioncore/common.h>
#include <libtu/obj.h>

INTRSTRUCT(WWinMatch);

DECLSTRUCT(WWinMatch){
    Watch target_watch;
    char *client_id;
    char *window_role;
    char *wclass;
    char *winstance;
    char *wm_name;
    char *wm_cmd;
    
    WWinMatch *next, *prev;
};

extern WRegion *mod_sm_match_cwin_to_saved(WClientWin *cwin);
extern void mod_sm_register_win_match(WWinMatch *match);
extern char *mod_sm_get_window_cmd(Window window);
extern char *mod_sm_get_client_id(Window window);
extern char *mod_sm_get_window_role(Window window);
extern bool mod_sm_have_match_list();
extern void mod_sm_start_purge_timer();

extern bool mod_sm_add_match(WWindow *par, ExtlTab tab);
extern void mod_sm_get_configuration(WClientWin *cwin, ExtlTab tab);

#endif /* ION_MOD_SM_WINMATCH_H */
