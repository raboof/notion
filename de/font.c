/*
 * notion/de/font.c
 *
 * Copyright (c) the Notion team 2013.
 * Copyright (c) Tuomo Valkonen 1999-2009.
 *
 * See the included file LICENSE for details.
 */

#include <string.h>

#include <libtu/objp.h>
#include <ioncore/common.h>
#include <ioncore/log.h>
#include "font.h"
#ifdef HAVE_X11_XFT
#include <fontconfig/fontconfig.h>
#else
#include "fontset.h"
#endif /* HAVE_X11_XFT */
#include "brush.h"
#include "precompose.h"


/*{{{ UTF-8 processing */


#define UTF_DATA 0x3F
#define UTF_2_DATA 0x1F
#define UTF_3_DATA 0x0F
#define UTF_1 0x80
#define UTF_2 0xC0
#define UTF_3 0xE0

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

const char *de_default_fontname()
{
    if(ioncore_g.use_mb)
        return "-*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*";
    else
        return "fixed";
}

DEFont *de_load_font(const char *fontname)
{
#ifdef HAVE_X11_XFT
    XftFont *font;
#endif
    DEFont *fnt;
    XFontSet fontset=NULL;
    XFontStruct *fontstruct=NULL;
    const char *default_fontname=de_default_fontname();

    assert(fontname!=NULL);

    /* There shouldn't be that many fonts... */
    for(fnt=fonts; fnt!=NULL; fnt=fnt->next){
        if(strcmp(fnt->pattern, fontname)==0){
            fnt->refcount++;
            return fnt;
        }
    }

#ifdef HAVE_X11_XFT
    LOG(DEBUG, FONT, "Loading font %s via XFT", fontname);
    if(strncmp(fontname, "xft:", 4)==0){
        font=XftFontOpenName(ioncore_g.dpy, DefaultScreen(ioncore_g.dpy), fontname+4);
    }else{
        font=XftFontOpenXlfd(ioncore_g.dpy, DefaultScreen(ioncore_g.dpy), fontname);
    }

    if(font==NULL){
        if(strcmp(fontname, default_fontname)!=0){
            warn(TR("Could not load font \"%s\", trying \"%s\""),
                fontname, default_fontname);
            fnt=de_load_font(default_fontname);
            if(fnt==NULL)
                LOG(WARN, FONT, TR("Failed to load fallback font."));
            return fnt;
        }
        return NULL;
    }else{
        FcPatternPrint(font->pattern);
    }
#else /* HAVE_X11_XFT */
    if(ioncore_g.use_mb && !(ioncore_g.enc_utf8 && iso10646_font(fontname))){
        LOG(DEBUG, FONT, "Loading fontset %s", fontname);
        fontset=de_create_font_set(fontname);
        if(fontset!=NULL){
            if(XContextDependentDrawing(fontset)){
                warn(TR("Fontset for font pattern '%s' implements context "
                        "dependent drawing, which is unsupported. Expect "
                        "clutter."), fontname);
            }
        }
    }else{
        LOG(DEBUG, FONT, "Loading fontstruct %s", fontname);
        fontstruct=XLoadQueryFont(ioncore_g.dpy, fontname);
    }

    if(fontstruct==NULL && fontset==NULL){
        if(strcmp(fontname, default_fontname)!=0){
            DEFont *fnt;
            LOG(WARN, FONT, TR("Could not load font \"%s\", trying \"%s\""),
                 fontname, default_fontname);
            fnt=de_load_font(default_fontname);
            if(fnt==NULL)
                LOG(WARN, FONT, TR("Failed to load fallback font."));
            return fnt;
        }
        return NULL;
    }

#endif /* HAVE_X11_XFT */
    fnt=ALLOC(DEFont);

    if(fnt==NULL)
        return NULL;

#ifdef HAVE_X11_XFT
    fnt->font=font;
#else
    fnt->fontset=fontset;
    fnt->fontstruct=fontstruct;
#endif
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

#ifndef HAVE_X11_XFT
    if(style->font->fontstruct!=NULL){
        XSetFont(ioncore_g.dpy, style->normal_gc,
                 style->font->fontstruct->fid);
    }

#endif /* ! HAVE_X11_XFT */
    return TRUE;
}


bool de_load_font_for_style(DEStyle *style, const char *fontname)
{
    if(style->font!=NULL)
        de_free_font(style->font);

    style->font=de_load_font(fontname);

    if(style->font==NULL)
        return FALSE;

#ifndef HAVE_X11_XFT
    if(style->font->fontstruct!=NULL){
        XSetFont(ioncore_g.dpy, style->normal_gc,
                 style->font->fontstruct->fid);
    }

#endif /* ! HAVE_X11_XFT */
    return TRUE;
}


void de_free_font(DEFont *font)
{
    if(--font->refcount!=0)
        return;

#ifdef HAVE_X11_XFT
    if(font->font!=NULL)
        XftFontClose(ioncore_g.dpy, font->font);
#else /* HAVE_X11_XFT */
    if(font->fontset!=NULL)
        XFreeFontSet(ioncore_g.dpy, font->fontset);
    if(font->fontstruct!=NULL)
        XFreeFont(ioncore_g.dpy, font->fontstruct);
#endif /* HAVE_X11_XFT */
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
#ifdef HAVE_X11_XFT
    if(font->font!=NULL){
        fnte->max_height=font->font->ascent+font->font->descent;
        fnte->max_width=font->font->max_advance_width;
        fnte->baseline=font->font->ascent;
        return;
    }
#else /* HAVE_X11_XFT */
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
#endif /* HAVE_X11_XFT */
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
#ifdef HAVE_X11_XFT
    if(font->font!=NULL){
        XGlyphInfo extents;
        if(ioncore_g.enc_utf8)
            XftTextExtentsUtf8(ioncore_g.dpy, font->font, (XftChar8*)text, len, &extents);
        else
            XftTextExtents8(ioncore_g.dpy, font->font, (XftChar8*)text, len, &extents);
        return extents.xOff;
    }else{
        return 0;
    }
#else /* HAVE_X11_XFT */
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
#endif /* HAVE_X11_XFT */
}


/*}}}*/


/*{{{ String drawing */


#ifdef HAVE_X11_XFT
void debrush_do_draw_string_default(DEBrush *brush,
                                    int x, int y, const char *str,
                                    int len, bool needfill,
                                    DEColourGroup *colours)
{
    Window win = brush->win;
    GC gc=brush->d->normal_gc;
    XftDraw *draw;
    XftFont *font;

    if(brush->d->font==NULL)
        return;

    font=brush->d->font->font;
    draw=debrush_get_draw(brush, win);

    if(needfill){
        XGlyphInfo extents;
        if(ioncore_g.enc_utf8){
            XftTextExtentsUtf8(ioncore_g.dpy, font, (XftChar8*)str, len,
                               &extents);
        }else{
            XftTextExtents8(ioncore_g.dpy, font, (XftChar8*)str, len, &extents);
        }
        XftDrawRect(draw, &(colours->bg), x-extents.x, y-extents.y,
                    extents.width+10, extents.height);
    }

    if(ioncore_g.enc_utf8){
        XftDrawStringUtf8(draw, &(colours->fg), font, x, y, (XftChar8*)str,
                          len);
    }else{
        XftDrawString8(draw, &(colours->fg), font, x, y, (XftChar8*)str, len);
    }
}
#else /* HAVE_X11_XFT */
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

#endif /* HAVE_X11_XFT */

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

