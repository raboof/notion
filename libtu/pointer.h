/*
 * libtu/pointer.h
 *
 * Copyright (c) Tuomo Valkonen 2005. 
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#ifndef LIBTU_POINTER_H
#define LIBTU_POINTER_H

#define FIELD_OFFSET(T, F) ((long)((char*)&((T*)0)->F))
#define FIELD_TO_STRUCT(T, F, A) ((T*)(((char*)A)-FIELD_OFFSET(T, F)))

#endif /* LIBTU_POINTER_H */
