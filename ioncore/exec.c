/*
 * ion/ioncore/exec.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <limits.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "common.h"
#include "exec.h"
#include "property.h"
#include "signal.h"
#include "global.h"
#include "ioncore.h"
#include "mainloop.h"


#define SHELL_PATH "/bin/sh"
#define SHELL_NAME "sh"
#define SHELL_ARG "-c"


/*{{{ Exec */


void ioncore_do_exec(const char *cmd)
{
    char *argv[4];
    char *tmp=NULL;

    close(ioncore_g.conn);
    
    libtu_asprintf(&tmp, "exec %s", cmd);
    if(tmp!=NULL)
        cmd=tmp;
    
    ioncore_g.dpy=NULL;
    
#ifndef CF_NO_SETPGID
    setpgid(0, 0);
#endif
    
    argv[0]=SHELL_NAME;
    argv[1]=SHELL_ARG;
    argv[2]=(char*)cmd; /* stupid execve... */
    argv[3]=NULL;
    execvp(SHELL_PATH, argv);

    die_err_obj(cmd);
}


static bool do_exec_on_rootwin(int xscr, const char *cmd)
{
    int pid;
    
    if(cmd==NULL)
        return FALSE;

    XSync(ioncore_g.dpy, False);
    
    pid=fork();
    
    if(pid<0){
        warn_err();
        return FALSE;
    }
    
    if(pid!=0)
        return TRUE;
    
    ioncore_setup_environ(xscr);
    
    ioncore_do_exec(cmd);
    
    /* We should not get here */
    return FALSE;
}


/*EXTL_DOC
 * Run \var{cmd} with the environment variable DISPLAY set to point to the
 * root window of the X screen \var{reg} is on.
 */
EXTL_EXPORT
bool ioncore_exec_on(WRegion *reg, const char *cmd)
{
    WRootWin *rw=region_rootwin_of(reg);
    if(rw==NULL){
        WARN_FUNC("No root window.");
        return FALSE;
    }
    
    return do_exec_on_rootwin(rw->xscr, cmd);
}


/*EXTL_DOC
 * Run \var{cmd} with the environment variable DISPLAY set to point to the
 * X display the WM is running on. No specific screen is set unlike with
 * \fnref{WRootWin.exec_on}.
 */
EXTL_EXPORT
bool ioncore_exec(const char *cmd)
{
    return do_exec_on_rootwin(-1, cmd);
}


#define BL 1024

static void process_pipe(int fd, void *p)
{
    char buf[BL];
    int n;
    
    while(1){
        n=read(fd, buf, BL-1);
        if(n<0){
            if(errno==EAGAIN || errno==EINTR)
                return;
            n=0;
            warn_err_obj("pipe read");
        }
        if(n>0){
            buf[n]='\0';
            if(!extl_call(*(ExtlFn*)p, "s", NULL, &buf))
                break;
        }
        if(n==0){
            /* Call with no argument/NULL string to signify EOF */
            extl_call(*(ExtlFn*)p, NULL, NULL);
            break;
        }
        if(n<BL-1)
            return;
    }
    
    /* We get here on EOL or if the handler failed */
    ioncore_unregister_input_fd(fd);
    close(fd);
    extl_unref_fn(*(ExtlFn*)p);
    free(p);
}

#undef BL    
    
/*EXTL_DOC
 * Run \var{cmd} with a read pipe connected to its stdout.
 * When data is received through the pipe, \var{handler} is called
 * with that data.
 */
EXTL_EXPORT
bool ioncore_popen_bgread(const char *cmd, ExtlFn handler)
{
    int pid;
    int fds[2];
    
    if(pipe(fds)!=0){
        warn_err_obj("pipe()");
        return FALSE;
    }

    pid=fork();
    
    if(pid<0){
        close(fds[0]);
        close(fds[1]);
        warn_err();
    }
    
    if(pid!=0){
        ExtlFn *p;
        close(fds[1]);
        /* unblock */ {
            int fl=fcntl(fds[0], F_GETFL);
            if(fl!=-1)
                fl=fcntl(fds[0], F_SETFL, fl|O_NONBLOCK);
            if(fl==-1){
                warn_err();
                close(fds[0]);
                return FALSE;
            }
        }
            
        p=ALLOC(ExtlFn);
        if(p!=NULL){
            *(ExtlFn*)p=extl_ref_fn(handler);
            if(ioncore_register_input_fd(fds[0], p, process_pipe))
                return TRUE;
            extl_unref_fn(*(ExtlFn*)p);
            free(p);
        }
        close(fds[0]);
        return FALSE;
    }
    
    /* Redirect stdout */
    close(fds[0]);
    close(1);
    dup(fds[1]);
    
    ioncore_setup_environ(-1);
    
    ioncore_do_exec(cmd);
    
    /* We should not get here */
    return FALSE;
}


void ioncore_setup_environ(int xscr)
{
    char *tmp, *ptr;
    char *display;
    
    display=XDisplayName(ioncore_g.display);
    
    /* %ui, UINT_MAX is used to ensure there is enough space for the screen
     * number
     */
    libtu_asprintf(&tmp, "DISPLAY=%s.0123456789a", display);

    if(tmp==NULL){
        warn_err();
        return;
    }

    ptr=strchr(tmp, ':');
    if(ptr!=NULL){
        ptr=strchr(ptr, '.');
        if(ptr!=NULL)
            *ptr='\0';
    }

    if(xscr>=0)
        snprintf(tmp+strlen(tmp), 11, ".%u", (unsigned)xscr);
    
    putenv(tmp);
    
    /* No need to free it, we'll execve soon */
    /*free(tmp);*/

    /*XFree(display);*/
}


/*}}}*/


/*{{{ Exit and restart */


static void exitret(int retval)
{    
    ioncore_deinit();
    exit(retval);
}


/*EXTL_DOC
 * Causes the window manager to exit.
 */
EXTL_EXPORT
void ioncore_exit()
{
    exitret(EXIT_SUCCESS);
}


/*EXTL_DOC
 * Attempt to restart another window manager \var{cmd}.
 */
EXTL_EXPORT
void ioncore_restart_other(const char *cmd)
{
    ioncore_deinit();
    if(cmd!=NULL){
        if(ioncore_g.display!=NULL)
            ioncore_setup_environ(-1);
        ioncore_do_exec(cmd);
    }
    execvp(ioncore_g.argv[0], ioncore_g.argv);
    die_err_obj(ioncore_g.argv[0]);
}


/*EXTL_DOC
 * Restart Ioncore.
 */
EXTL_EXPORT
void ioncore_restart()
{
    ioncore_restart_other(NULL);
}


/*}}}*/

