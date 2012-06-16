/*
 * ion/ioncore/frame-draw.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#include <string.h>

#include <libtu/objp.h>
#include <libtu/minmax.h>
#include <libtu/map.h>

#include "common.h"
#include "frame.h"
#include "framep.h"
#include "frame-draw.h"
#include "strings.h"
#include "activity.h"
#include "names.h"
#include "gr.h"
#include "gr-util.h"


#define BAR_INSIDE_BORDER(FRAME) \
    ((FRAME)->barmode==FRAME_BAR_INSIDE || (FRAME)->barmode==FRAME_BAR_NONE)
#define BAR_EXISTS(FRAME) ((FRAME)->barmode!=FRAME_BAR_NONE)
#define BAR_H(FRAME) (FRAME)->bar_h


/*{{{ Attributes */


GR_DEFATTR(active);
GR_DEFATTR(inactive);
GR_DEFATTR(selected);
GR_DEFATTR(unselected);
GR_DEFATTR(tagged);
GR_DEFATTR(not_tagged);
GR_DEFATTR(dragged);
GR_DEFATTR(not_dragged);
GR_DEFATTR(activity);
GR_DEFATTR(no_activity);
GR_DEFATTR(quasiactive);
GR_DEFATTR(not_quasiactive);


static void ensure_create_attrs()
{
    GR_ALLOCATTR_BEGIN;
    GR_ALLOCATTR(active);
    GR_ALLOCATTR(inactive);
    GR_ALLOCATTR(selected);
    GR_ALLOCATTR(unselected);
    GR_ALLOCATTR(tagged);
    GR_ALLOCATTR(not_tagged);
    GR_ALLOCATTR(dragged);
    GR_ALLOCATTR(not_dragged);
    GR_ALLOCATTR(no_activity);
    GR_ALLOCATTR(activity);
    GR_ALLOCATTR(quasiactive);
    GR_ALLOCATTR(not_quasiactive);
    GR_ALLOCATTR_END;
}
    

void frame_update_attr(WFrame *frame, int i, WRegion *reg)
{
    GrStyleSpec *spec;
    bool selected, tagged, dragged, activity;
    
    if(i>=frame->titles_n){
        /* Might happen when deinitialising */
        return;
    }
    
    ensure_create_attrs();
    
    spec=&frame->titles[i].attr;
    
    selected=(reg==FRAME_CURRENT(frame));
    tagged=(reg!=NULL && reg->flags&REGION_TAGGED);
    dragged=(i==frame->tab_dragged_idx);
    activity=(reg!=NULL && region_is_activity_r(reg));
    
    gr_stylespec_unalloc(spec);
    gr_stylespec_set(spec, selected ? GR_ATTR(selected) : GR_ATTR(unselected));
    gr_stylespec_set(spec, tagged ? GR_ATTR(tagged) : GR_ATTR(not_tagged));
    gr_stylespec_set(spec, dragged ? GR_ATTR(dragged) : GR_ATTR(not_dragged));
    gr_stylespec_set(spec, activity ? GR_ATTR(activity) : GR_ATTR(no_activity));
}


/*}}}*/


/*{{{ (WFrame) dynfun default implementations */


static uint get_spacing(const WFrame *frame)
{
    GrBorderWidths bdw;
    
    if(frame->brush==NULL)
        return 0;
    
    grbrush_get_border_widths(frame->brush, &bdw);
    
    return bdw.spacing;
}


void frame_border_geom(const WFrame *frame, WRectangle *geom)
{
    geom->x=0;
    geom->y=0;
    geom->w=REGION_GEOM(frame).w;
    geom->h=REGION_GEOM(frame).h;
    
    if(!BAR_INSIDE_BORDER(frame) && frame->brush!=NULL){
        geom->y+=frame->bar_h;
        geom->h=maxof(0, geom->h-frame->bar_h);
    }
}


void frame_border_inner_geom(const WFrame *frame, WRectangle *geom)
{
    GrBorderWidths bdw;
    
    frame_border_geom(frame, geom);

    if(frame->brush!=NULL){
        grbrush_get_border_widths(frame->brush, &bdw);

        geom->x+=bdw.left;
        geom->y+=bdw.top;
        geom->w=maxof(0, geom->w-(bdw.left+bdw.right));
        geom->h=maxof(0, geom->h-(bdw.top+bdw.bottom));
    }
}


