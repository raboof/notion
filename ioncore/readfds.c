/*
 * ion/ioncore/readfds.c
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

void register_input_fd(int fd, void (*process_input_fn)(int fd))
{
	InputFd *tmp;
	
	if(find_input_fd(fd)!=NULL){
		warn("File descriptor already registered\n");
		return;
	}
	
	tmp=ALLOC(InputFd);
	tmp->fd=fd;
	tmp->process_input_fn=process_input_fn;
	
	LINK_ITEM(input_fds, tmp, next, prev);
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
	InputFd *tmp=input_fds;
	
	while(tmp){
		if(FD_ISSET(tmp->fd, rfds))
			tmp->process_input_fn(tmp->fd);
		tmp=tmp->next;
	}
}
