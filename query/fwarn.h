/*
 * query/fwarn.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef FWARN_FWARN_H
#define FWARN_FWARN_H

#include <src/frame.h>

extern void fwarn(WFrame *frame, const char *p);
extern void fwarn_free(WFrame *frame, char *p);

#endif /* FWARN_FWARN_H */
