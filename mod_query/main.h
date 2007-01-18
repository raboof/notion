/*
 * ion/mod_query/main.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_QUERY_MAIN_H
#define ION_MOD_QUERY_MAIN_H

extern bool mod_query_init();
extern void mod_query_deinit();

INTRSTRUCT(ModQueryConfig);

DECLSTRUCT(ModQueryConfig){
    int autoshowcompl_delay;
    bool autoshowcompl;
};


extern ModQueryConfig mod_query_config;

#endif /* ION_MOD_QUERY_MAIN_H */
