/*
 * ion/ioncore/strings.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
 */

#include <libtu/output.h>
#include <libtu/misc.h>
#include <string.h>
#include <regex.h>
#include "log.h"
#include "common.h"
#include "global.h"
#include "strings.h"


/*{{{ String scanning */


wchar_t str_wchar_at(char *p, int max)
{
    wchar_t wc;
    if(mbtowc(&wc, p, max)>0)
        return wc;
    return 0;
}
    

char *str_stripws(char *p)
{
    mbstate_t ps;
    wchar_t wc;
    int first=-1, pos=0;
    int n=strlen(p);
    int ret;
    
    memset(&ps, 0, sizeof(ps));
    
    while(1){
        ret=mbrtowc(&wc, p+pos, n-pos, &ps);
        if(ret<=0)
            break;
        if(!iswspace(wc))
            break;
        pos+=ret;
    }
    
    if(pos!=0)
        memmove(p, p+pos, n-pos+1);
    
    if(ret<=0)
        return p;
    
    pos=ret;
    
    while(1){
        ret=mbrtowc(&wc, p+pos, n-pos, &ps);
        if(ret<=0)
            break;
        if(iswspace(wc)){
            if(first==-1)
                first=pos;
        }else{
            first=-1;
        }
        pos+=ret;
    }
        
    if(first!=-1)
        p[first]='\0';
    
    return p;
}


int str_prevoff(const char *p, int pos)
{
    if(ioncore_g.enc_sb)
        return (pos>0 ? 1 : 0);

    if(ioncore_g.enc_utf8){
        int opos=pos;
        
        while(pos>0){
            pos--;
            if((p[pos]&0xC0)!=0x80)
                break;
        }
        return opos-pos;
    }
    
    assert(ioncore_g.use_mb);
    {
        /* *sigh* */
        int l, prev=0;
        mbstate_t ps;
        
        memset(&ps, 0, sizeof(ps));
        
        while(1){
            l=mbrlen(p+prev, pos-prev, &ps);
            if(l<0){
                warn(TR("Invalid multibyte string."));
                return 0;
            }
            if(prev+l>=pos)
                return pos-prev;
            prev+=l;
        }
        
    }
}


int str_nextoff(const char *p, int opos)
{
    if(ioncore_g.enc_sb)
        return (*(p+opos)=='\0' ? 0 : 1);

    if(ioncore_g.enc_utf8){
        int pos=opos;
        
        while(p[pos]){
            pos++;
            if((p[pos]&0xC0)!=0x80)
                break;
        }
        return pos-opos;
    }

    assert(ioncore_g.use_mb);
    {
        mbstate_t ps;
        int l;
        memset(&ps, 0, sizeof(ps));
        
        l=mbrlen(p+opos, strlen(p+opos), &ps);
        if(l<0){
            warn(TR("Invalid multibyte string."));
            return 0;
        }
        return l;
    }
}


int str_len(const char *p)
{
    if(ioncore_g.enc_sb)
        return strlen(p);

    if(ioncore_g.enc_utf8){
        int len=0;
        
        while(*p){
            if(((*p)&0xC0)!=0x80)
                len++;
            p++;
        }
        return len;
    }

    assert(ioncore_g.use_mb);
    {
        mbstate_t ps;
        int len=0, bytes=strlen(p), l;
        memset(&ps, 0, sizeof(ps));
        
        while(bytes>0){
            l=mbrlen(p, bytes, &ps);
            if(l<=0){
                warn(TR("Invalid multibyte string."));
                break;
            }
            len++;
            bytes-=l;
            p += l;
        }
        return len;
    }
    
}

/*}}}*/


/*{{{ Title shortening */


static char *scatn3(const char *p1, int l1,
                    const char *p2, int l2,
                    const char *p3, int l3)
{
    char *p=ALLOC_N(char, l1+l2+l3+1);
    
    if(p!=NULL){
        strncat(p, p1, l1);
        strncat(p, p2, l2);
        strncat(p, p3, l3);
    }
    return p;
}

INTRSTRUCT(SR);
    
DECLSTRUCT(SR){
    regex_t re;
    char *rule;
    SR *next, *prev;
    bool always;
};


static SR *shortenrules=NULL;


/*EXTL_DOC
 * Add a rule describing how too long titles should be shortened to fit in tabs.
 * The regular expression \var{rx} (POSIX, not Lua!) is used to match titles
 * and when \var{rx} matches, \var{rule} is attempted to use as a replacement
 * for title. If \var{always} is set, the rule is used even if no shortening 
 * is necessary.
 *
 * Similarly to sed's 's' command, \var{rule} may contain characters that are
 * inserted in the resulting string and specials as follows:
 * 
 * \begin{tabularx}{\linewidth}{lX}
 *  \tabhead{Special & Description}
 *  \$0 &          Place the original string here. \\
 *  \$1 to \$9 & Insert n:th capture here (as usual,captures are surrounded
 *                 by parentheses in the regex). \\
 *  \$| &          Alternative shortening separator. The shortening described
 *                 before the first this kind of separator is tried first and
 *                 if it fails to make the string short enough, the next is 
 *                  tried, and so on. \\
 *  \$< &         Remove characters on the left of this marker to shorten the
 *                 string. \\
 *  \$> &         Remove characters on the right of this marker to shorten the
 *                 string. Only the first \$< or \$> within an alternative 
 *                 shortening is used. \\
 * \end{tabularx}
 */
