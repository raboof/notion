/*
 * ion/draw.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <wmcore/common.h>
#include <wmcore/draw.h>
#include <wmcore/drawp.h>
#include <wmcore/font.h>


#define BORDER_TOTAL(X) (BORDER_TL_TOTAL(X)+BORDER_BR_TOTAL(X))

/*{{{ Precalc */


static void calc_frame_offs(WGRData *grdata)
{
	const WBorder *frame_border=&(grdata->frame_border);
	const WBorder *tab_border=&(grdata->tab_border);
	int sp=grdata->spacing;
	int isp=grdata->frame_border.ipad;
	
	grdata->bar_h=(FONT_HEIGHT(grdata->tab_font)+
				   BORDER_TL_TOTAL(tab_border)+BORDER_BR_TOTAL(tab_border));
	
#if 0
	grdata->border_off.x=0;
	grdata->border_off.y=0;
	grdata->border_off.w=-sp*2;
	grdata->border_off.h=-sp*2;
#else
	grdata->border_off.x=sp;
	grdata->border_off.y=sp;
	grdata->border_off.w=-2*sp;
	grdata->border_off.h=-2*sp;
#endif

	grdata->bar_off=grdata->border_off; /* except that h isn't used */
	
	if(grdata->bar_inside_frame){
		grdata->bar_off.y+=BORDER_TL_TOTAL(frame_border);
		grdata->bar_off.x+=BORDER_TL_TOTAL(frame_border);
		grdata->bar_off.w-=BORDER_TOTAL(frame_border);
		grdata->tab_spacing=isp;
	}else{
		grdata->border_off.y+=grdata->bar_h+sp;
		grdata->border_off.h-=grdata->bar_h+sp;
		grdata->tab_spacing=sp;
	}
	
	grdata->client_off=grdata->border_off;
	grdata->client_off.x+=BORDER_TL_TOTAL(frame_border);
	grdata->client_off.y+=BORDER_TL_TOTAL(frame_border);
	grdata->client_off.w-=BORDER_TOTAL(frame_border);
	grdata->client_off.h-=BORDER_TOTAL(frame_border);
	
	if(grdata->bar_inside_frame){
		grdata->client_off.y+=grdata->bar_h+isp;
		grdata->client_off.h-=grdata->bar_h+isp;
	}	
	
	/*pgeom("border_off", grdata->border_off);
	pgeom("bar_off", grdata->bar_off);*/
}


void calc_grdata(WScreen *scr)
{
	calc_frame_offs(&(scr->grdata));
}


/*}}}*/


