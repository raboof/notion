/*
 * mod_notionflux/mod_notionflux/mod_notionflux.c
 *
 * Copyright (c) Tuomo Valkonen 2004-2005. 
 *
 * This is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <ioncore/../version.h>
#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/property.h>
#include <libmainloop/select.h>
#include <libtu/errorlog.h>
#include <libextl/extl.h>

#include "notionflux.h"

typedef struct{
    int fd;
    int ndata;
    char *data;
} Buf;

Buf bufs[MAX_SERVED];

static int listenfd=-1;
static char *listenfile=NULL;
static ExtlFn tostringfn;


/* Without the 'or nil' kludge tostring may have no parameters. */
static const char tostringstr[]=
    "local arg={...}\n"
    "local callable=arg[1]\n"
    "local result=callable()\n"
    "if type(result)=='string' then\n"
    "    return string.format('%q', result)\n"
    "else\n"
    "    return tostring(result)\n"
    "end\n";

static void writes(int fd, const char *s)
{
    if(s!=NULL)
        write(fd, s, strlen(s));
}


static void close_conn(Buf *buf)
{
    if(buf->fd<0)
        return;

    mainloop_unregister_input_fd(buf->fd);
    
    close(buf->fd);
    buf->fd=-1;
    buf->ndata=0;
    if(buf->data!=NULL){
        free(buf->data);
        buf->data=NULL;
    }
}


static void receive_data(int fd, void *buf_)
{
    Buf *buf=(Buf*)buf_;
    bool end=FALSE;
    ErrorLog el;
    ExtlFn fn;
    int i, n;
    bool success=FALSE;
    
    n=read(fd, buf->data+buf->ndata, MAX_DATA-buf->ndata);
    
    if(n==0){
        warn("Connection closed prematurely.");
        close_conn(buf);
        return;
    }
    
    if(n<0){
        if(errno!=EAGAIN && errno!=EINTR){
            writes(fd, "Error: I/O");
            close_conn(buf);
        }
        return;
    }

    for(i=0; i<n; i++){
        if(buf->data[buf->ndata+i]=='\0')
            end=TRUE;
    }

    buf->ndata+=n;
    
    if(!end && buf->ndata+n==MAX_DATA){
        writes(fd, "Error: too much data\n");
        close_conn(buf);
        return;
    }
    
    if(!end)
        return;
    
    errorlog_begin(&el);
    if(extl_loadstring(buf->data, &fn)){
        char *retstr=NULL;
        if(extl_call(tostringfn, "f", "s", fn, &retstr)){
            success=TRUE;
            writes(fd, "S");
            writes(fd, retstr);
            writes(fd, "\n");
            free(retstr);
        }
        extl_unref_fn(fn);
    }
    errorlog_end(&el);
    if(el.msgs!=NULL && !success){
        writes(fd, "E");
        writes(fd, el.msgs);
    }
    errorlog_deinit(&el);

    close_conn(buf);
}


static void connection_attempt(int lfd, void *data)
{
    int i, fd;
    struct sockaddr_un from;
    socklen_t fromlen=sizeof(from);
    
    fd=accept(lfd, (struct sockaddr*)&from, &fromlen);
    
    if(fd<0){
        warn_err();
        return;
    }
    
    /* unblock */ {
        int fl=fcntl(fd, F_GETFL);
        if(fl!=-1)
            fl=fcntl(fd, F_SETFL, fl|O_NONBLOCK);
        if(fl==-1){
            warn_err();
            close(fd);
            return;
        }
    }
    
    /* close socket on exec */ {
        int fl=fcntl(fd, F_GETFD);
        if(fl!=-1)
            fl=fcntl(fd, F_SETFD, fl|FD_CLOEXEC);
        if(fl==-1){
            warn_err();
            close(fd);
            return;
        }

    }
    
    for(i=0; i<MAX_SERVED; i++){
        if(bufs[i].fd<0)
            break;
    }
    
    if(i==MAX_SERVED){
        writes(fd, "Error: busy\n");
        close(fd);
        return;
    }
        
    assert(bufs[i].data==NULL && bufs[i].ndata==0);
    
    bufs[i].data=ALLOC_N(char, MAX_DATA);
    
    if(bufs[i].data!=NULL){
        if(mainloop_register_input_fd(fd, &(bufs[i]), receive_data)){
            bufs[i].fd=fd;
            return;
        }
    }
    
    writes(fd, "Error: malloc\n");
    close(fd);
}


