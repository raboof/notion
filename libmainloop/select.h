/*
 * ion/mainloop/select.h
 * 
 * Based on a contributed readfds code.
 * 
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_LIBMAINLOOP_SELECT_H
#define ION_LIBMAINLOOP_SELECT_H

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <libtu/obj.h>
#include <libtu/types.h>

INTRSTRUCT(WInputFd);

DECLSTRUCT(WInputFd){
    int fd;
    void *data;
    void (*process_input_fn)(int fd, void *data);
    WInputFd *next, *prev;
};

extern bool mainloop_register_input_fd(int fd, void *data,
                                       void (*callback)(int fd, void *data));
extern void mainloop_unregister_input_fd(int fd);

extern void mainloop_select();

#endif /* ION_LIBMAINLOOP_SELECT_H */
