/*
 * mod_notionflux/mod_notionflux/mod_notionflux.c
 *
 * Copyright (c) Tuomo Valkonen 2004-2005.
 * Copyright (c) The Notion development team 2019
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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <ioncore/../version.h>
#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/property.h>
#include <ioncore/tempdir.h>
#include <libmainloop/select.h>
#include <libtu/errorlog.h>
#include <libextl/extl.h>
#include <libextl/luaextl.h>

#include "notionflux.h"
#include "exports.h"

typedef struct{
    int fd;
    FILE *stdoutput;
    int ndata;
    char *data;
} Buf;

Buf bufs[MAX_SERVED];

static int listenfd=-1;
static char *listenfile=NULL;
static ExtlFn tostringfn;

static void writes(int fd, const char *s)
{
    if(s!=NULL)
        ignore_value(write(fd, s, strlen(s)));
}


static void close_conn(Buf *buf)
{
    if(buf->fd<0)
        return;

    mainloop_unregister_input_fd(buf->fd);

    close(buf->fd);
    buf->fd=-1;
    buf->ndata=0;
    if(buf->stdoutput!=NULL){
        fclose(buf->stdoutput);
        buf->stdoutput=NULL;
    }
    if(buf->data!=NULL){
        free(buf->data);
        buf->data=NULL;
    }
}

/* Receive file descriptors from a unix domain socket.
   See unix(4), unix(7), sendmsg(2), cmsg(3) */
static int unix_recv_fd(int unix)
{
    /* tasty, tasty boilerplate. */
    struct msghdr msg={0};
    struct cmsghdr *cmsg;
    struct iovec iov;
    unsigned char magic[4]={23, 42, 128, 7};
    char iov_buf[4];
    int fd=-1;
    union { /* alignment, alignment, cmsg(3) */
        char buf[CMSG_SPACE(sizeof(fd))];
        struct cmsghdr align;
    } u;
    int iter=0, rv;

    iov.iov_len=sizeof(iov_buf);
    iov.iov_base=&iov_buf;

    msg.msg_name=NULL;
    msg.msg_namelen=0;
    msg.msg_iov=&iov;
    msg.msg_iovlen=1;

    msg.msg_control=&u.buf;
    msg.msg_controllen=sizeof(u.buf);

    rv=recvmsg(unix, &msg, MSG_CMSG_CLOEXEC);
    if (rv== -1){
        warn("Failed to receive fd from unix domain socket.");
        return -1;
    }else if(rv==0){ /* remote socket was shut down */
        return -2;
    }
    if(memcmp(iov_buf, magic, sizeof(magic))!=0)
        return -3;

    for (cmsg=CMSG_FIRSTHDR(&msg); cmsg!=NULL; cmsg=CMSG_NXTHDR(&msg, cmsg)) {
        if(iter>0){
            warn("notionflux client is sending us unexpected packets");
        }else if(cmsg->cmsg_level == SOL_SOCKET
              && cmsg->cmsg_type == SCM_RIGHTS
              && cmsg->cmsg_len == CMSG_LEN(sizeof(fd))){
            unsigned char const *data=CMSG_DATA(cmsg);
            int newfd=*((int *)data);
            if (newfd == -1) {
                warn("notionflux managed to send invalid fd.");
            }else if (fd != -1 && newfd != fd) {
                warn("notionflux client is spamming us with fds, ignoring.");
                close(newfd); /* only want one fd from this connection */
            } else {
                fd = newfd;
            }
        }else{
            warn("Unknown control message on unix domain socket.");
        }
        iter++;
    }

    return fd;
}

