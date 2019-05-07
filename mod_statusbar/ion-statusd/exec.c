/*
 * ion/mod_statusbar/ion-statusd/exec.c
 *
 * Copyright (c) Tuomo Valkonen 2005-2006.
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */


#include <libmainloop/select.h>
#include <libmainloop/exec.h>


/*EXTL_DOC
 * Run \var{cmd} in the background.
 */
EXTL_SAFE
EXTL_EXPORT
int statusd_exec(const char *cmd)
{
    return mainloop_spawn(cmd);
}


/*EXTL_DOC
 * Run \var{cmd} with a read pipe connected to its stdout.
 * When data is received through the pipe, \var{h} is called
 * with that data.
 */
EXTL_SAFE
EXTL_EXPORT
int statusd_popen_bgread(const char *cmd, ExtlFn h, ExtlFn errh)
{
    return mainloop_popen_bgread(cmd, NULL, NULL, h, errh);
}

