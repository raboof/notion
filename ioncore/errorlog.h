/*
 * ion/ioncore/errorlog.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_ERRORLOG_H
#define ION_IONCORE_ERRORLOG_H

#include <stdio.h>

#include "common.h"
#include "obj.h"

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
extern void begin_errorlog(ErrorLog *el);
extern void begin_errorlog_file(ErrorLog *el, FILE *file);
/* For end_errorlog el Must be the one begin_errorlog was last called with */
extern bool end_errorlog(ErrorLog *el);
extern void deinit_errorlog(ErrorLog *el);

#endif /* ION_IONCORE_ERRORLOG_H */
