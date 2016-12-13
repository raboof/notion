/*
 * libtu/errorlog.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004.
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#ifndef LIBTU_ERRORLOG_H
#define LIBTU_ERRORLOG_H

#include <stdio.h>

#include "types.h"
#include "obj.h"
#include "output.h"

#define ERRORLOG_MAX_SIZE (1024*4)

INTRSTRUCT(ErrorLog);
DECLSTRUCT(ErrorLog){
    char *msgs;
    int msgs_len;
    FILE *file;
    bool errors;
    ErrorLog *prev;
    WarnHandler *old_handler;
};

/* el is assumed to be uninitialised  */
extern void errorlog_begin(ErrorLog *el);
extern void errorlog_begin_file(ErrorLog *el, FILE *file);
/* For errorlog_end el Must be the one errorlog_begin was last called with */
extern bool errorlog_end(ErrorLog *el);
extern void errorlog_deinit(ErrorLog *el);

#endif /* LIBTU_ERRORLOG_H */
