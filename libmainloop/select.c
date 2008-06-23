/*
 * libmainloop/select.c
 * 
 * Partly based on a contributed code.
 *
 * See the included file LICENSE for details.
 */

#include <signal.h>
#include <sys/select.h>

#include <libtu/types.h>
#include <libtu/misc.h>
#include <libtu/dlist.h>

#include "select.h"
#include "signal.h"


/*{{{ File descriptor management */


static WInputFd *input_fds=NULL;

static WInputFd *find_input_fd(int fd)
{
    WInputFd *tmp=input_fds;
    
    while(tmp){
        if(tmp->fd==fd)
            break;
        tmp=tmp->next;
    }
    return tmp;
}

bool mainloop_register_input_fd(int fd, void *data,
                                void (*callback)(int fd, void *d))
{
    WInputFd *tmp;
    
    if(find_input_fd(fd)!=NULL)
        return FALSE;
    
    tmp=ALLOC(WInputFd);
    if(tmp==NULL)
        return FALSE;
    
    tmp->fd=fd;
    tmp->data=data;
    tmp->process_input_fn=callback;
    
    LINK_ITEM(input_fds, tmp, next, prev);
    
    return TRUE;
}

void mainloop_unregister_input_fd(int fd)
{
    WInputFd *tmp=find_input_fd(fd);
    
    if(tmp!=NULL){
        UNLINK_ITEM(input_fds, tmp, next, prev);
        free(tmp);
    }
}

static void set_input_fds(fd_set *rfds, int *nfds)
{
    WInputFd *tmp=input_fds;
    
    while(tmp){
        FD_SET(tmp->fd, rfds);
        if(tmp->fd>*nfds)
            *nfds=tmp->fd;
        tmp=tmp->next;
    }
}

static void check_input_fds(fd_set *rfds)
{
    WInputFd *tmp=input_fds, *next=NULL;
    
    while(tmp){
        next=tmp->next;
        if(FD_ISSET(tmp->fd, rfds))
            tmp->process_input_fn(tmp->fd, tmp->data);
        tmp=next;
    }
}

/*}}}*/


/*{{{ Select */

void mainloop_select()
{
    fd_set rfds;
    int nfds=0;
    int ret=0;
    
    FD_ZERO(&rfds);
    
    set_input_fds(&rfds, &nfds);
    
#ifdef _POSIX_SELECT
    {
        sigset_t oldmask;
        
        mainloop_block_signals(&oldmask);
        
        if(!mainloop_unhandled_signals())
            ret=pselect(nfds+1, &rfds, NULL, NULL, NULL, &oldmask);
        
        sigprocmask(SIG_SETMASK, &oldmask, NULL);
    }
#else
    #warning "pselect() unavailable"
    if(!mainloop_unhandled_signals())
        ret=select(nfds+1, &rfds, NULL, NULL, NULL);
#endif
    if(ret>0)
        check_input_fds(&rfds);
}


/*}}}*/
