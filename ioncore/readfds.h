/*
 * ion/ioncore/readfds.h
 * 
 * Based on a contributed code.
 * 
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_READFDS_H
#define ION_IONCORE_READFDS_H

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "common.h"
#include "obj.h"

INTRSTRUCT(InputFd);
	
DECLSTRUCT(InputFd){
	int fd;
	void *data;
	void (*process_input_fn)(int fd, void *data);
	InputFd *next, *prev;
};

extern bool register_input_fd(int fd, void *data,
							  void (*process_input_fn)(int fd, void *data));
extern void unregister_input_fd(int fd);
extern void set_input_fds(fd_set *rfds, int *nfds);
extern void check_input_fds(fd_set *rfds);

#endif /* ION_IONCORE_READFDS_H */
