/*
 * libtu/minmax.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#ifndef LIBTU_MINMAX_H
#define LIBTU_MINMAX_H


inline int minof(int x, int y)
{
    return (x<y ? x : y);
}


inline int maxof(int x, int y)
{
    return (x>y ? x : y);
}


#endif /* LIBTU_MINMAX_H */

