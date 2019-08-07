/*
 * notion/ioncore/tempdir.c
 *
 * Copyright (c) 2019 Moritz Wilhelmy
 *
 * See the included file LICENSE for details.
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L /* mkdtemp(3) */

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <stddef.h>

#include "tempdir.h"
#include "log.h"

static char template[]="/tmp/notion.XXXXXX\0\0";
static char const *tempdir=NULL;

/*EXTL_DOC
 * Returns the global temporary directory for this notion instance with a
 * trailing slash. The directory is created in case it does not exist.
 * The directory will be non-recursively deleted on teardown, therefore no
 * subdirectories should be created.
 */
EXTL_SAFE
EXTL_EXPORT
const char *ioncore_tempdir()
{
    if(tempdir)
        return tempdir;

    tempdir=mkdtemp(template); /* points to template */
    if(!tempdir){
        LOG(ERROR, GENERAL, "Error creating temporary directory: %s.",
            strerror(errno));
        return NULL;
    }

    template[strlen(tempdir)]='/';
    return tempdir;
}

void ioncore_tempdir_cleanup(void)
{
    DIR *dir;
    struct dirent *dent;
    char fname[sizeof(template)+NAME_MAX];
    size_t const offs=strlen(template);

    if(!tempdir)
        return;

    dir=opendir(tempdir);
    if(!dir){
        LOG(ERROR, GENERAL, "Error opening temporary directory: %s.",
            strerror(errno));
        return;
    }

    strcpy(fname, tempdir);
    while((dent=readdir(dir))){
        strcpy(fname+offs, dent->d_name);
        unlink(fname);
    }
    rmdir(tempdir);
    tempdir=NULL;
}
