/*
 * ion/mod_tiling/split-stdisp.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
 */

#include <libtu/minmax.h>
#include <ioncore/common.h>
#include <ioncore/mplex.h>
#include <ioncore/resize.h>
#include "split.h"
#include "split-stdisp.h"
#include "tiling.h"


/*{{{ Helper routines */


#define STDISP_IS_HORIZONTAL(STDISP) \
        ((STDISP)->orientation==REGION_ORIENTATION_HORIZONTAL)

#define STDISP_IS_VERTICAL(STDISP) \
        ((STDISP)->orientation==REGION_ORIENTATION_VERTICAL)

#define STDISP_GROWS_L_TO_R(STDISP) (STDISP_IS_HORIZONTAL(STDISP) && \
    ((STDISP)->corner==MPLEX_STDISP_TL ||                            \
     (STDISP)->corner==MPLEX_STDISP_BL))

#define STDISP_GROWS_R_TO_L(STDISP) (STDISP_IS_HORIZONTAL(STDISP) && \
    ((STDISP)->corner==MPLEX_STDISP_TR ||                            \
     (STDISP)->corner==MPLEX_STDISP_BR))

#define STDISP_GROWS_T_TO_B(STDISP) (STDISP_IS_VERTICAL(STDISP) && \
    ((STDISP)->corner==MPLEX_STDISP_TL ||                          \
     (STDISP)->corner==MPLEX_STDISP_TR))

#define STDISP_GROWS_B_TO_T(STDISP) (STDISP_IS_VERTICAL(STDISP) && \
    ((STDISP)->corner==MPLEX_STDISP_BL ||                          \
     (STDISP)->corner==MPLEX_STDISP_BR))

#define GEOM(S) (((WSplit*)S)->geom)

#define IMPLIES(X, Y) (!(X) || (Y))


static int other_dir(int dir)
{
    return (dir==SPLIT_VERTICAL ? SPLIT_HORIZONTAL : SPLIT_VERTICAL);
}


int stdisp_recommended_w(WSplitST *stdisp)
{
    if(stdisp->regnode.reg==NULL)
        return CF_STDISP_MIN_SZ;
    
    if(stdisp->fullsize && stdisp->orientation==REGION_ORIENTATION_HORIZONTAL){
        WTiling *ws=REGION_MANAGER_CHK(stdisp->regnode.reg, WTiling);
        assert(ws!=NULL);
        return REGION_GEOM(ws).w;
    }
    
    return MAXOF(CF_STDISP_MIN_SZ, region_min_w(stdisp->regnode.reg));
}


int stdisp_recommended_h(WSplitST *stdisp)
{
    if(stdisp->regnode.reg==NULL)
        return CF_STDISP_MIN_SZ;
    
    if(stdisp->fullsize && stdisp->orientation==REGION_ORIENTATION_VERTICAL){
        WTiling *ws=REGION_MANAGER_CHK(stdisp->regnode.reg, WTiling);
        assert(ws!=NULL);
        return REGION_GEOM(ws).h;
    }

    return MAXOF(CF_STDISP_MIN_SZ, region_min_h(stdisp->regnode.reg));
}


static bool stdisp_dir_ok(WSplitSplit *p, WSplitST *stdisp)
{
    assert(p->tl==(WSplit*)stdisp || p->br==(WSplit*)stdisp);
    
    return (IMPLIES(STDISP_IS_HORIZONTAL(stdisp), p->dir==SPLIT_VERTICAL) &&
            IMPLIES(STDISP_IS_VERTICAL(stdisp), p->dir==SPLIT_HORIZONTAL));
}


/*}}}*/


/*{{{ New rotation and flipping primitives */


static void replace(WSplitSplit *a, WSplitSplit *p)
{
    if(((WSplit*)a)->parent!=NULL)
        splitinner_replace(((WSplit*)a)->parent, (WSplit*)a, (WSplit*)p);
    else
        splittree_changeroot((WSplit*)a, (WSplit*)p);
}


/* Yes, it is overparametrised */
static void rotate_right(WSplitSplit *a, WSplitSplit *p, WSplit *y)
{
    assert(a->tl==(WSplit*)p && p->tl==y);
    
    /* Right rotation:
     *        a             p
     *      /  \          /   \
     *     p    x   =>   y     a
     *   /   \               /   \
     *  y     ?             ?     x
     */
    
    a->tl=p->br;
    a->tl->parent=(WSplitInner*)a;
    replace(a, p);
    p->br=(WSplit*)a;
    ((WSplit*)a)->parent=(WSplitInner*)p;
}


