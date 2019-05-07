/*
 * libtu/setparam.c
 *
 * Copyright (c) Tuomo Valkonen 2005.
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#include <string.h>

#include "setparam.h"

int libtu_string_to_setparam(const char *str)
{
    if(str!=NULL){
        if(strcmp(str, "set")==0 || strcmp(str, "true")==0)
            return SETPARAM_SET;
        else if(strcmp(str, "unset")==0 || strcmp(str, "false")==0)
            return SETPARAM_UNSET;
        else if(strcmp(str, "toggle")==0)
            return SETPARAM_TOGGLE;
    }

    return SETPARAM_UNKNOWN;
}


bool libtu_do_setparam(int sp, bool val)
{
    switch(sp){
    case SETPARAM_SET:
        return TRUE;
    case SETPARAM_UNSET:
        return FALSE;
    case SETPARAM_TOGGLE:
        return (val==FALSE);
    default:
        return val;
    }
}

bool libtu_do_setparam_str(const char *str, bool val)
{
    return libtu_do_setparam(libtu_string_to_setparam(str), val);
}


int libtu_setparam_invert(int sp)
{
    switch(sp){
    case SETPARAM_SET:
        return SETPARAM_UNSET;
    case SETPARAM_UNSET:
        return SETPARAM_SET;
    default:
        return sp;
    }
}

