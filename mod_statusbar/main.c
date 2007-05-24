/*
 * ion/mod_statusbar/main.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
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
#include "exports.h"


#define CF_STATUSD_TIMEOUT_SEC 3


/*{{{ Module information */


#include "../version.h"

char mod_statusbar_ion_api_version[]=ION_API_VERSION;


/*}}}*/


/*{{{ Bindmaps */


WBindmap *mod_statusbar_statusbar_bindmap=NULL;


/*}}}*/


/*{{{ Statusd launch helper */


#define BL 1024


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


#define USEC 1000000


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


/*}}}*/


/*{{{ Systray */


static bool is_systray(WClientWin *cwin)
{
    static Atom atom__kde_net_wm_system_tray_window_for=None;
    Atom actual_type=None;
    int actual_format;
    unsigned long nitems;
    unsigned long bytes_after;
    unsigned char *prop;
    char *dummy;
    bool is=FALSE;

    if(extl_table_gets_s(cwin->proptab, "statusbar", &dummy)){
        free(dummy);
        return TRUE;
    }
    
    if(atom__kde_net_wm_system_tray_window_for==None){
        atom__kde_net_wm_system_tray_window_for=XInternAtom(ioncore_g.dpy,
                                                            "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR",
                                                            False);
    }
    if(XGetWindowProperty(ioncore_g.dpy, cwin->win,
                          atom__kde_net_wm_system_tray_window_for, 0,
                          sizeof(Atom), False, AnyPropertyType, 
                          &actual_type, &actual_format, &nitems,
                          &bytes_after, &prop)==Success){
        if(actual_type!=None){
            is=TRUE;
        }
        XFree(prop);
    }
    
    return is;
}


static bool clientwin_do_manage_hook(WClientWin *cwin, const WManageParams *param)
{
    WStatusBar *sb=NULL;
    
    if(!is_systray(cwin))
        return FALSE;
    
    sb=mod_statusbar_find_suitable(cwin, param);
    if(sb==NULL)
        return FALSE;

    return region_manage_clientwin((WRegion*)sb, cwin, param,
                                   MANAGE_PRIORITY_NONE);
}

    
/*}}}*/


/*{{{ Init & deinit */


void mod_statusbar_deinit()
{
    hook_remove(clientwin_do_manage_alt, 
                (WHookDummy*)clientwin_do_manage_hook);

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
    
    hook_add(clientwin_do_manage_alt, 
             (WHookDummy*)clientwin_do_manage_hook);

    /*ioncore_read_config("cfg_statusbar", NULL, TRUE);*/
    
    return TRUE;
}


/*}}}*/

