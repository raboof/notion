/*
 * ion/mainloop/select.h
 * 
 * Based on a contributed readfds code.
 * 
 * See the included file LICENSE for details.
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

extern void mainloop_select(void);

#endif /* ION_LIBMAINLOOP_SELECT_H */