void frame_bar_geom(const WFrame *frame, WRectangle *geom)
{
    uint off;
    
    if(BAR_INSIDE_BORDER(frame)){
        off=0; /*get_spacing(frame);*/
        frame_border_inner_geom(frame, geom);
    }else{
        off=0;
        geom->x=0;
        geom->y=0;
        geom->w=(frame->barmode==FRAME_BAR_SHAPED
                 ? frame->bar_w
                 : REGION_GEOM(frame).w);
    }
    geom->x+=off;
    geom->y+=off;
    geom->w=maxof(0, geom->w-2*off);
    geom->h=BAR_H(frame);
}


void frame_managed_geom(const WFrame *frame, WRectangle *geom)
{
    uint spacing=get_spacing(frame);
    
    frame_border_inner_geom(frame, geom);
    
    /*
    geom->x+=spacing;
    geom->y+=spacing;
    geom->w-=2*spacing;
    geom->h-=2*spacing;
    */
    
    if(BAR_INSIDE_BORDER(frame) && BAR_EXISTS(frame)){
        geom->y+=frame->bar_h+spacing;
        geom->h-=frame->bar_h+spacing;
    }
    
    geom->w=maxof(geom->w, 0);
    geom->h=maxof(geom->h, 0);
}


void frame_set_shape(WFrame *frame)
{
    WRectangle gs[2];
    int n=0;
    
    if(frame->brush!=NULL){
        if(BAR_EXISTS(frame)){
            frame_bar_geom(frame, gs+n);
            n++;
        }
        frame_border_geom(frame, gs+n);
        n++;
    
        grbrush_set_window_shape(frame->brush, TRUE, n, gs);
    }
}


void frame_clear_shape(WFrame *frame)
{
    grbrush_set_window_shape(frame->brush, TRUE, 0, NULL);
}


#define CF_TAB_MAX_TEXT_X_OFF 10

void frame_get_max_width_and_elastic(WFrame * frame,int bar_w,int *maxw,int *elastic, int *minw);//FIXME

static void frame_shaped_recalc_bar_size(WFrame *frame)
{
    int bar_w=0, textw=0, tmaxw=frame->tab_min_w, tmp=0;
    WLListIterTmp itmp;
    WRegion *sub;
    const char *p;
    GrBorderWidths bdw;
    char *title;
    uint bdtotal;
    int i, m;
    
    if(frame->bar_brush==NULL)
        return;
    
    m=FRAME_MCOUNT(frame);
    
    if(m>0){
        grbrush_get_border_widths(frame->bar_brush, &bdw);
        bdtotal=((m-1)*(bdw.tb_ileft+bdw.tb_iright+bdw.spacing)
                 +bdw.right+bdw.left);

        FRAME_MX_FOR_ALL(sub, frame, itmp){
            p=region_displayname(sub);
            if(p==NULL)
                continue;
            
            textw=grbrush_get_text_width(frame->bar_brush,
                                         p, strlen(p));
            if(textw>tmaxw)
                tmaxw=textw;
        }

        bar_w=frame->bar_max_width_q*REGION_GEOM(frame).w;
        if(bar_w<frame->tab_min_w && 
           REGION_GEOM(frame).w>frame->tab_min_w)
            bar_w=frame->tab_min_w;
        
        tmp=bar_w-bdtotal-m*tmaxw;
        
        if(tmp>0){
            /* No label truncation needed, good. See how much can be padded. */
            tmp/=m*2;
            if(tmp>CF_TAB_MAX_TEXT_X_OFF)
                tmp=CF_TAB_MAX_TEXT_X_OFF;
            bar_w=(tmaxw+tmp*2)*m+bdtotal;
        }else{
            /* Some labels must be truncated */
        }
    }else{
        bar_w=frame->tab_min_w;
        if(bar_w>frame->bar_max_width_q*REGION_GEOM(frame).w)
            bar_w=frame->bar_max_width_q*REGION_GEOM(frame).w;
    }

    if(frame->bar_w!=bar_w){
        frame->bar_w=bar_w;
        frame_set_shape(frame);
    }
}


static int init_title(WFrame *frame, int i)
{
    int textw;
    
    if(frame->titles[i].text!=NULL){
        free(frame->titles[i].text);
        frame->titles[i].text=NULL;
    }
    
    textw=frame_nth_tab_iw((WFrame*)frame, i);
    frame->titles[i].iw=textw;
    return textw;
}