static void receive_data(int fd, void *buf_)
{
    Buf *buf=(Buf*)buf_;
    bool end=FALSE;
    ErrorLog el;
    ExtlFn fn;
    int i, n;
    bool success=FALSE;
    int idx=buf-bufs;

    if(buf->stdoutput==NULL){ /* no fd received yet, must be the very beginning */
        int stdoutput_fd=unix_recv_fd(fd);
        if(stdoutput_fd==-2)
            goto closefd;
        if(stdoutput_fd==-3){
            char const *err="Magic number mismatch on notionflux socket - "
                "is notionflux the same version as notion?";
            writes(fd, "E");
            writes(fd, err);
            warn(err);
            goto closefd;
        }

        if(stdoutput_fd==-1) {
            if(errno==EWOULDBLOCK || errno==EAGAIN)
                return; /* try again later */
            warn("No file descriptor received from notionflux, closing.");
            goto closefd;
        }
        if((buf->stdoutput=fdopen(stdoutput_fd, "w"))==NULL) {
            warn("fdopen() failed on fd from notionflux");
            goto closefd;
        }
    }

    n=read(fd, buf->data+buf->ndata, MAX_DATA-buf->ndata);

    if(n==0){
        warn("Connection closed prematurely.");
        goto closefd;
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
        goto closefd;
    }

    if(!end)
        return;

    errorlog_begin(&el);

    if(!extl_lookup_global_value(&tostringfn, 'f', "mod_notionflux", "run_lua", NULL)){
        warn("mod_notionflux.run_lua() function is missing");
        return;
    }
    if(extl_loadstring(buf->data, &fn)){
        char *retstr=NULL;
        if(extl_call(tostringfn, "fi", "s", fn, idx, &retstr)){
            success=TRUE;
            writes(fd, "S");
            writes(fd, retstr);
            writes(fd, "\n");
            free(retstr);
        }
        extl_unref_fn(fn);
    }
    extl_unref_fn(tostringfn);
    errorlog_end(&el);
    if(el.msgs!=NULL && !success){
        writes(fd, "E");
        writes(fd, el.msgs);
    }
    errorlog_deinit(&el);

closefd:
    close_conn(buf);
    return;
}

/**EXTL_DOC
 * Write \var{str} to the output file descriptor associated with client \var{idx}
 */
EXTL_SAFE
EXTL_EXPORT
bool mod_notionflux_xwrite(int idx, const char *str)
{
    if (idx<0 || idx>=MAX_SERVED || bufs[idx].stdoutput==NULL)
        return FALSE;
    return fputs(str, bufs[idx].stdoutput)!=EOF;
}

static void connection_attempt(int lfd, void *UNUSED(data))
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
    char const *tempdir=ioncore_tempdir();
    char const *filename="notionflux.socket";

    listenfile = ALLOC_N(char, strlen(tempdir)+strlen(filename)+1);
    if(!listenfile || !tempdir)
        goto err;

    strcat(listenfile, tempdir);
    strcat(listenfile, filename);

    if(strlen(listenfile)>sizeof(addr.sun_path)-1){
        warn("Socket path too long");
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

    if(bind(listenfd, (struct sockaddr*) &addr, sizeof(addr))<0)
        goto errwarn;

    if(chmod(listenfile, S_IRUSR|S_IWUSR)<0)
        goto errwarn;

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

    if(listenfile!=NULL){
        free(listenfile);
        listenfile=NULL;
    }

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
        free(listenfile);
        listenfile=NULL;
    }

    for(i=0; i<MAX_SERVED; i++){
        if(bufs[i].fd>=0)
            close_conn(&(bufs[i]));
    }

}

char mod_notionflux_ion_api_version[]=ION_API_VERSION;

static Atom flux_socket=None;

bool mod_notionflux_init()
{
    int i;
    WRootWin *rw;

    if (!mod_notionflux_register_exports())
        return FALSE;

    for(i=0; i<MAX_SERVED; i++){
        bufs[i].fd=-1;
        bufs[i].stdoutput=NULL;
        bufs[i].data=NULL;
        bufs[i].ndata=0;
    }

    if (!start_listening())
        goto err_listening;

    flux_socket=XInternAtom(ioncore_g.dpy, "_NOTION_MOD_NOTIONFLUX_SOCKET", False);

    FOR_ALL_ROOTWINS(rw){
        xwindow_set_string_property(region_xwindow((WRegion*)rw), flux_socket, listenfile);
    }

    return TRUE;

err_listening:
    close_connections();
    return FALSE;
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

