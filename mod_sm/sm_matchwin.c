#include <string.h>
#include <unistd.h>
#include <time.h>
#include <X11/Xatom.h>

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/exec.h>
#include <ioncore/signal.h>
#include <ioncore/names.h>
#include <ioncore/property.h>

#include "sm_matchwin.h"

#define TIME_OUT 60000

static WWinMatch *match_list=NULL;

static void sm_purge_matches(WTimer *timer);
static WTimer purge_timer=TIMER_INIT(sm_purge_matches);


char *sm_get_window_role(Window window)
{
    Atom atom;
    XTextProperty tp;
    
    atom=XInternAtom(ioncore_g.dpy, "WM_WINDOW_ROLE", False);
    
    if(XGetTextProperty(ioncore_g.dpy, window, &tp, atom))
    {
        if(tp.encoding == XA_STRING && tp.format == 8 && tp.nitems != 0)
            return((char *)tp.value);
    }
    
    return NULL;
}

Window sm_get_client_leader(Window window)
{
    Window client_leader = 0;
    Atom atom;
    Atom actual_type;
    int actual_format;
    unsigned long nitems;
    unsigned long bytes_after;
    unsigned char *prop = NULL;
    
    atom=XInternAtom(ioncore_g.dpy, "WM_CLIENT_LEADER", False);
    
    if(XGetWindowProperty(ioncore_g.dpy, window, atom,
			  0L, 1L, False, AnyPropertyType, &actual_type,
			  &actual_format, &nitems, &bytes_after,
			  &prop) == Success)
    {
        if(actual_type == XA_WINDOW && actual_format == 32
           && nitems == 1	&& bytes_after == 0)
            client_leader=*((Window *)prop);
        XFree(prop);
    }
    return client_leader;
}

char *sm_get_client_id(Window window)
{
    char *client_id = NULL;
    Window client_leader;
    XTextProperty tp;
    Atom atom;
    
    if((client_leader=sm_get_client_leader(window))!=0){
        atom=XInternAtom(ioncore_g.dpy, "SM_CLIENT_ID", False);  	
        if (XGetTextProperty (ioncore_g.dpy, client_leader, &tp, atom))
            if (tp.encoding == XA_STRING && tp.format == 8 && tp.nitems != 0)
                client_id = (char *) tp.value;
    }
    
    return client_id;
}

char *sm_get_window_cmd(Window window)
{
    char **cmd_argv, *command=NULL;
    int id, i, len=0, cmd_argc=0;
    
    if(XGetCommand(ioncore_g.dpy, window, &cmd_argv, &cmd_argc) && (cmd_argc > 0))
        ;
    else if((id=sm_get_client_leader(window)))
        XGetCommand(ioncore_g.dpy, id, &cmd_argv, &cmd_argc);
    
    if(cmd_argc > 0){
        for(i=0; i < cmd_argc; i++)
            len+=strlen(cmd_argv[i])+1;
        command=ALLOC_N(char, len+1);
        strcpy(command, cmd_argv[0]);
        for(i=1; i < cmd_argc; i++){
            strcat(command, " ");
            strcat(command, cmd_argv[i]);
        }
        XFreeStringList(cmd_argv);
    }
    
    return command;
}

static void free_win_match(WWinMatch *match)
{	
    UNLINK_ITEM(match_list, match, next, prev);
    
    watch_reset(&(match->target_watch));
    
    if(match->client_id)
        free(match->client_id);
    if(match->window_role)
        free(match->window_role);
    if(match->wclass)
        free(match->wclass);
    if(match->wm_name)
        free(match->wm_name);
    if(match->wm_cmd)
        free(match->wm_cmd);		
    free(match);
}

static void sm_purge_matches(WTimer *timer)
{
#ifdef DEBUG
    warn("purging remaining matches\n");
#endif
    while(match_list)
        free_win_match(match_list);
}

void sm_start_purge_timer()
{
    timer_set(&purge_timer, TIME_OUT);
}

void sm_register_win_match(WWinMatch *match)
{
    LINK_ITEM(match_list, match, next, prev);
}

bool sm_have_match_list()
{
    if(match_list!=NULL)
        return TRUE;
    else
        return FALSE;
}

#define xstreq(a,b) (a && b && (strcmp(a,b)==0))

/* Tries to match a window against a list of loaded matches */