static void rot_rs_rotate_right(WSplitSplit *a, WSplitSplit *p, WSplit *y)
{
    WRectangle xg, yg, pg, ag, qg;
    WSplit *x=a->br, *q=p->br;
    
    assert(a->dir==other_dir(p->dir));

    qg=GEOM(q);
    xg=GEOM(x);
    yg=GEOM(y);
    pg=GEOM(p);
    ag=GEOM(a);
    
    if(a->dir==SPLIT_HORIZONTAL){
        /* yyxx    yyyy
         * ??xx => ??xx
         * ??xx    ??xx
         */
        yg.w=ag.w;
        pg.w=ag.w;
        xg.h=qg.h;
        ag.h=qg.h;
        xg.y=qg.y;
        ag.y=qg.y;
    }else{
        /* y??    y??
         * y??    y??
         * xxx => yxx
         * xxx    yxx
         */
        yg.h=ag.h;
        pg.h=ag.h;
        xg.w=qg.w;
        ag.w=qg.w;
        xg.x=qg.x;
        ag.x=qg.x;
    }
    
    rotate_right(a, p, y);
    
    GEOM(p)=pg;
    GEOM(a)=ag;
    
    split_do_resize(x, &xg, PRIMN_TL, PRIMN_TL, FALSE);
    split_do_resize(y, &yg, PRIMN_BR, PRIMN_BR, FALSE);
}



static void rotate_left(WSplitSplit *a, WSplitSplit *p, WSplit *y)
{
    assert(a->br==(WSplit*)p && p->br==y);
    
    /* Left rotation:
     *     a                  p
     *   /   \              /   \
     *  x     p     =>     a     y
     *      /   \        /  \
     *     ?     y      x    ?
     */
    
    a->br=p->tl;
    a->br->parent=(WSplitInner*)a;
    replace(a, p);
    p->tl=(WSplit*)a;
    ((WSplit*)a)->parent=(WSplitInner*)p;
}


static void rot_rs_rotate_left(WSplitSplit *a, WSplitSplit *p, WSplit *y)
{
    WRectangle xg, yg, pg, ag, qg;
    WSplit *x=a->tl, *q=p->tl;
    
    assert(a->dir==other_dir(p->dir));

    qg=GEOM(q);
    xg=GEOM(x);
    yg=GEOM(y);
    pg=GEOM(p);
    ag=GEOM(a);
    
    if(a->dir==SPLIT_HORIZONTAL){
        /* xx??    xx??
         * xx?? => xx??
         * xxyy    yyyy
         */
        yg.w=ag.w;
        yg.x=ag.x;
        pg.w=ag.w;
        pg.x=ag.x;
        xg.h=qg.h;
        ag.h=qg.h;
    }else{
        /* xxx    xxy
         * xxx    xxy
         * ??y => ??y
         * ??y    ??y
         */
        yg.h=ag.h;
        yg.y=ag.y;
        pg.h=ag.h;
        pg.y=ag.y;
        xg.w=qg.w;
        ag.w=qg.w;
    }

    rotate_left(a, p, y);
    
    GEOM(p)=pg;
    GEOM(a)=ag;
    
    split_do_resize(x, &xg, PRIMN_BR, PRIMN_BR, FALSE);
    split_do_resize(y, &yg, PRIMN_TL, PRIMN_TL, FALSE);
}



static void flip_right(WSplitSplit *a, WSplitSplit *p)
{
    assert(a->tl==(WSplit*)p);
    
    /* Right flip:
     *        a               p 
     *      /   \           /   \
     *     p     x   =>    a     y
     *   /  \            /  \
     *  ?    y          ?    x
     */

    a->tl=p->tl;
    a->tl->parent=(WSplitInner*)a;
    replace(a, p);
    p->tl=(WSplit*)a;
    ((WSplit*)a)->parent=(WSplitInner*)p;
}


static void rot_rs_flip_right(WSplitSplit *a, WSplitSplit *p)
{
    WRectangle xg, yg, pg, ag, qg;
    WSplit *x=a->br, *q=p->tl, *y=p->br;
    
    assert(a->dir==other_dir(p->dir));

    qg=GEOM(q);
    xg=GEOM(x);
    yg=GEOM(y);
    pg=GEOM(p);
    ag=GEOM(a);
    
    if(a->dir==SPLIT_HORIZONTAL){
        /* ??xx    ??xx
         * ??xx => ??xx
         * yyxx    yyyy
         */
        yg.w=ag.w;
        pg.w=ag.w;
        ag.h=qg.h;
        xg.h=qg.h;
    }else{
        /* ??y    ??y
         * ??y    ??y
         * xxx => xxy
         * xxx    xxy
         */
        yg.h=ag.h;
        pg.h=ag.h;
        ag.w=qg.w;
        xg.w=qg.w;
    }
    
    flip_right(a, p);
    
    GEOM(p)=pg;
    GEOM(a)=ag;
    
    split_do_resize(x, &xg, PRIMN_BR, PRIMN_BR, FALSE);
    split_do_resize(y, &yg, PRIMN_BR, PRIMN_BR, FALSE);
}