/* Proportional tabs algorithm:
  * Sort tabs by text sizes.
  * From smallest to largest do:
    * Compare current tab width with remaining width (incl. current)
      divided by no. of remaining tabs (incl. current).
      * If larger or equal, then set maximum width to the number computed
	above and set elastic space to zero (resp. to remain after division).
	(Return.)
      * If smaller, subctract current width from remaining width and
	subtract one from no. of remaining tabs. If this is the last
	tab, then set additional padding to remaining width divided
	by total number of tabs.

  Do not use w for shaped frames.
*/
void frame_get_max_width_and_elastic(WFrame * frame,int bar_w,int *maxw,int *elastic, int *minw){
    /* Dummy implementation O(n^2), instead of O(n*log(n)) */
    int textw=0,tmp,tmaxw,tminw=frame->propor_tab_min_w;
    WLListIterTmp itmp;
    WRegion *sub;
    const char *displayname;
    GrBorderWidths bdw;
    uint bdtotal,curw,nextw;
    int i, m, n;

    m=FRAME_MCOUNT(frame);

    *minw=0;
    
/*
    if(frame->bar_brush==NULL)
        *elastic=0;
        *maxw=-1;
fprintf(stderr,"ZERR\n");
        return;
*/
    if(m>0){
	grbrush_get_border_widths(frame->bar_brush, &bdw);
	bdtotal=((m-1)*(bdw.tb_ileft+bdw.tb_iright+bdw.spacing)
		 +bdw.right+bdw.left);
	tmp = bar_w - bdtotal;
#ifdef DEBUG
	fprintf(stderr,"TMPST:%i barw:%i bdt:%i\n",tmp,bar_w,bdtotal);
#endif
       
	curw=0;
	n=m;
	
	while (n>0) {
	    nextw=(uint)-1;/*FIXME: MAXINT*/
	    if ((signed)curw*n >= tmp) {
		/*Remainig tabs are too large => equal width.*/
		*maxw=tmp/n;
		*elastic=tmp-(*maxw)*n;
#ifdef DEBUG
		fprintf(stderr,"TRUNC maxw:%i elastic:%i tmp:%i n:%i\n",*maxw,*elastic,tmp,n);
#endif
		return;
	    }
	    FRAME_MX_FOR_ALL(sub, frame, itmp){
		displayname=region_displayname(sub);

		if(displayname==NULL)
		    textw=0;
		else
		    textw=grbrush_get_text_width(frame->bar_brush,
					     displayname, strlen(displayname));
		if (textw<tminw)
		    textw=tminw;
		if((unsigned)textw == curw){
#ifdef DEBUG
		    fprintf(stderr,"TW:%i (%s)\n",textw,displayname);
#endif
		    tmp-=textw;
		    n--;
		} else if((unsigned)textw>curw){
		    if ((unsigned)textw<nextw)
			nextw=textw;
		}
	    }
	    curw = nextw;
	}
#ifdef DEBUG
	fprintf(stderr,"TMP elastic:%i\n",tmp);
#endif

	n=0;
	curw=0;
	/*We have some padding left. Try to enlarge small tabs*/
	while (n<m) {
	    nextw=(uint)-1;/*FIXME: MAXINT*/
	    FRAME_MX_FOR_ALL(sub, frame, itmp){
		displayname=region_displayname(sub);
		if(displayname==NULL)
		    continue;
		
		textw=grbrush_get_text_width(frame->bar_brush,
					     displayname, strlen(displayname));
		if (textw<tminw)
		    textw=tminw;
		if((unsigned)textw == curw){
		    n++;
		} else if((unsigned)textw>curw){
		    if ((unsigned)textw<nextw)
			nextw=textw;
		}
	    }
	    if (nextw>(unsigned)frame->float_tab_min_w)
		nextw=frame->float_tab_min_w;
#ifdef DEBUG
	    fprintf(stderr,"TMP --- tmp:%i n:%i curw:%i nextw:%i min:%i\n",tmp,n,curw,nextw,*minw);
#endif
	    if (n*(nextw-curw)<(unsigned)tmp) {
		/*we can extend small tabs to 'nextw'*/
		*minw=nextw;
		tmp-=n*(nextw-curw);
#ifdef DEBUG
		fprintf(stderr,"TMPSUBGO tmp:%i n:%i curw:%i nextw:%i min:%i\n",tmp,n,curw,nextw,*minw);
#endif
	    } else {
		/*we can extend small tabs only to 'curw+tmp/n'*/
		*minw+=tmp/n;
		tmp-=(*minw-curw)*n;
#ifdef DEBUG
		fprintf(stderr,"TMPSUBBRK tmp:%i n:%i curw:%i nextw:%i min:%i\n",tmp,n,curw,nextw,*minw);
#endif
		break;
	    }
	    if (nextw==(unsigned)frame->float_tab_min_w)
		break;
	    curw=nextw;
	}
	
#ifdef DEBUG
	fprintf(stderr,"TMP elastic:%i min:%i\n",tmp,*minw);
#endif
	*elastic=tmp;
    } else {
	*elastic=0;
    }
    *maxw=-1;
}

