/*
 * mod_notionflux/notionflux/notionflux.c
 *
 * Copyright (c) Tuomo Valkonen 2004-2005. 
 *
 * This is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <libtu/types.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "../notionflux.h"

static void die(const char *s)
{
    fprintf(stderr, "%s\n", s);
    exit(1);
}


static void die_e(const char *s)
{
    perror(s);
    exit(1);
}

static void mywrite(int fd, const char *buf, int n)
{
    while(n>0){
        int k;
        k=write(fd, buf, n);
        if(k<0 && (errno!=EAGAIN && errno!=EINTR))
            die_e("Writing");
        if(k>0){
            n-=k;
            buf+=k;
        }
    }
}


static int myread(int fd, char *buf, int n)
{
    int left=n;
    
    while(left>0){
        int k;
        k=read(fd, buf, left);
        if(k<0 && (errno!=EAGAIN && errno!=EINTR))
            die_e("Writing");
        if(k==0)
            break;
        if(k>0){
            left-=k;
            buf+=k;
        }
    }
    return n-left;
}


static char buf[MAX_DATA];


/*{{{ X */


static Display *dpy=NULL;


static ulong xwindow_get_property_(Window win, Atom atom, Atom type, 
                                   ulong n32expected, bool more, uchar **p,
                                   int *format)
{
    Atom real_type;
    ulong n=-1, extra=0;
    int status;
    
    do{
        status=XGetWindowProperty(dpy, win, atom, 0L, n32expected, 
                                  False, type, &real_type, format, &n,
                                  &extra, p);
        
        if(status!=Success || *p==NULL)
            return -1;
    
        if(extra==0 || !more)
            break;
        
        XFree((void*)*p);
        n32expected+=(extra+4)/4;
        more=FALSE;
    }while(1);

    if(n==0){
        XFree((void*)*p);
        *p=NULL;
        return -1;
    }
    
    return n;
}


ulong xwindow_get_property(Window win, Atom atom, Atom type, 
                           ulong n32expected, bool more, uchar **p)
{
    int format=0;
    return xwindow_get_property_(win, atom, type, n32expected, more, p, 
                                 &format);
}


char *xwindow_get_string_property(Window win, Atom a, int *nret)
{
    char *p;
    int n;
    
    n=xwindow_get_property(win, a, XA_STRING, 64L, TRUE, (uchar**)&p);
    
    if(nret!=NULL)
        *nret=n;
    
    return (n<=0 ? NULL : p);
}


void xwindow_set_string_property(Window win, Atom a, const char *value)
{
    if(value==NULL){
        XDeleteProperty(dpy, win, a);
    }else{
        XChangeProperty(dpy, win, a, XA_STRING,
                        8, PropModeReplace, (uchar*)value, strlen(value));
    }
}


static char *get_socket()
{
    Atom a;
    char *s;
    
    dpy=XOpenDisplay(NULL);
    
    if(dpy==NULL)
        die_e("Unable to open display.");
    
    a=XInternAtom(dpy, "_ION_MOD_IONFLUX_SOCKET", True);
    
    if(a==None)
        die_e("Missing atom. Ion not running?");
    
    s=xwindow_get_string_property(DefaultRootWindow(dpy), a, NULL);
    
    XCloseDisplay(dpy);
    
    return s;
}


/*}}}*/


int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_un serv;
    const char *sockname;
    int use_stdin=1;
    char res;
    int n;
    
    if(argc>1){
        if(argc!=3 || strcmp(argv[1], "-e")!=0)
            die("Usage: ionflux [-e code]");
            
        if(strlen(argv[2])>=MAX_DATA)
            die("Too much data.");
        
        use_stdin=0;
    }
        
    sockname=get_socket();
    if(sockname==NULL)
        die("No socket.");
    
    if(strlen(sockname)>SOCK_MAX)
        die("Socket name too long.");
    
    sock=socket(AF_UNIX, SOCK_STREAM, 0);
    if(sock<0)
        die_e("Opening socket");
 
    serv.sun_family=AF_UNIX;
    strcpy(serv.sun_path, sockname);
    
    if(connect(sock, (struct sockaddr*)&serv, sizeof(struct sockaddr_un))<0)
        die_e("Connecting socket");
    
    if(!use_stdin){
        mywrite(sock, argv[2], strlen(argv[2])+1);
    }else{
        char c='\0';
        while(1){
            if(fgets(buf, MAX_DATA, stdin)==NULL)
                break;
            mywrite(sock, buf, strlen(buf));
        }
        mywrite(sock, &c, 1);
    }

    n=myread(sock, &res, 1);
    
    if(n!=1 || (res!='E' && res!='S'))
        die("Invalid response");

    while(1){
        n=myread(sock, buf, MAX_DATA);
        
        if(n==0)
            break;
        
        if(res=='S')
            mywrite(1, buf, n);
        else /* res=='E' */
            mywrite(2, buf, n);
        
        if(n<MAX_DATA)
            break;
    }

    return 0;
}