static void flip_left(WSplitSplit *a, WSplitSplit *p)
{
    assert(a->br==(WSplit*)p);
    
    /* Left flip:
     *     a               p 
     *   /   \           /   \
     *  x     p    =>   y     a
     *      /  \            /  \
     *     y    ?          x    ?
     */
    
    a->br=p->br;
    a->br->parent=(WSplitInner*)a;
    replace(a, p);
    p->br=(WSplit*)a;
    ((WSplit*)a)->parent=(WSplitInner*)p;
}


static void rot_rs_flip_left(WSplitSplit *a, WSplitSplit *p)
{
    WRectangle xg, yg, pg, ag, qg;
    WSplit *x=a->tl, *q=p->br, *y=p->tl;
    
    assert(a->dir==other_dir(p->dir));

    qg=GEOM(q);
    xg=GEOM(x);
    yg=GEOM(y);
    pg=GEOM(p);
    ag=GEOM(a);
    
    if(a->dir==SPLIT_HORIZONTAL){
        /* xxyy    yyyy
         * xx?? => xx??
         * xx??    xx??
         */
        yg.x=ag.x;
        yg.w=ag.w;
        pg.x=ag.x;
        pg.w=ag.w;
        xg.h=qg.h;
        xg.y=qg.y;
        ag.h=qg.h;
        ag.y=qg.y;
    }else{
        /* xxx    yxx
         * xxx    yxx
         * y?? => y??
         * y??    y??
         */
        yg.y=ag.y;
        yg.h=ag.h;
        pg.y=ag.y;
        pg.h=ag.h;
        xg.w=qg.w;
        xg.x=qg.x;
        ag.w=qg.w;
        ag.x=qg.x;
    }
    
    flip_left(a, p);
    
    GEOM(p)=pg;
    GEOM(a)=ag;
    
    split_do_resize(x, &xg, PRIMN_TL, PRIMN_TL, FALSE);
    split_do_resize(y, &yg, PRIMN_TL, PRIMN_TL, FALSE);
}


static void rot_para_right(WSplitSplit *a, WSplitSplit *p,
                           WSplit *y)
{
    rotate_right(a, p, y);
    if(a->dir==SPLIT_VERTICAL){
        GEOM(p).y=GEOM(a).y;
        GEOM(p).h=GEOM(a).h;
        GEOM(a).y=GEOM(a->tl).y;
        GEOM(a).h=GEOM(a->br).y+GEOM(a->br).h-GEOM(a).y;
    }else{
        GEOM(p).x=GEOM(a).x;
        GEOM(p).w=GEOM(a).w;
        GEOM(a).x=GEOM(a->tl).x;
        GEOM(a).w=GEOM(a->br).x+GEOM(a->br).w-GEOM(a).x;
    }
}


static void rot_para_left(WSplitSplit *a, WSplitSplit *p,
                          WSplit *y)
{
    rotate_left(a, p, y);
    if(a->dir==SPLIT_VERTICAL){
        GEOM(p).y=GEOM(a).y;
        GEOM(p).h=GEOM(a).h;
        GEOM(a).h=GEOM(a->br).y+GEOM(a->br).h-GEOM(a).y;
    }else{
        GEOM(p).x=GEOM(a).x;
        GEOM(p).w=GEOM(a).w;
        GEOM(a).w=GEOM(a->br).x+GEOM(a->br).w-GEOM(a).x;
    }
}


/*}}}*/


/*{{{ Sink */


