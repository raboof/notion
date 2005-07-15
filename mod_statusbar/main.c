/*
 * ion/mod_statusbar/main.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include <libtu/minmax.h>
#include <libextl/readconfig.h>
#include <libmainloop/exec.h>
#include <libmainloop/select.h>
#include <ioncore/saveload.h>
#include <ioncore/bindmaps.h>

#include "statusbar.h"
#include "exports.h"


#define CF_STATUSD_TIMEOUT_SEC 2


/*{{{ Module information */


#include "../version.h"

char mod_statusbar_ion_api_version[]=ION_API_VERSION;


/*}}}*/


/*{{{ Bindmaps */


WBindmap *mod_statusbar_statusbar_bindmap=NULL;


/*}}}*/


/*{{{ Statusd launch helper */


#define BL 1024


static bool process_pipe(int fd, ExtlFn fn, bool *doneseen)
{
    char buf[BL];
    int n;
    bool fnret;
    
    n=read(fd, buf, BL-1);
    if(n<0){
        if(errno==EAGAIN || errno==EINTR)
            return TRUE;
        warn_err_obj(TR("reading a pipe"));
        return FALSE;
    }else if(n>0){
        buf[n]='\0';
        *doneseen=FALSE;
        return extl_call(fn, "s", "b", &buf, doneseen);
    }
    return FALSE;
}


#define USEC 1000000
static void normalise_tv(struct timeval *tv)
{
    while(tv->tv_usec>USEC){
        tv->tv_sec++;
        tv->tv_usec-=USEC;
    }
}


static bool wait_statusd_init(int outfd, int errfd, ExtlFn dh, ExtlFn eh)
{
    fd_set rfds;
    struct timeval tv, endtime, now;
    int nfds=maxof(outfd, errfd);
    int retval;
    bool dummy, doneseen;

    if(gettimeofday(&endtime, NULL)!=0){
        warn_err();
        return FALSE;
    }
    
    now=endtime;
    endtime.tv_sec+=CF_STATUSD_TIMEOUT_SEC;
    
    normalise_tv(&endtime);
    
    while(1){
        FD_ZERO(&rfds);

        normalise_tv(&now);
    
        if(now.tv_sec>endtime.tv_sec){
            return FALSE;
        }else if(now.tv_sec==endtime.tv_sec){
            if(now.tv_usec>=endtime.tv_usec)
                return FALSE;
            tv.tv_sec=0;
            tv.tv_usec=endtime.tv_usec-now.tv_usec;
        }else{
            tv.tv_usec=USEC-now.tv_usec+endtime.tv_sec;
            tv.tv_sec=endtime.tv_sec-(now.tv_sec+1);
        }
        
        FD_SET(outfd, &rfds);
        FD_SET(errfd, &rfds);
    
        retval=select(nfds+1, &rfds, NULL, NULL, &tv);
        if(retval>0){
            if(FD_ISSET(errfd, &rfds)){
                if(!process_pipe(errfd, eh, &dummy))
                    return FALSE;
            }
            if(FD_ISSET(outfd, &rfds)){
                if(!process_pipe(outfd, dh, &doneseen))
                    return FALSE;
                if(doneseen)
                    return TRUE;
            }
        }else if(retval==0){
            /* Timeout */
            return FALSE;
        }
        
        if(gettimeofday(&now, NULL)!=0){
            warn_err();
            return FALSE;
        }
    }

}


EXTL_EXPORT
int mod_statusbar_launch_statusd_(const char *cmd,
                                  ExtlFn initdatahandler,
                                  ExtlFn initerrhandler,
                                  ExtlFn datahandler,
                                  ExtlFn errhandler)
{
    pid_t pid;
    int outfd=-1, errfd=-1;
    
    if(cmd==NULL)
        return -1;
    
    pid=mainloop_do_spawn(cmd, NULL, NULL,
                          NULL, &outfd, &errfd);
    
    if(pid<0)
        return -1;
    
    if(!wait_statusd_init(outfd, errfd, initdatahandler, initerrhandler))
        goto err;
    
    if(!mainloop_register_input_fd_extlfn(outfd, datahandler))
        goto err;
    
    if(!mainloop_register_input_fd_extlfn(errfd, errhandler))
        goto err2;

    return pid;
    
err2:    
    mainloop_unregister_input_fd(outfd);
err:    
    close(outfd);
    close(errfd);
    return -1;
}


/*}}}*/


/*{{{ Init & deinit */


void mod_statusbar_deinit()
{
    if(mod_statusbar_statusbar_bindmap!=NULL){
        ioncore_free_bindmap("WStatusBar", mod_statusbar_statusbar_bindmap);
        mod_statusbar_statusbar_bindmap=NULL;
    }

    ioncore_unregister_regclass(&CLASSDESCR(WStatusBar));

    mod_statusbar_unregister_exports();
}


bool mod_statusbar_init()
{
    mod_statusbar_statusbar_bindmap=ioncore_alloc_bindmap("WStatusBar", NULL);
    
    if(mod_statusbar_statusbar_bindmap==NULL)
        return FALSE;

    if(!ioncore_register_regclass(&CLASSDESCR(WStatusBar),
                                  (WRegionLoadCreateFn*) statusbar_load)){
        mod_statusbar_deinit();
        return FALSE;
    }

    if(!mod_statusbar_register_exports()){
        mod_statusbar_deinit();
        return FALSE;
    }
    
    /*ioncore_read_config("cfg_statusbar", NULL, TRUE);*/
    
    return TRUE;
}


/*}}}*/

