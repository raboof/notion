/*
 * libtu/common.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005.
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#ifndef LIBTU_DLIST_H
#define LIBTU_DLIST_H


/*{{{ Linking */


#define LINK_ITEM(LIST, ITEM, NEXT, PREV) \
    (ITEM)->NEXT=NULL;                    \
    if((LIST)==NULL){                     \
        (LIST)=(ITEM);                    \
        (ITEM)->PREV=(ITEM);              \
    }else{                                \
        (ITEM)->PREV=(LIST)->PREV;        \
        (ITEM)->PREV->NEXT=(ITEM);        \
        (LIST)->PREV=(ITEM);              \
    }


#define LINK_ITEM_FIRST(LIST, ITEM, NEXT, PREV) \
    (ITEM)->NEXT=(LIST);                        \
    if((LIST)==NULL){                           \
        (ITEM)->PREV=(ITEM);                    \
    }else{                                      \
        (ITEM)->PREV=(LIST)->PREV;              \
        (LIST)->PREV=(ITEM);                    \
    }                                           \
    (LIST)=(ITEM);


#define LINK_ITEM_LAST LINK_ITEM


#define LINK_ITEM_BEFORE(LIST, BEFORE, ITEM, NEXT, PREV) \
    (ITEM)->NEXT=(BEFORE);                               \
    (ITEM)->PREV=(BEFORE)->PREV;                         \
    (BEFORE)->PREV=(ITEM);                               \
    if((BEFORE)==(LIST))                                 \
        (LIST)=(ITEM);                                   \
    else                                                 \
        (ITEM)->PREV->NEXT=(ITEM)


#define LINK_ITEM_AFTER(LIST, AFTER, ITEM, NEXT, PREV) \
    (ITEM)->NEXT=(AFTER)->NEXT;                        \
    (ITEM)->PREV=(AFTER);                              \
    (AFTER)->NEXT=(ITEM);                              \
    if((ITEM)->NEXT==NULL)                             \
        (LIST)->PREV=(ITEM);                           \
    else                                               \
        (ITEM)->NEXT->PREV=ITEM;


#define UNLINK_ITEM(LIST, ITEM, NEXT, PREV)  \
    if((ITEM)->PREV!=NULL){                  \
        if((ITEM)==(LIST)){                  \
            (LIST)=(ITEM)->NEXT;             \
            if((LIST)!=NULL)                 \
                (LIST)->PREV=(ITEM)->PREV;   \
        }else if((ITEM)->NEXT==NULL){        \
            (ITEM)->PREV->NEXT=NULL;         \
            (LIST)->PREV=(ITEM)->PREV;       \
        }else{                               \
            (ITEM)->PREV->NEXT=(ITEM)->NEXT; \
            (ITEM)->NEXT->PREV=(ITEM)->PREV; \
        }                                    \
    }                                        \
    (ITEM)->NEXT=NULL;                       \
    (ITEM)->PREV=NULL;


/*}}}*/


/*{{{ Iteration */


#define LIST_FIRST(LIST, NEXT, PREV) \
    (LIST)
#define LIST_LAST(LIST, NEXT, PREV) \
    ((LIST)==NULL ? NULL : LIST_PREV_WRAP(LIST, LIST, NEXT, PREV))
#define LIST_NEXT(LIST, REG, NEXT, PREV) \
    ((REG)->NEXT)
#define LIST_PREV(LIST, REG, NEXT, PREV) \
    ((REG)->PREV->NEXT ? (REG)->PREV : NULL)
#define LIST_NEXT_WRAP(LIST, REG, NEXT, PREV) \
    (((REG) && (REG)->NEXT) ? (REG)->NEXT : (LIST))
#define LIST_PREV_WRAP(LIST, REG, NEXT, PREV) \
    ((REG) ? (REG)->PREV : (LIST))

#define LIST_FOR_ALL(LIST, NODE, NEXT, PREV) \
    for(NODE=LIST; NODE!=NULL; NODE=(NODE)->NEXT)

#define LIST_FOR_ALL_REV(LIST, NODE, NEXT, PREV)     \
    for(NODE=((LIST)==NULL ? NULL : (LIST)->PREV);   \
        NODE!=NULL;                                  \
        NODE=((NODE)==(LIST) ? NULL : (NODE)->PREV))

#define LIST_FOR_ALL_W_NEXT(LIST, NODE, NXT, NEXT, PREV)  \
    for(NODE=LL, NXT=(NODE==NULL ? NULL : (NODE)->NEXT);  \
        NODE!=NULL;                                       \
        NODE=NXT, NXT=(NODE==NULL ? NULL : (NODE)->NEXT))

#define LIST_FOR_ALL_W_NEXT_REV(LIST, NODE, NXT, NEXT, PREV) \
    for(NODE=((LIST)==NULL ? NULL : (LIST)->PREV),           \
         NXT=((NODE)==(LIST) ? NULL : (NODE)->PREV);         \
        NODE!=NULL;                                          \
        NODE=NXT,                                            \
         NXT=((NODE)==(LIST) ? NULL : (NODE)->PREV))


/*}}}*/


#endif /* LIBTU_DLIST_H */
