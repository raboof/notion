/*
 * libtu/util.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "locale.h"
#include "util.h"
#include "misc.h"


static const char *progname=NULL;


void libtu_init(const char *argv0)
{
    progname=argv0;

#ifndef CF_NO_GETTEXT
    textdomain(simple_basename(argv0));
#endif
}


const char *libtu_progname()
{
    return progname;
}


const char *libtu_progbasename()
{
    const char *s=strrchr(progname, '/');
    
    return (s==NULL ? progname : s+1);
}

