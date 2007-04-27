/*
 * ion/ioncore/region-iter.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_REGION_ITER_H
#define ION_IONCORE_REGION_ITER_H

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
