/*
 * ion/ioncore/errorlog.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_ERRORLOG_H
#define ION_IONCORE_ERRORLOG_H

#include <stdio.h>

#include "common.h"
#include "obj.h"

#define ERRORLOG_MAX_SIZE (1024*4)

INTRSTRUCT(WErrorLog);
DECLSTRUCT(WErrorLog){
	char *msgs;
	int msgs_len;
	FILE *file;
	bool errors;
	WErrorLog *prev;
	WarnHandler *old_handler;
};

/* el is assumed to be uninitialised  */
extern void errorlog_begin(WErrorLog *el);
extern void errorlog_begin_file(WErrorLog *el, FILE *file);
/* For errorlog_end el Must be the one errorlog_begin was last called with */
extern bool errorlog_end(WErrorLog *el);
extern void errorlog_deinit(WErrorLog *el);

#endif /* ION_IONCORE_ERRORLOG_H */
