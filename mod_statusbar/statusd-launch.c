/*
 * ion/mod_statusbar/statusd-launch.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2008. 
 *
 * See the included file LICENSE for details.
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
#include <libmainloop/signal.h>
#include <ioncore/saveload.h>
#include <ioncore/bindmaps.h>
#include <ioncore/global.h>
#include <ioncore/ioncore.h>

#include "statusbar.h"


#define CF_STATUSD_TIMEOUT_SEC 3

#define BL 1024

#define USEC 1000000


static bool process_pipe(int fd, ExtlFn fn, 
                         bool *doneseen, bool *eagain)
{
    char buf[BL];
    int n;
    bool fnret;
    
    *eagain=FALSE;
    
    n=read(fd, buf, BL-1);
    
    if(n<0){
        if(errno==EAGAIN || errno==EINTR){
            *eagain=(errno==EAGAIN);
            return TRUE;
        }
        warn_err_obj(TR("reading a pipe"));
        return FALSE;
    }else if(n>0){
        buf[n]='\0';
        *doneseen=FALSE;
        return extl_call(fn, "s", "b", &buf, doneseen);
    }
    
    return FALSE;
}


static bool wait_statusd_init(int outfd, int errfd, ExtlFn dh, ExtlFn eh)
{
    fd_set rfds;
    struct timeval tv, endtime, now;
    int nfds=maxof(outfd, errfd);
    int retval;
    bool dummy, doneseen, eagain=FALSE;
    
    if(mainloop_gettime(&endtime)!=0){
        warn_err();
        return FALSE;
    }
    
    now=endtime;
    endtime.tv_sec+=CF_STATUSD_TIMEOUT_SEC;
    
    while(1){
        FD_ZERO(&rfds);

        /* Calculate remaining time */
        if(now.tv_sec>endtime.tv_sec){
            goto timeout;
        }else if(now.tv_sec==endtime.tv_sec){
            if(now.tv_usec>=endtime.tv_usec)
                goto timeout;
            tv.tv_sec=0;
            tv.tv_usec=endtime.tv_usec-now.tv_usec;
        }else{
            tv.tv_usec=USEC+endtime.tv_usec-now.tv_usec;
            tv.tv_sec=-1+endtime.tv_sec-now.tv_sec;
            /* Kernel lameness tuner: */
            tv.tv_sec+=tv.tv_usec/USEC;
            tv.tv_usec%=USEC;
        }
        
        FD_SET(outfd, &rfds);
        FD_SET(errfd, &rfds);
    
        retval=select(nfds+1, &rfds, NULL, NULL, &tv);
        if(retval>0){
            if(FD_ISSET(errfd, &rfds)){
                if(!process_pipe(errfd, eh, &dummy, &eagain))
                    return FALSE;
            }
            if(FD_ISSET(outfd, &rfds)){
                if(!process_pipe(outfd, dh, &doneseen, &eagain))
                    return FALSE;
                if(doneseen){
                    /* Read rest of errors. */
                    bool ok;
                    do{
                        ok=process_pipe(errfd, eh, &dummy, &eagain);
                    }while(ok && !eagain);
                    return TRUE;
                }
            }
        }else if(retval==0){
            goto timeout;
        }
        
        if(mainloop_gettime(&now)!=0){
            warn_err();
            return FALSE;
        }
    }
    
    return TRUE;
    
timeout:
    /* Just complain to stderr, not startup error log, and do not fail.
     * The system might just be a bit slow. We can continue, but without
     * initial values for the meters, geometry adjustments may be necessary
     * when we finally get that information.
     */
    ioncore_warn_nolog(TR("ion-statusd timed out."));
    return TRUE;
}


EXTL_EXPORT
int mod_statusbar__launch_statusd(const char *cmd,
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

