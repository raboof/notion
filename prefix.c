
#include <string.h>
#include <libtu/misc.h>
#include <libtu/locale.h>
#include <libextl/readconfig.h>


static char *the_prefix=NULL;

#ifdef CF_RELOCATABLE

void get_prefix(const char *binloc, const char *dflt)
{
    int i=strlen(binloc);
    int j=strlen(dflt);

    if(binloc[0]!='/')
        die(TR("This relocatable binary should be started with an absolute path."));
    
    while(i>0 && j>0){
        if(binloc[i-1]!=dflt[j-1])
            break;
        i--;
        j--;
    }
    
    the_prefix=scopyn(binloc, i);
    
}


char *ion_prefix(const char *s)
{
    if(the_prefix==NULL)
        return scopy(s);
    else
        return scat3(the_prefix, "/", s);
}


void wrap_extl_add_searchdir(const char *s)
{
    char *s2=ion_prefix(s);
    if(s2!=NULL){
        extl_add_searchdir(s2);
        free(s2);
    }
}

#else

void get_prefix(const char *binloc, const char *dflt)
{
}

char *ion_prefix(const char *s)
{
    return scopy(s);
}


void wrap_extl_add_searchdir(const char *s)
{
    extl_add_searchdir(s);
}

#endif
