/*
 * ion/ioncore/exec.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <libmainloop/select.h>
#include <libmainloop/exec.h>

#include "common.h"
#include "exec.h"
#include "property.h"
#include "global.h"
#include "ioncore.h"
#include "saveload.h"


/*{{{ Exec */


static void ioncore_setup_display(WRootWin *rw)
{
    char *tmp, *ptr;
    char *display;

    /* Set up $DISPLAY */
    
    display=XDisplayName(ioncore_g.display);
    
    /* %ui, UINT_MAX is used to ensure there is enough space for the screen
     * number
     */
    libtu_asprintf(&tmp, "DISPLAY=%s.0123456789a", display);

    if(tmp==NULL)
        return;

    ptr=strchr(tmp, ':');
    if(ptr!=NULL){
        ptr=strchr(ptr, '.');
        if(ptr!=NULL)
            *ptr='\0';
    }

    if(rw!=NULL)
        snprintf(tmp+strlen(tmp), 11, ".%u", (unsigned)rw->xscr);
    
    putenv(tmp);
        
    /* No need to free it, we'll execve soon */
    /*free(tmp);*/

    /*XFree(display);*/
}


void ioncore_setup_environ(const WExecP *p)
{
    /* Set up $DISPLAY */
    
    ioncore_setup_display(p->target!=NULL 
                          ? region_rootwin_of(p->target)
                          : NULL);
    
    /* Set up working directory */
    
    if(p->wd!=NULL){
        if(chdir(p->wd)!=0)
            warn_err_obj(p->wd);
    }
}


WHook *ioncore_exec_environ_hook=NULL;


static void setup_exec(void *execp)
{
    hook_call_p(ioncore_exec_environ_hook, execp, NULL);
    
#ifndef CF_NO_SETPGID
    setpgid(0, 0);
#endif
    
    ioncore_g.dpy=NULL;
}


EXTL_EXPORT
int ioncore_do_exec_on(WRegion *reg, const char *cmd, const char *wd,
                       ExtlFn errh)
{
    WExecP p;
    
    p.target=reg;
    p.cmd=cmd;
    p.wd=wd;
    
    return mainloop_popen_bgread(cmd, setup_exec, (void*)&p, 
                                 extl_fn_none(), errh);
}


/*EXTL_DOC
 * Run \var{cmd} with the environment variable DISPLAY set to point to the
 * X display the WM is running on. No specific screen is set unlike with
 * \fnref{WRootWin.exec_on}. The PID of the (shell executing the) new 
 * process is returned.
 */
EXTL_SAFE
EXTL_EXPORT
int ioncore_exec(const char *cmd)
{
    return ioncore_do_exec_on(NULL, cmd, NULL, extl_fn_none());
}


/*EXTL_DOC
 * Run \var{cmd} with a read pipe connected to its stdout.
 * When data is received through the pipe, \var{handler} is called
 * with that data.
 */
EXTL_SAFE
EXTL_EXPORT
int ioncore_popen_bgread(const char *cmd, ExtlFn h, ExtlFn errh)
{
    WExecP p;
    
    p.target=NULL;
    p.wd=NULL;
    p.cmd=cmd;
    
    return mainloop_popen_bgread(cmd, setup_exec, (void*)&p, h, errh);
}



/*}}}*/


/*{{{ Exit, restart, snapshot */


static void (*smhook)(int what);

bool ioncore_set_smhook(void (*fn)(int what))
{
    smhook=fn;
    return TRUE;
}


void ioncore_do_exit()
{
    ioncore_deinit();
    exit(0);
}


bool ioncore_do_snapshot()
{
    if(!ioncore_save_layout())
        return FALSE;

    extl_protect(NULL);
    hook_call_v(ioncore_snapshot_hook);
    extl_unprotect(NULL);
    
    return TRUE;
}


void ioncore_emergency_snapshot()
{
    if(smhook!=NULL)
        warn(TR("Not saving state: running under session manager."));
    else
        ioncore_do_snapshot();
}



static char *other=NULL;

static void set_other(const char *s)
{
    if(other!=NULL)
        free(other);
    other=(s==NULL ? NULL : scopy(s));
}


void ioncore_do_restart()
{
    ioncore_deinit();
    if(other!=NULL){
        if(ioncore_g.display!=NULL)
            ioncore_setup_display(NULL);
        mainloop_do_exec(other);
        warn_err_obj(other);
    }
    execvp(ioncore_g.argv[0], ioncore_g.argv);
    die_err_obj(ioncore_g.argv[0]);
}


/*EXTL_DOC
 * Causes the window manager to simply exit without saving
 * state/session.
 */
EXTL_EXPORT
void ioncore_resign()
{
    if(smhook!=NULL){
        smhook(IONCORE_SM_RESIGN);
    }else{
        ioncore_do_exit();
    }
}


/*EXTL_DOC
 * End session saving it first.
 */
EXTL_EXPORT
void ioncore_shutdown()
{
    if(smhook!=NULL){
        smhook(IONCORE_SM_SHUTDOWN);
    }else{
        ioncore_do_snapshot();
        ioncore_do_exit();
    }
}


/*EXTL_DOC
 * Restart, saving session first.
 */
EXTL_EXPORT
void ioncore_restart()
{
    set_other(NULL);
    
    if(smhook!=NULL){
        smhook(IONCORE_SM_RESTART);
    }else{
        ioncore_do_snapshot();
        ioncore_do_restart();
    }
}


/*EXTL_DOC
 * Attempt to restart another window manager \var{cmd}.
 */
EXTL_EXPORT
void ioncore_restart_other(const char *cmd)
{
    set_other(cmd);
    
    if(smhook!=NULL){
        smhook(IONCORE_SM_RESTART_OTHER);
    }else{
        ioncore_do_snapshot();
        ioncore_do_restart();
    }
}


/*EXTL_DOC
 * Save session.
 */
EXTL_EXPORT
void ioncore_snapshot()
{
    if(smhook!=NULL)
        smhook(IONCORE_SM_SNAPSHOT);
    else
        ioncore_do_snapshot();
}


/*}}}*/

