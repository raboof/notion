/*
 * ion/ioncore/readfds.c
 * 
 * Based on a contributed code.
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "readfds.h"


static InputFd *input_fds=NULL;

static InputFd *find_input_fd(int fd)
{
	InputFd *tmp=input_fds;
	
	while(tmp){
		if(tmp->fd==fd)
			break;
		tmp=tmp->next;
	}
	return tmp;
}

bool register_input_fd(int fd, void *data,
					   void (*process_input_fn)(int fd, void *d))
{
	InputFd *tmp;
	
	if(find_input_fd(fd)!=NULL){
		warn("File descriptor already registered\n");
		return FALSE;
	}
	
	tmp=ALLOC(InputFd);
	if(tmp==NULL){
		warn_err();
		return FALSE;
	}
	
	tmp->fd=fd;
	tmp->data=data;
	tmp->process_input_fn=process_input_fn;
	
	LINK_ITEM(input_fds, tmp, next, prev);
	
	return TRUE;
}

void unregister_input_fd(int fd)
{
	InputFd *tmp=find_input_fd(fd);
	
	if(tmp!=NULL){
		UNLINK_ITEM(input_fds, tmp, next, prev);
		free(tmp);
	}
}

void set_input_fds(fd_set *rfds, int *nfds)
{
	InputFd *tmp=input_fds;
	
	while(tmp){
		FD_SET(tmp->fd, rfds);
		if(tmp->fd>*nfds)
			*nfds=tmp->fd;
		tmp=tmp->next;
	}
}

void check_input_fds(fd_set *rfds)
{
	InputFd *tmp=input_fds, *next=NULL;
	
	while(tmp){
		next=tmp->next;
		if(FD_ISSET(tmp->fd, rfds))
			tmp->process_input_fn(tmp->fd, tmp->data);
		tmp=next;
	}
}