static WWinMatch *match_cwin(WClientWin *cwin)
{
    WWinMatch *match=match_list;
    int win_match;
    XClassHint clss;
    char *client_id=sm_get_client_id(cwin->win);
    char *window_role=sm_get_window_role(cwin->win);
    char *wm_cmd=sm_get_window_cmd(cwin->win);
    char **wm_name=NULL;
    int n;
    
    wm_name=xwindow_get_text_property(cwin->win, XA_WM_NAME, &n);
    if(n<=0)
        assert(wm_name==NULL);
    
    XGetClassHint(ioncore_g.dpy, cwin->win, &clss);
    
    for(; match!=NULL; match=match->next){
        
        win_match=0;
        
        if(client_id || match->client_id){
            if(xstreq(match->client_id, client_id)){
                win_match+=2;
                if(xstreq(match->window_role, window_role))
                    win_match++;
            }
        }
        if(win_match<3){
            if(xstreq(match->wclass, clss.res_class) && xstreq(match->winstance, clss.res_name)){
                win_match++;
                if(win_match<3){
                    if(xstreq(match->wm_cmd, wm_cmd))
                        win_match++;
                    if(*wm_name!=NULL && xstreq(match->wm_name, *wm_name))
                        win_match++;
                }
            }
        }
        if(win_match>2)
            break;		
    }
    XFree(client_id);
    XFree(window_role);
    XFreeStringList(wm_name);
    free(wm_cmd);
    return match;
}

/* Returns frame_id of a match. Called from add_clientwin_alt in sm.c */

WRegion *sm_match_cwin_to_saved(WClientWin *cwin)
{
    WWinMatch *match=NULL;
    WRegion *reg=NULL;

    if((match=match_cwin(cwin))){
        reg=(WRegion*)(match->target_watch.obj);
        free_win_match(match);
    }
    return reg;
}


bool mod_sm_add_match(WWindow *par, ExtlTab tab)
{
    WWinMatch *m=NULL;
    
    m=ALLOC(WWinMatch);
    if(m==NULL)
        return FALSE;
    watch_init(&(m->target_watch));
    m->client_id=NULL;
    m->window_role=NULL;
    m->winstance=NULL;
    m->wclass=NULL;
    m->wm_name=NULL;
    m->wm_cmd=NULL;
    
    extl_table_gets_s(tab, "mod_sm_client_id", &(m->client_id));
    extl_table_gets_s(tab, "mod_sm_window_role", &(m->window_role));
    extl_table_gets_s(tab, "mod_sm_wclass", &(m->wclass));
    extl_table_gets_s(tab, "mod_sm_winstance", &(m->winstance));
    extl_table_gets_s(tab, "mod_sm_wm_name", &(m->wm_name));
    extl_table_gets_s(tab, "mod_sm_wm_cmd", &(m->wm_cmd));
    
    watch_setup(&(m->target_watch), (Obj*)par, NULL);
    
    sm_register_win_match(m);

    return TRUE;
}


void mod_sm_get_configuration(WClientWin *cwin, ExtlTab tab)
{
    XClassHint clss;
    char *client_id=NULL, *window_role=NULL, *wm_cmd=NULL, **wm_name;
    int n=0;
    
    if((client_id=sm_get_client_id(cwin->win))){
        extl_table_sets_s(tab, "mod_sm_client_id", client_id);
        XFree(client_id);
    }
     
    
    if((window_role=sm_get_window_role(cwin->win))){
        extl_table_sets_s(tab, "mod_sm_window_role", window_role);
        XFree(window_role);
    }
    
    if(XGetClassHint(ioncore_g.dpy, cwin->win, &clss) != 0){
        extl_table_sets_s(tab, "mod_sm_wclass", clss.res_class);
        extl_table_sets_s(tab, "mod_sm_winstance", clss.res_name);
    }
    
    wm_name=xwindow_get_text_property(cwin->win, XA_WM_NAME, &n);

    if(n>0 && wm_name!=NULL){
        extl_table_sets_s(tab, "mod_sm_wm_name", *wm_name);
        XFreeStringList(wm_name);
        
    }
    
    if((wm_cmd=sm_get_window_cmd(cwin->win))){
        extl_table_sets_s(tab, "mod_sm_wm_cmd", wm_cmd);
        free(wm_cmd);
    }
}
