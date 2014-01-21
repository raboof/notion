/*
 * libtu/map.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#include <string.h>
#include "map.h"


int stringintmap_ndx(const StringIntMap *map, const char *str)
{
    int i;
    
    for(i=0; map[i].string!=NULL; i++){
        if(strcmp(str, map[i].string)==0)
            return i;
    }
    
    return -1;
}


int stringintmap_value(const StringIntMap *map, const char *str, int dflt)
{
    int i=stringintmap_ndx(map, str);
    return (i==-1 ? dflt : map[i].value); 
}


const char *stringintmap_key(const StringIntMap *map, int value, 
                             const char *dflt)
{
    int i;
    
    for(i=0; map[i].string!=NULL; ++i){
        if(map[i].value==value){
            return map[i].string;
        }
    }
    
    return dflt;
    
}



int stringfunptrmap_ndx(const StringFunPtrMap *map, const char *str)
{
    int i;
    
    for(i=0; map[i].string!=NULL; i++){
        if(strcmp(str, map[i].string)==0)
            return i;
    }
    
    return -1;
}


FunPtr stringfunptrmap_value(const StringFunPtrMap *map, const char *str, FunPtr dflt)
{
    int i=stringfunptrmap_ndx(map, str);
    return (i==-1 ? dflt : map[i].value); 
}


const char *stringfunptrmap_key(const StringFunPtrMap *map, FunPtr value, 
                             const char *dflt)
{
    int i;
    
    for(i=0; map[i].string!=NULL; ++i){
        if(map[i].value==value){
            return map[i].string;
        }
    }
    
    return dflt;
}
