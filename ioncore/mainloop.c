/*
 * ion/ioncore/mainloop.c
 * 
 * Partly based on a contributed code.
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "mainloop.h"
#include "global.h"
#include "signal.h"
#include "defer.h"


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

bool ioncore_register_input_fd(int fd, void *data,
                               void (*callback)(int fd, void *d))
{
	WInputFd *tmp;
	
	if(find_input_fd(fd)!=NULL){
		warn("File descriptor already registered\n");
		return FALSE;
	}
	
	tmp=ALLOC(WInputFd);
	if(tmp==NULL){
		warn_err();
		return FALSE;
	}
	
	tmp->fd=fd;
	tmp->data=data;
	tmp->process_input_fn=callback;
	
	LINK_ITEM(input_fds, tmp, next, prev);
	
	return TRUE;
}

void ioncore_unregister_input_fd(int fd)
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

void ioncore_mainloop()
{
	fd_set rfds;
	int nfds=0;
    
	ioncore_g.opmode=IONCORE_OPMODE_NORMAL;

	while(1){
		ioncore_check_signals();
		ioncore_execute_deferred();
		
		FD_ZERO(&rfds);
		FD_SET(ioncore_g.conn, &rfds);
		
		set_input_fds(&rfds, &nfds);
		
		if(select(nfds+1, &rfds, NULL, NULL, NULL)>0)
			check_input_fds(&rfds);
	}
}

