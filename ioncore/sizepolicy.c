/*
 * ion/ioncore/sizepolicy.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "common.h"
#include "region.h"
#include "sizepolicy.h"


void sizepolicy(WSizePolicy szplcy, const WRegion *reg,
                const WRectangle *rq_geom, WFitParams *fp)
{
    if(szplcy==SIZEPOLICY_FREE){
        WRectangle tmp;
        if(rq_geom!=NULL)
            tmp=*rq_geom;
        else if(reg!=NULL)
            tmp=REGION_GEOM(reg);
        else
            tmp=fp->g;
        
        /* TODO: size hints */
        rectangle_constrain(&tmp, &(fp->g));
        fp->g=tmp;
        fp->mode=REGION_FIT_EXACT;
    }else{
        fp->mode=(szplcy==SIZEPOLICY_FULL_BOUNDS
                  ? REGION_FIT_BOUNDS
                  : REGION_FIT_EXACT);
    }
}

