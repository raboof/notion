/*
 * ion/ioncore/minmax.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_MINMAX_H
#define ION_IONCORE_MINMAX_H


static int minof(int x, int y)
{
	return (x<y ? x : y);
}


static int maxof(int x, int y)
{
	return (x>y ? x : y);
}


#endif /* ION_IONCORE_MINMAX_H */