EXTL_EXPORT
bool ioncore_defshortening(const char *rx, const char *rule, bool always)
{
    SR *si;
    int ret;
    #define ERRBUF_SIZE 256
    static char errbuf[ERRBUF_SIZE];
        
    if(rx==NULL || rule==NULL)
        return FALSE;
    
    si=ALLOC(SR);

    if(si==NULL)
        return FALSE;
    
    ret=regcomp(&(si->re), rx, REG_EXTENDED);
    
    if(ret!=0){
        errbuf[0]='\0';
        regerror(ret, &(si->re), errbuf, ERRBUF_SIZE);
        warn(TR("Error compiling regular expression: %s"), errbuf);
        goto fail2;
    }
    
    si->rule=scopy(rule);
    si->always=always;
    
    if(si->rule==NULL)
        goto fail;
    
    LINK_ITEM(shortenrules, si, next, prev);
    
    return TRUE;
    
fail:
    regfree(&(si->re));
fail2:
    free(si);
    return FALSE;
}


static char *shorten(GrBrush *brush, const char *str, uint maxw,
                     const char *rule, int nmatch, regmatch_t *pmatch)
{
    char *s;
    int rulelen, slen, i, j, k, ll;
    int strippt=0;
    int stripdir=-1;
    bool more=FALSE;
    
    /* Ensure matches are at character boundaries */
    if(!ioncore_g.enc_sb){
        int pos=0, len, strl;
        mbstate_t ps;
        memset(&ps, 0, sizeof(ps));
        
        strl=strlen(str);

        while(pos<strl){
            len=mbrtowc(NULL, str+pos, strl-pos, &ps);
            if(len<0){
                /* Invalid multibyte string */
                return scopy("???");
            }
            if(len==0)
                break;
            for(i=0; i<nmatch; i++){
                if(pmatch[i].rm_so>pos && pmatch[i].rm_so<pos+len)
                    pmatch[i].rm_so=pos+len;
                if(pmatch[i].rm_eo>pos && pmatch[i].rm_eo<pos+len)
                    pmatch[i].rm_eo=pos;
            }
            pos+=len;
        }
    }

    /* Stupid alloc rule that wastes space */
    rulelen=strlen(rule);
    slen=rulelen;
    
    for(i=0; i<nmatch; i++){
        if(pmatch[i].rm_so==-1)
            continue;
        slen+=(pmatch[i].rm_eo-pmatch[i].rm_so);
    }
    
    s=ALLOC_N(char, slen);
    
    if(s==NULL)
        return NULL;
    
    do{
        more=FALSE;
        j=0;
        strippt=0;
        stripdir=-1;
        
        for(i=0; i<rulelen; i++){
            if(rule[i]!='$'){
                s[j++]=rule[i];
                continue;
            }
            
            i++;
            
            if(rule[i]=='|'){
                rule=rule+i+1;
                rulelen=rulelen-i-1;
                more=TRUE;
                break;
            }
            
            if(rule[i]=='$'){
                s[j++]='$';
                continue;
            }
            
            if(rule[i]=='<'){
                strippt=j;
                stripdir=-1;
                continue;
            }
            
            if(rule[i]=='>'){
                strippt=j;
                stripdir=1;
                continue;
            }
            
            if(rule[i]>='0' && rule[i]<='9'){
                k=(int)(rule[i]-'0');
                if(k>=nmatch)
                    continue;
                if(pmatch[k].rm_so==-1)
                    continue;
                ll=(pmatch[k].rm_eo-pmatch[k].rm_so);
                strncpy(s+j, str+pmatch[k].rm_so, ll);
                j+=ll;
            }
        }
        
        slen=j;
        s[slen]='\0';
        
        i=strippt;
        j=strippt;
        
        /* shorten */
        {
            uint bl=grbrush_get_text_width(brush, s, i);
            uint el=grbrush_get_text_width(brush, s+j, slen-j);

            while(1){
                /* el+bl may not be the actual length, but close enough. */
                if(el+bl<=maxw){
                    memmove(s+i, s+j, slen-j+1);
                    return s;
                }
                
                if(stripdir==-1){
                    ll=str_prevoff(s, i);
                    if(ll==0)
                        break;
                    i-=ll;
                    bl=grbrush_get_text_width(brush, s, i);
                }else{
                    ll=str_nextoff(s, j);
                    if(ll==0)
                        break;
                    j+=ll;
                    el=grbrush_get_text_width(brush, s+j, slen-j);
                }
            }
        }
    }while(more);
        
    free(s);

    return NULL;
}


char *grbrush_make_label(GrBrush *brush, const char *str, uint maxw)
{
    size_t nmatch=10;
    regmatch_t pmatch[10];
    SR *rule;
    int ret;
    char *retstr;
    bool fits=FALSE;
    
    if(grbrush_get_text_width(brush, str, strlen(str))<=maxw)
        fits=TRUE;
        
        /*return scopy(str);*/
    
    for(rule=shortenrules; rule!=NULL; rule=rule->next){
        if(fits && !rule->always)
            continue;
        ret=regexec(&(rule->re), str, nmatch, pmatch, 0);
        if(ret!=0)
            continue;
        retstr=shorten(brush, str, maxw, rule->rule, nmatch, pmatch);
        goto rettest;
    }

    if(fits){
        retstr=scopy(str);
    }else{
        pmatch[0].rm_so=0;
        pmatch[0].rm_eo=strlen(str)-1;
        retstr=shorten(brush, str, maxw, "$1$<...", 1, pmatch);
    }
    
rettest:
    if(retstr!=NULL)
        return retstr;
    return scopy("");
}


/*}}}*/