void frame_recalc_bar(WFrame *frame)
{
    int textw, i;
    WLListIterTmp tmp;
    WRegion *sub;
    char *title;

    if(frame->bar_brush==NULL || frame->titles==NULL)
        return;
    
    if(frame->barmode==FRAME_BAR_SHAPED)
        frame_shaped_recalc_bar_size(frame);
    
    i=0;
    
    if(FRAME_MCOUNT(frame)==0){
        textw=init_title(frame, i);
        if(textw>0){
            title=grbrush_make_label(frame->bar_brush, TR("<empty frame>"), 
                                     textw);
            frame->titles[i].text=title;
        }
        return;
    }
    
    FRAME_MX_FOR_ALL(sub, frame, tmp){
        textw=init_title(frame, i);
        if(textw>0){
            if(frame->flags&FRAME_SHOW_NUMBERS){
                char *s=NULL;
                libtu_asprintf(&s, "[%d]", i+1);
                if(s!=NULL){
                    title=grbrush_make_label(frame->bar_brush, s, textw);
                    free(s);
                }else{
                    title=NULL;
                }
            }else{
                title=region_make_label(sub, textw, frame->bar_brush);
            }
            frame->titles[i].text=title;
        }
        i++;
    }
}


void frame_draw_bar(const WFrame *frame, bool complete)
{
    WRectangle geom;
    
    if(frame->bar_brush==NULL
       || !BAR_EXISTS(frame)
       || frame->titles==NULL){
        return;
    }
    
    frame_bar_geom(frame, &geom);

    grbrush_begin(frame->bar_brush, &geom, GRBRUSH_AMEND);
    
    grbrush_init_attr(frame->bar_brush, &frame->baseattr);
    
    grbrush_draw_textboxes(frame->bar_brush, &geom, frame->titles_n, 
                           frame->titles, complete);
    
    grbrush_end(frame->bar_brush);
}

void frame_draw(const WFrame *frame, bool complete)
{
    WRectangle geom;
    
    if(frame->brush==NULL)
        return;
        
    frame_border_geom(frame, &geom);
    
    grbrush_begin(frame->brush, &geom, (complete ? 0 : GRBRUSH_NO_CLEAR_OK));
    
    grbrush_init_attr(frame->brush, &frame->baseattr);
    
    grbrush_draw_border(frame->brush, &geom);
    
    frame_draw_bar(frame, TRUE);
    
    grbrush_end(frame->brush);
}


void frame_brushes_updated(WFrame *frame)
{
    WFrameBarMode barmode=FRAME_BAR_INSIDE;
    ExtlTab tab;
    char *s;

    if(frame->brush==NULL)
        return;
    
    if(frame->mode==FRAME_MODE_FLOATING)
        barmode=FRAME_BAR_SHAPED;
    else if(frame->mode==FRAME_MODE_TRANSIENT)
        barmode=FRAME_BAR_NONE;
    else if(frame->mode==FRAME_MODE_TILED_ALT)
        barmode=FRAME_BAR_NONE;
    
    if(grbrush_get_extra(frame->brush, "bar", 's', &s)){
        if(strcmp(s, "inside")==0)
            barmode=FRAME_BAR_INSIDE;
        else if(strcmp(s, "outside")==0)
            barmode=FRAME_BAR_OUTSIDE;
        else if(strcmp(s, "shaped")==0)
            barmode=FRAME_BAR_SHAPED;
        else if(strcmp(s, "none")==0)
            barmode=FRAME_BAR_NONE;
	free(s);
    }
        
    frame->barmode=barmode;
    
    if(barmode==FRAME_BAR_NONE || frame->bar_brush==NULL){
        frame->bar_h=0;
    }else{
        GrBorderWidths bdw;
        GrFontExtents fnte;

        grbrush_get_border_widths(frame->bar_brush, &bdw);
        grbrush_get_font_extents(frame->bar_brush, &fnte);

        frame->bar_h=bdw.top+bdw.bottom+fnte.max_height;
    }
    
    /* shaped mode stuff */
    frame->tab_min_w=100;
    frame->bar_max_width_q=0.95;
    
    if(grbrush_get_extra(frame->brush, "floatframe_tab_min_w",
                         'i', &(frame->tab_min_w))){
        if(frame->tab_min_w<=0)
            frame->tab_min_w=1;
    }
    
    if(grbrush_get_extra(frame->brush, "floatframe_bar_max_w_q", 
                         'd', &(frame->bar_max_width_q))){
        if(frame->bar_max_width_q<=0.0 || frame->bar_max_width_q>1.0)
            frame->bar_max_width_q=1.0;
    }
}


