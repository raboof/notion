/*
 * ion/mod_query/main.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_MOD_QUERY_MAIN_H
#define ION_MOD_QUERY_MAIN_H

extern bool mod_query_init();
extern void mod_query_deinit();

INTRSTRUCT(ModQueryConfig);

DECLSTRUCT(ModQueryConfig){
    int autoshowcompl_delay;
    bool autoshowcompl;
    bool caseicompl;
    bool substrcompl;
};


extern ModQueryConfig mod_query_config;

#endif /* ION_MOD_QUERY_MAIN_H */