static bool do_try_sink_stdisp_orth(WSplitSplit *p, WSplitST *stdisp, 
                                    WSplitSplit *other, bool force)
{
    bool doit=force;
    
    assert(p->dir==other_dir(other->dir));
    assert(stdisp_dir_ok(p, stdisp));
    
    if(STDISP_GROWS_T_TO_B(stdisp) || STDISP_GROWS_L_TO_R(stdisp)){
        if(STDISP_GROWS_L_TO_R(stdisp)){
            assert(other->dir==SPLIT_HORIZONTAL);
            if(other->tl->geom.w>=stdisp_recommended_w(stdisp))
                doit=TRUE;
        }else{ /* STDISP_GROWS_T_TO_B */
            assert(other->dir==SPLIT_VERTICAL);
            if(other->tl->geom.h>=stdisp_recommended_h(stdisp))
                doit=TRUE;
        }
        
        if(doit){
            if(p->br==(WSplit*)stdisp)
                rot_rs_flip_right(p, other);
            else /* p->tl==stdisp */
                rot_rs_rotate_left(p, other, other->br);
        }
    }else{ /* STDISP_GROWS_B_TO_T or STDISP_GROW_R_TO_L */
        if(STDISP_GROWS_R_TO_L(stdisp)){
            assert(other->dir==SPLIT_HORIZONTAL);
            if(other->br->geom.w>=stdisp_recommended_w(stdisp))
                doit=TRUE;
        }else{ /* STDISP_GROWS_B_TO_T */
            assert(other->dir==SPLIT_VERTICAL);
            if(other->br->geom.h>=stdisp_recommended_h(stdisp))
                doit=TRUE;
        }
        
        if(doit){
            if(p->tl==(WSplit*)stdisp)
                rot_rs_flip_left(p, other);
            else /* p->br==stdisp */
                rot_rs_rotate_right(p, other, other->tl);
        }
    }
    
    return doit;
}


static bool do_try_sink_stdisp_para(WSplitSplit *p, WSplitST *stdisp, 
                                    WSplitSplit *other, bool force)
{
    if(!force){
        if(STDISP_IS_HORIZONTAL(stdisp)){
            if(stdisp_recommended_w(stdisp)>=GEOM(p).w)
                return FALSE;
        }else{
            if(stdisp_recommended_h(stdisp)>=GEOM(p).h)
                return FALSE;
        }
    }
    
    if(p->tl==(WSplit*)stdisp)
        rot_para_left(p, other, other->br);
    else
        rot_para_right(p, other, other->tl);
    
    return TRUE;
}


bool split_try_sink_stdisp(WSplitSplit *node, bool iterate, bool force)
{
    bool didsomething=FALSE;
    bool more=TRUE;
    
    /*assert(OBJ_IS_EXACTLY(node, WSplitSplit));*/
    
    while(more){
        WSplit *tl=node->tl;
        WSplit *br=node->br;
        WSplitSplit *other=NULL;
        WSplitST *st;
        
        if(OBJ_IS(tl, WSplitST)){
            st=(WSplitST*)tl;
            other=OBJ_CAST(br, WSplitSplit);
        }else if(OBJ_IS(br, WSplitST)){
            st=(WSplitST*)br;
            other=OBJ_CAST(tl, WSplitSplit);
        }else{
            break;
        }
        
        if(other==NULL)
            break;
        
        if(!stdisp_dir_ok(node, st))
            break;
        
        if(other->dir==other_dir(node->dir)){
            if(!do_try_sink_stdisp_orth(node, st, other, force))
                break;
        }else /*if(br->dir==node->dir)*/{
            if(!do_try_sink_stdisp_para(node, st, other, force))
                break;
        }
        didsomething=TRUE;
        more=iterate;
    }
    
    return didsomething;
}


/*}}}*/


/*{{{ Unsink */


