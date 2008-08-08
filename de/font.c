/*
 * ion/de/font.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2008. 
 *
 * See the included file LICENSE for details.
 */

#include <string.h>

#include <libtu/objp.h>
#include <ioncore/common.h>
#include "font.h"
#include "fontset.h"
#include "brush.h"
#include "precompose.h"


/*{{{ UTF-8 processing */


#define UTF_DATA 0x3F
#define UTF_2_DATA 0x1F
#define UTF_3_DATA 0x0F
#define UTF_1 0x80
#define UTF_2 0xC0
#define UTF_3 0xE0
#define UTF_4 0xF0
#define UTF_5 0xF8
#define UTF_6 0xFC

static void toucs(const char *str_, int len, XChar2b **str16, int *len16)
{
    int i=0;
    const uchar *str=(const uchar*)str_;
    wchar_t prev=0;
    
    *str16=ALLOC_N(XChar2b, len);
    *len16=0;
    
    while(i<len){
        wchar_t ch=0;
        
        if((str[i] & UTF_3) == UTF_3){
             if(i+2>=len)
                 break;
             ch=((str[i] & UTF_3_DATA) << 12) 
                 | ((str[i+1] & UTF_DATA) << 6)
                 | (str[i+2] & UTF_DATA);
            i+=3;
        }else if((str[i] & UTF_2) == UTF_2){
            if(i+1>=len)
                break;
            ch = ((str[i] & UTF_2_DATA) << 6) | (str[i+1] & UTF_DATA);
            i+=2;
        }else if(str[i] < UTF_1){
            ch = str[i];
            i++;
        }else{
            ch='?';
            i++;
        }
        
        if(*len16>0){
            wchar_t precomp=do_precomposition(prev, ch);
            if(precomp!=-1){
                (*len16)--;
                ch=precomp;
            }
        }
            
        (*str16)[*len16].byte2=ch&0xff;
        (*str16)[*len16].byte1=(ch>>8)&0xff;
        (*len16)++;
        prev=ch;
    }
}


/*}}}*/


/*{{{ Load/free */


static DEFont *fonts=NULL;


static bool iso10646_font(const char *fontname)
{
    const char *iso;
    
    if(strchr(fontname, ',')!=NULL)
        return FALSE; /* fontset */
        
    iso=strstr(fontname, "iso10646-1");
    return (iso!=NULL && iso[10]=='\0');
}


DEFont *de_load_font(const char *fontname)
{
    DEFont *fnt;
    XFontSet fontset=NULL;
    XFontStruct *fontstruct=NULL;
    
    assert(fontname!=NULL);
    
    /* There shouldn't be that many fonts... */
    for(fnt=fonts; fnt!=NULL; fnt=fnt->next){
        if(strcmp(fnt->pattern, fontname)==0){
            fnt->refcount++;
            return fnt;
        }
    }
    
    if(ioncore_g.use_mb && !(ioncore_g.enc_utf8 && iso10646_font(fontname))){
        fontset=de_create_font_set(fontname);
        if(fontset!=NULL){
            if(XContextDependentDrawing(fontset)){
                warn(TR("Fontset for font pattern '%s' implements context "
                        "dependent drawing, which is unsupported. Expect "
                        "clutter."), fontname);
            }
        }
    }else{
        fontstruct=XLoadQueryFont(ioncore_g.dpy, fontname);
    }
    
    if(fontstruct==NULL && fontset==NULL){
        if(strcmp(fontname, CF_FALLBACK_FONT_NAME)!=0){
            DEFont *fnt;
            warn(TR("Could not load font \"%s\", trying \"%s\""),
                 fontname, CF_FALLBACK_FONT_NAME);
            fnt=de_load_font(CF_FALLBACK_FONT_NAME);
            if(fnt==NULL)
                warn(TR("Failed to load fallback font."));
            return fnt;
        }
        return NULL;
    }
    
    fnt=ALLOC(DEFont);
    
    if(fnt==NULL)
        return NULL;
    
    fnt->fontset=fontset;
    fnt->fontstruct=fontstruct;
    fnt->pattern=scopy(fontname);
    fnt->next=NULL;
    fnt->prev=NULL;
    fnt->refcount=1;
    
    LINK_ITEM(fonts, fnt, next, prev);
    
    return fnt;
}


bool de_set_font_for_style(DEStyle *style, DEFont *font)
{
    if(style->font!=NULL)
        de_free_font(style->font);
    
    style->font=font;
    font->refcount++;
    
    if(style->font->fontstruct!=NULL){
        XSetFont(ioncore_g.dpy, style->normal_gc, 
                 style->font->fontstruct->fid);
    }

    return TRUE;
}


bool de_load_font_for_style(DEStyle *style, const char *fontname)
{
    if(style->font!=NULL)
        de_free_font(style->font);
    
    style->font=de_load_font(fontname);

    if(style->font==NULL)
        return FALSE;
    
    if(style->font->fontstruct!=NULL){
        XSetFont(ioncore_g.dpy, style->normal_gc, 
                 style->font->fontstruct->fid);
    }
    
    return TRUE;
}