/*}}}*/


/*{{{ Misc. */


void frame_updategr(WFrame *frame)
{
    frame_release_brushes(frame);
    
    frame_initialise_gr(frame);
    
    /* Update children */
    region_updategr_default((WRegion*)frame);
    
    mplex_fit_managed(&frame->mplex);
    frame_recalc_bar(frame);
    frame_set_background(frame, TRUE);
}


StringIntMap frame_tab_styles[]={
    {"tab-frame-tiled", FRAME_MODE_TILED},
    {"tab-frame-tiled-alt", FRAME_MODE_TILED_ALT},
    {"tab-frame-floating", FRAME_MODE_FLOATING},
    {"tab-frame-transient", FRAME_MODE_TRANSIENT},
    END_STRINGINTMAP
};


const char *framemode_get_tab_style(WFrameMode mode)
{
    return stringintmap_key(frame_tab_styles, mode, "tab-frame");
}


const char *framemode_get_style(WFrameMode mode)
{
    const char *p=framemode_get_tab_style(mode);
    assert(p!=NULL);
    return (p+4);
}


void frame_initialise_gr(WFrame *frame)
{
    Window win=frame->mplex.win.win;
    WRootWin *rw=region_rootwin_of((WRegion*)frame);
    const char *style=framemode_get_style(frame->mode);
    const char *tab_style=framemode_get_tab_style(frame->mode);
    
    frame->brush=gr_get_brush(win, rw, style);
    
    if(frame->brush==NULL)
        return;
    
    frame->bar_brush=grbrush_get_slave(frame->brush, rw, tab_style);
    
    if(frame->bar_brush==NULL)
        return;
    
    frame_brushes_updated(frame);
}


void frame_release_brushes(WFrame *frame)
{
    if(frame->bar_brush!=NULL){
        grbrush_release(frame->bar_brush);
        frame->bar_brush=NULL;
    }
    
    if(frame->brush!=NULL){
        grbrush_release(frame->brush);
        frame->brush=NULL;
    }
}


bool frame_set_background(WFrame *frame, bool set_always)
{
    GrTransparency mode=GR_TRANSPARENCY_DEFAULT;
    
    if(FRAME_CURRENT(frame)!=NULL){
        if(OBJ_IS(FRAME_CURRENT(frame), WClientWin)){
            WClientWin *cwin=(WClientWin*)FRAME_CURRENT(frame);
            mode=(cwin->flags&CLIENTWIN_PROP_TRANSPARENT
                  ? GR_TRANSPARENCY_YES : GR_TRANSPARENCY_NO);
        }else if(!OBJ_IS(FRAME_CURRENT(frame), WGroup)){
            mode=GR_TRANSPARENCY_NO;
        }
    }
    
    if(mode!=frame->tr_mode || set_always){
        frame->tr_mode=mode;
        if(frame->brush!=NULL){
            grbrush_enable_transparency(frame->brush, mode);
            window_draw((WWindow*)frame, TRUE);
        }
        return TRUE;
    }
    
    return FALSE;
}


void frame_setup_dragwin_style(WFrame *frame, GrStyleSpec *spec, int tab)
{
    gr_stylespec_append(spec, &frame->baseattr);
    gr_stylespec_append(spec, &frame->titles[tab].attr);
}


/*}}}*/


/*{{{ Activated/inactivated */


void frame_inactivated(WFrame *frame)
{
    ensure_create_attrs();
    
    gr_stylespec_set(&frame->baseattr, GR_ATTR(inactive));
    gr_stylespec_unset(&frame->baseattr, GR_ATTR(active));

    window_draw((WWindow*)frame, FALSE);
}


void frame_activated(WFrame *frame)
{
    ensure_create_attrs();
    
    gr_stylespec_set(&frame->baseattr, GR_ATTR(active));
    gr_stylespec_unset(&frame->baseattr, GR_ATTR(inactive));
    
    window_draw((WWindow*)frame, FALSE);
}


void frame_quasiactivity_change(WFrame *frame)
{
    bool is=(frame->quasiactive_count>0);
    
    ensure_create_attrs();
    
    if(is){
        gr_stylespec_set(&frame->baseattr, GR_ATTR(quasiactive));
        gr_stylespec_unset(&frame->baseattr, GR_ATTR(not_quasiactive));
    }else{
        gr_stylespec_set(&frame->baseattr, GR_ATTR(not_quasiactive));
        gr_stylespec_unset(&frame->baseattr, GR_ATTR(quasiactive));
    }
    
    if(!REGION_IS_ACTIVE(frame))
        window_draw((WWindow*)frame, FALSE);
}


/*}}}*/