static bool do_try_unsink_stdisp_orth(WSplitSplit *a, WSplitSplit *p,
                                      WSplitST *stdisp, bool force)
{
    bool doit=force;
    
    assert(p->dir==other_dir(a->dir));
    assert(stdisp_dir_ok(p, stdisp));
    
    if(STDISP_GROWS_T_TO_B(stdisp) || STDISP_GROWS_L_TO_R(stdisp)){
        if(STDISP_GROWS_L_TO_R(stdisp)){
            assert(a->dir==SPLIT_HORIZONTAL);
            if(GEOM(stdisp).w<stdisp_recommended_w(stdisp))
                doit=TRUE;
        }else{ /* STDISP_GROWS_T_TO_B */
            assert(a->dir==SPLIT_VERTICAL);
            if(GEOM(stdisp).h<stdisp_recommended_h(stdisp))
                doit=TRUE;
        }
        
        if(doit){
            if((WSplit*)p==a->tl){
                if((WSplit*)stdisp==p->br)
                    rot_rs_flip_right(a, p);
                else /*stdisp==p->tl*/
                    rot_rs_rotate_right(a, p, (WSplit*)stdisp);
            }else{ /*p==a->br*/
#if 0
                /* abnormal cases. */
                warn(TR("Status display in bad split configuration."));
                return FALSE;
#else                
                if((WSplit*)stdisp==p->br)
                    rot_rs_rotate_left(a, p, (WSplit*)stdisp);
                else /*stdisp==p->tl*/
                    rot_rs_flip_left(a, p);
#endif
            }
        }
    }else{ /*STDISP_GROWS_B_TO_T || STDISP_GROWS_R_TO_L*/
        if(STDISP_GROWS_R_TO_L(stdisp)){
            assert(a->dir==SPLIT_HORIZONTAL);
            if(GEOM(stdisp).w<stdisp_recommended_w(stdisp))
                doit=TRUE;
        }else{ /* STDISP_GROWS_B_TO_T */
            assert(a->dir==SPLIT_VERTICAL);
            if(GEOM(stdisp).h<stdisp_recommended_h(stdisp))
                doit=TRUE;
        }
        
        if(doit){
            if((WSplit*)p==a->tl){
#if 0
                /* abnormal cases. */
                warn(TR("Status display in bad split configuration."));
                return FALSE;
#else                
                if((WSplit*)stdisp==p->br)
                    rot_rs_flip_right(a, p);
                else /*stdisp==p->tl*/
                    rot_rs_rotate_right(a, p, (WSplit*)stdisp);
#endif                
            }else{ /*p==a->br*/
                if((WSplit*)stdisp==p->br)
                    rot_rs_rotate_left(a, p, (WSplit*)stdisp);
                else /*stdisp==p->tl*/
                    rot_rs_flip_left(a, p);
            }
        }
    }
    
    return doit;
}


static bool do_try_unsink_stdisp_para(WSplitSplit *a, WSplitSplit *p,
                                      WSplitST *stdisp,  bool force)
{
    if(!force){
        if(STDISP_IS_HORIZONTAL(stdisp)){
            if(stdisp_recommended_w(stdisp)<=GEOM(p).w)
                return FALSE;
        }else{
            if(stdisp_recommended_h(stdisp)<=GEOM(p).h)
                return FALSE;
        }
    }
    
    
    if(a->tl==(WSplit*)p && p->tl==(WSplit*)stdisp){
        rot_para_right(a, p, (WSplit*)stdisp);
    }else if(a->br==(WSplit*)p && p->br==(WSplit*)stdisp){
        rot_para_left(a, p, (WSplit*)stdisp);
    }else{
        warn(TR("Status display badly located in split tree."));
        return FALSE;
    }
    
    return TRUE;
}


bool split_try_unsink_stdisp(WSplitSplit *node, bool iterate, bool force)
{
    bool didsomething=FALSE;
    bool more=TRUE;
    
    /*assert(OBJ_IS_EXACTLY(node, WSplitSplit));*/

    while(more){
        WSplitSplit *p=OBJ_CAST(((WSplit*)node)->parent, WSplitSplit);
        WSplit *tl=node->tl;
        WSplit *br=node->br;
        WSplitST *st;

        if(p==NULL)
            break;
        
        if(OBJ_IS(tl, WSplitST))
            st=(WSplitST*)tl;
        else if(OBJ_IS(br, WSplitST))
            st=(WSplitST*)br;
        else
            break;
        
        if(!stdisp_dir_ok(node, st))
            break;
        
        if(p->dir==other_dir(node->dir)){
            if(!do_try_unsink_stdisp_orth(p, node, st, force))
                break;
        }else /*if(p->dir==node->dir)*/{
            if(!do_try_unsink_stdisp_para(p, node, st, force))
                break;
        }
        
        didsomething=TRUE;
        more=iterate;
    }
    
    return didsomething;
}


/*}}}*/


/*{{{ Sink or unsink */


bool split_regularise_stdisp(WSplitST *stdisp)
{
    WSplitSplit *node=OBJ_CAST(((WSplit*)stdisp)->parent, WSplitSplit);
    
    if(node==NULL)
        return FALSE;
    
    if(STDISP_IS_HORIZONTAL(stdisp)){
        if(GEOM(stdisp).w<stdisp_recommended_w(stdisp))
            return split_try_unsink_stdisp(node, TRUE, FALSE);
        else if(GEOM(stdisp).w>stdisp_recommended_w(stdisp))
            return split_try_sink_stdisp(node, TRUE, FALSE);
    }else{
        if(GEOM(stdisp).h<stdisp_recommended_h(stdisp))
            return split_try_unsink_stdisp(node, TRUE, FALSE);
        else if(GEOM(stdisp).h>stdisp_recommended_h(stdisp))
            return split_try_sink_stdisp(node, TRUE, FALSE);
    }
    
    return FALSE;
}


/*}}}*/

