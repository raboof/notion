/*
 * ion/ioncore/readfds.h
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
	void (*process_input_fn)(int fd);
	InputFd *next, *prev;
};

extern void register_input_fd(int fd, void (*process_input_fn)(int fd));
extern void unregister_input_fd(int fd);
extern void set_input_fds(fd_set *rfds, int *nfds);
extern void check_input_fds(fd_set *rfds);

#endif /* ION_IONCORE_READFDS_H */