static bool start_listening()
{
    struct sockaddr_un addr;

    listenfile=tmpnam(NULL);
    if(listenfile==NULL){
        warn_err();
        return FALSE;
    }
    
    if(strlen(listenfile)>SOCK_MAX){
        warn("Too long socket path");
        goto err;
    }
    
    listenfd=socket(AF_UNIX, SOCK_STREAM, 0);
    if(listenfd<0)
        goto errwarn;
    
    if(fchmod(listenfd, S_IRUSR|S_IWUSR)<0)
        goto errwarn;
    
    addr.sun_family=AF_UNIX;
    strcpy(addr.sun_path, listenfile);

    {
        int fl=fcntl(listenfd, F_GETFD);
        if(fl!=-1)
            fl=fcntl(listenfd, F_SETFD, fl|FD_CLOEXEC);
        if(fl==-1)
            goto errwarn;
    }

    if(bind(listenfd, (struct sockaddr*) &addr,
            strlen(addr.sun_path)+sizeof(addr.sun_family))<0){
        goto errwarn;
    }
    
    if(listen(listenfd, MAX_SERVED)<0)
        goto errwarn;
        
    if(!mainloop_register_input_fd(listenfd, NULL, connection_attempt))
        goto err;
    
    return TRUE;

errwarn:
    warn_err_obj("mod_notionflux listening socket");
err:
    if(listenfd>=0){
        close(listenfd);
        listenfd=-1;
    }

    /*if(listenfile!=NULL){
        free(listenfile);
        listenfile=NULL;
    }*/
        
    return FALSE;
}


void close_connections()
{
    int i;
    
    if(listenfd>=0){
        mainloop_unregister_input_fd(listenfd);
        close(listenfd);
        listenfd=-1;
    }

    if(listenfile!=NULL){
        unlink(listenfile);
        /*free(listenfile);
        listenfile=NULL;*/
    }
    
    for(i=0; i<MAX_SERVED; i++){
        if(bufs[i].fd>=0)
            close_conn(&(bufs[i]));
    }
    
    extl_unref_fn(tostringfn);
}
    
char mod_notionflux_ion_api_version[]=ION_API_VERSION;

static Atom flux_socket=None;

bool mod_notionflux_init()
{
    int i;
    WRootWin *rw;
    
    for(i=0; i<MAX_SERVED; i++){
        bufs[i].fd=-1;
        bufs[i].data=NULL;
        bufs[i].ndata=0;
    }
    
    if(!extl_loadstring(tostringstr, &tostringfn))
        return FALSE;
    
    if(!start_listening()){
        extl_unref_fn(tostringfn);
        close_connections();
        return FALSE;
    }
        
    flux_socket=XInternAtom(ioncore_g.dpy, "_NOTION_MOD_NOTIONFLUX_SOCKET", False);
    
    FOR_ALL_ROOTWINS(rw){
        xwindow_set_string_property(region_xwindow((WRegion*)rw), flux_socket, listenfile);
    }
    
    return TRUE;
}


void mod_notionflux_deinit()
{
    WRootWin *rw;
    
    if(flux_socket!=None){
        FOR_ALL_ROOTWINS(rw){
          XDeleteProperty(ioncore_g.dpy, region_xwindow((WRegion*)rw), flux_socket);
        }
    }
    
    close_connections();
}