void de_free_font(DEFont *font)
{
    if(--font->refcount!=0)
        return;
    
    if(font->fontset!=NULL)
        XFreeFontSet(ioncore_g.dpy, font->fontset);
    if(font->fontstruct!=NULL)
        XFreeFont(ioncore_g.dpy, font->fontstruct);
    if(font->pattern!=NULL)
        free(font->pattern);
    
    UNLINK_ITEM(fonts, font, next, prev);
    free(font);
}


/*}}}*/


/*{{{ Lengths */


void debrush_get_font_extents(DEBrush *brush, GrFontExtents *fnte)
{
    if(brush->d->font==NULL){
        DE_RESET_FONT_EXTENTS(fnte);
        return;
    }
    
    defont_get_font_extents(brush->d->font, fnte);
}


void defont_get_font_extents(DEFont *font, GrFontExtents *fnte)
{
    if(font->fontset!=NULL){
        XFontSetExtents *ext=XExtentsOfFontSet(font->fontset);
        if(ext==NULL)
            goto fail;
        fnte->max_height=ext->max_logical_extent.height;
        fnte->max_width=ext->max_logical_extent.width;
        fnte->baseline=-ext->max_logical_extent.y;
        return;
    }else if(font->fontstruct!=NULL){
        XFontStruct *fnt=font->fontstruct;
        fnte->max_height=fnt->ascent+fnt->descent;
        fnte->max_width=fnt->max_bounds.width;
        fnte->baseline=fnt->ascent;
        return;
    }
    
fail:
    DE_RESET_FONT_EXTENTS(fnte);
}


uint debrush_get_text_width(DEBrush *brush, const char *text, uint len)
{
    if(brush->d->font==NULL || text==NULL || len==0)
        return 0;
    
    return defont_get_text_width(brush->d->font, text, len);
}


uint defont_get_text_width(DEFont *font, const char *text, uint len)
{
    if(font->fontset!=NULL){
        XRectangle lext;
#ifdef CF_DE_USE_XUTF8
        if(ioncore_g.enc_utf8)
            Xutf8TextExtents(font->fontset, text, len, NULL, &lext);
        else
#endif
            XmbTextExtents(font->fontset, text, len, NULL, &lext);
        return lext.width;
    }else if(font->fontstruct!=NULL){
        if(ioncore_g.enc_utf8){
            XChar2b *str16; int len16=0;
            uint res;
            
            toucs(text, len, &str16, &len16);
            
            res=XTextWidth16(font->fontstruct, str16, len16);
            
            free(str16);
            
            return res;
        }else{
            return XTextWidth(font->fontstruct, text, len);
        }
    }else{
        return 0;
    }
}


/*}}}*/


/*{{{ String drawing */


void debrush_do_draw_string_default(DEBrush *brush, int x, int y,
                                    const char *str, int len, bool needfill, 
                                    DEColourGroup *colours)
{
    GC gc=brush->d->normal_gc;

    if(brush->d->font==NULL)
        return;
    
    XSetForeground(ioncore_g.dpy, gc, colours->fg);
    
    if(!needfill){
        if(brush->d->font->fontset!=NULL){
#ifdef CF_DE_USE_XUTF8
            if(ioncore_g.enc_utf8)
                Xutf8DrawString(ioncore_g.dpy, brush->win, 
                                brush->d->font->fontset,
                                gc, x, y, str, len);
            else
#endif
                XmbDrawString(ioncore_g.dpy, brush->win, 
                              brush->d->font->fontset,
                              gc, x, y, str, len);
        }else if(brush->d->font->fontstruct!=NULL){
            if(ioncore_g.enc_utf8){
                XChar2b *str16; int len16=0;
                toucs(str, len, &str16, &len16);
                XDrawString16(ioncore_g.dpy, brush->win, gc, x, y, str16, len16);
                free(str16);
            }else{
                XDrawString(ioncore_g.dpy, brush->win, gc, x, y, str, len);
            }
        }
    }else{
        XSetBackground(ioncore_g.dpy, gc, colours->bg);
        if(brush->d->font->fontset!=NULL){
#ifdef CF_DE_USE_XUTF8
            if(ioncore_g.enc_utf8)
                Xutf8DrawImageString(ioncore_g.dpy, brush->win, 
                                     brush->d->font->fontset,
                                     gc, x, y, str, len);
            else
#endif
                XmbDrawImageString(ioncore_g.dpy, brush->win, 
                                   brush->d->font->fontset,
                                   gc, x, y, str, len);
        }else if(brush->d->font->fontstruct!=NULL){
            if(ioncore_g.enc_utf8){
                XChar2b *str16; int len16=0;
                toucs(str, len, &str16, &len16);
                XDrawImageString16(ioncore_g.dpy, brush->win, gc, x, y, str16, len16);
                free(str16);
            }else{
                XDrawImageString(ioncore_g.dpy, brush->win, gc, x, y, str, len);
            }
        }
    }
}


void debrush_do_draw_string(DEBrush *brush, int x, int y,
                            const char *str, int len, bool needfill, 
                            DEColourGroup *colours)
{
    CALL_DYN(debrush_do_draw_string, brush, (brush, x, y, str, len,
                                             needfill, colours));
}


void debrush_draw_string(DEBrush *brush, int x, int y,
                         const char *str, int len, bool needfill)
{
    DEColourGroup *cg=debrush_get_current_colour_group(brush);
    if(cg!=NULL)
        debrush_do_draw_string(brush, x, y, str, len, needfill, cg);
}


/*}}}*/

