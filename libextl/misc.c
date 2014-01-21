/*
 * libextl/misc.c
 *
 * Copyright (c) Tuomo Valkonen 2004-2005.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#include "private.h"


bool extl_obj_error(int ndx, const char *got, const char *wanted)
{
    extl_warn(TR("Type checking failed in level 2 call handler for "
                 "parameter %d (got %s, expected %s)."),
              ndx, got ? got : "nil", wanted);

    return FALSE;
}

