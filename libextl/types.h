/*
 * libextl/types.h
 *
 * Copyright (c) Tuomo Valkonen 2004-2005.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#ifndef LIBEXTL_TYPES_H
#define LIBEXTL_TYPES_H

#if 1

#include <libtu/types.h>
#include <libtu/obj.h>

#else

#ifndef bool
#define bool int
#define TRUE 1
#define FALSE 0
#endif

/*typedef ? Obj;*/

#endif

#endif /* LIBEXTL_TYPES_H */
