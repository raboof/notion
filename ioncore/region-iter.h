/*
 * ion/ioncore/region-iter.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_REGION_ITER_H
#define ION_IONCORE_REGION_ITER_H

/* Manager -- managed */

#define REGION_MANAGER(R)          (((WRegion*)(R))->manager)
#define REGION_MANAGER_CHK(R, TYPE) OBJ_CAST(REGION_MANAGER(R), TYPE)

/* Parent -- child */

#define REGION_PARENT(REG) (((WRegion*)(REG))->parent)
#define REGION_PARENT_REG(REG) ((WRegion*)REGION_PARENT(REG))

#define REGION_FIRST_CHILD(PAR) (((WRegion*)(PAR))->children)
#define REGION_LAST_CHILD(PAR)                                               \
 (REGION_FIRST_CHILD(PAR)==NULL ? NULL                                       \
 : REGION_PREV_CHILD_WRAP(REGION_FIRST_CHILD(PAR), REGION_FIRST_CHILD(PAR)))
#define REGION_NEXT_CHILD(PAR, REG) (((WRegion*)(REG))->p_next)
#define REGION_PREV_CHILD(PAR, REG) ((((WRegion*)(REG))->p_prev->p_next) ? \
                                 (((WRegion*)(REG))->p_prev) : NULL)
#define REGION_NEXT_CHILD_WRAP(PAR, REG)                 \
  (((REG) && ((WRegion*)(REG))->p_next)                  \
  ? ((WRegion*)(REG))->p_next : REGION_FIRST_CHILD(PAR))
#define REGION_PREV_CHILD_WRAP(PAR, REG) \
  ((REG) ? ((WRegion*)(REG))->p_prev     \
  : REGION_FIRST_CHILD(PAR))

#define FOR_ALL_CHILDREN(PAR, REG) \
    for((REG)=((WRegion*)(PAR))->children; (REG)!=NULL; (REG)=(REG)->p_next)

#define FOR_ALL_CHILDREN_W_NEXT(PAR, REG, NEXT)                              \
  for((REG)=((WRegion*)(PAR))->children, (NEXT)=((REG)==NULL ? NULL : (REG)->p_next);\
      (REG)!=NULL;                                                            \
      (REG)=(NEXT), (NEXT)=((REG)==NULL ? NULL : (REG)->p_next))

#endif /* ION_IONCORE_REGION_ITER_H */
