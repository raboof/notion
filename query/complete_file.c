/****************************************************************************/
/*                                                                          */
/* Copyright 1992 Simmule Turner and Rich Salz.  All rights reserved.       */
/*                                                                          */
/* This software is not subject to any license of the American Telephone    */
/* and Telegraph Company or of the Regents of the University of California. */
/*                                                                          */
/* Permission is granted to anyone to use this software for any purpose on  */
/* any computer system, and to alter it and redistribute it freely, subject */
/* to the following restrictions:                                           */
/* 1. The authors are not responsible for the consequences of use of this   */
/*    software, no matter how awful, even if they arise from flaws in it.   */
/* 2. The origin of this software must not be misrepresented, either by     */
/*    explicit claim or by omission.  Since few users ever read sources,    */
/*    credits must appear in the documentation.                             */
/* 3. Altered versions must be plainly marked as such, and must not be      */
/*    misrepresented as being the original software.  Since few users       */
/*    ever read sources, credits must appear in the documentation.          */
/* 4. This notice may not be removed or altered.                            */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/*  This is a line-editing library, it can be linked into almost any        */
/*  program to provide command-line editing and recall.                     */
/*                                                                          */
/*  Posted to comp.sources.misc Sun, 2 Aug 1992 03:05:27 GMT                */
/*      by rsalz@osf.org (Rich $alz)                                        */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/*  The version contained here has some modifications by awb@cstr.ed.ac.uk  */
/*  (Alan W Black) in order to integrate it with the Edinburgh Speech Tools */
/*  library and Scheme-in-one-defun in particular.  All modifications to    */
/*  to this work are continued with the same copyright above.  That is      */
/*  This version editline does not have the the "no commercial use"         */
/*  restriction that some of the rest of the EST library may have           */
/*  awb Dec 30 1998                                                         */
/*                                                                          */
/****************************************************************************/
/*  $Revision: 1.1 $
 **
 **  History and file completion functions for editline library.
 */


/*
 * Adapted for use with Ion and tilde-expansion added by
 * Tuomo Valkonen <tuomov@cc.tut.fi>, 2000-08-23.
 */

#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>

#include <wmcore/common.h>
#include "complete_file.h"

#define STRDUP(X) scopy(X)
#define DISPOSE(X) free(X)
#define NEW(T, X)  ALLOC_N(T, X)
#define STATIC static
#define EL_CONST const
#define SIZE_T int
#define MEM_INC 64
#define COPYFROMTO(new, p, len) \
	(void)memcpy((char *)(new), (char *)(p), (int)(len))

typedef struct dirent DIRENTRY;

static void el_add_slash(char *path,char *p)
{
	struct stat sb;
	
	if (stat(path, &sb) >= 0)
		(void)strcat(p, S_ISDIR(sb.st_mode) ? "/" : " ");
}

static int el_is_directory(char *path)
{
	struct stat sb;
	
	if ((stat(path, &sb) >= 0) && S_ISDIR(sb.st_mode))
		return 1;
	else
		return 0;
}

static int el_is_executable(char *path)
{
	unsigned int t_uid = geteuid();
	struct stat sb;
	if((stat(path, &sb) >= 0) && S_ISREG(sb.st_mode)) {
		/* Normal file, see if we can execute it. */
		
		if (sb.st_mode & S_IXOTH) { /* All can execute */
			return (1);
		}
		if (sb.st_uid == t_uid && (sb.st_mode & S_IXUSR)) {
			return (1);
		}
	}
	return (0);
}


/*
 **  strcmp-like sorting predicate for qsort.
 */
/*STATIC int compare(EL_CONST void *p1,EL_CONST void *p2)
 {
 EL_CONST char	**v1;
 EL_CONST char	**v2;
 * 
 v1 = (EL_CONST char **)p1;
 v2 = (EL_CONST char **)p2;
 return strcmp(*v1, *v2);
 }*/

/*
 **  Fill in *avp with an array of names that match file, up to its length.
 **  Ignore . and .. .
 */
static int FindMatches(char *dir,char *file,char ***avp)
{
    char	**av;
    char	**new;
    char	*p;
    DIR		*dp;
    DIRENTRY	*ep;
    SIZE_T	ac;
    SIZE_T	len;
	
	if(*dir=='\0')
		dir=".";
	
    if ((dp = opendir(dir)) == NULL)
		return 0;
	
    av = NULL;
    ac = 0;
    len = strlen(file);
    while ((ep = readdir(dp)) != NULL) {
		p = ep->d_name;
		if (p[0] == '.' && (p[1] == '\0' || (p[1] == '.' && p[2] == '\0')))
			continue;
		if (len && strncmp(p, file, len) != 0)
			continue;
		
		if ((ac % MEM_INC) == 0) {
			if ((new = NEW(char*, ac + MEM_INC)) == NULL)
				break;
			if (ac) {
				COPYFROMTO(new, av, ac * sizeof (char **));
				DISPOSE(av);
			}
			*avp = av = new;
		}
		
		if ((av[ac] = STRDUP(p)) == NULL) {
			if (ac == 0)
				DISPOSE(av);
			break;
		}
		ac++;
    }
	
    /* Clean up and return. */
    (void)closedir(dp);
	/*    if (ac)
	 qsort(av, ac, sizeof (char **), compare);*/
    return ac;
}


/*
 ** Slight modification on FindMatches, to search through current PATH
 ** 
 */
static int FindFullPathMatches(char *file,char ***avp)
{
    char	**av;
    char	**new;
    char	*p;
    char    *path = getenv("PATH");
    char    t_tmp[1024];
    char    *t_path;
    char    *t_t_path;
    DIR		*dp;
    DIRENTRY*ep;
    SIZE_T	ac;
    SIZE_T	len;
	
    t_path = strdup(path);
    t_t_path = t_path;
	
    av = NULL;
    ac = 0;
	
    t_t_path = strtok(t_path, ":\n\0");
    while (t_t_path) {
		if ((dp = opendir(t_t_path)) == NULL) {
			t_t_path = strtok(NULL, ":\n\0");
			continue;
		}
		
		len = strlen(file);
		while ((ep = readdir(dp)) != NULL) {
			p = ep->d_name;
			if (p[0] == '.' && (p[1] == '\0' || (p[0] == '.' &&
												 p[1] == '.' &&
												 p[2] == '\0'))) {
				continue;
			}
			if (len && strncmp(p, file, len) != 0) {
				continue;
			}
			
			snprintf(t_tmp, 1024, "%s/%s", t_t_path, p);
			if(!el_is_executable(t_tmp)) {
				continue;
			}
			
			if ((ac % MEM_INC) == 0) {
				if ((new = NEW(char*, ac + MEM_INC)) == NULL) {
					break;
				}
				if (ac) {
					COPYFROMTO(new, av, ac * sizeof (char **));
					DISPOSE(av);
				}
				*avp = av = new;
			}
			if ((av[ac] = STRDUP(p)) == NULL) {
				if (ac == 0)
					DISPOSE(av);
				break;
			}
			ac++;
		}
		(void)closedir(dp);
		t_t_path = strtok(NULL, ":\n\0");
		
		
    } /* t_path-while */
	
    /* Clean up and return. */
	
    /*if (ac)
	 qsort(av, ac, sizeof (char **), compare); */
    free(t_path);
	
    return ac;
}


/*
 **  Split a pathname into allocated directory and trailing filename parts.
 */
STATIC int SplitPath(const char *path,char **dirpart,char **filepart)
{
    static char	DOT[] = "./";
    char	*dpart;
    char	*fpart;
	
    if ((fpart = strrchr(path, '/')) == NULL) {
		/* No slashes in path */
		if ((dpart = STRDUP(DOT)) == NULL)
			return -1;
		if ((fpart = STRDUP(path)) == NULL) {
			DISPOSE(dpart);
			return -1;
		}
    }else{
		if ((dpart = STRDUP(path)) == NULL)
			return -1;
		/* Include the slash -- Tuomo */
		dpart[fpart - path + 1] = '\0';
		if ((fpart = STRDUP(++fpart)) == NULL) {
			DISPOSE(dpart);
			return -1;
		}
		/* Root no longer a special case due above -- Tuomo
		 if (dpart[0] == '\0')
		 {
		 dpart[0] = '/';
		 dpart[1] = '\0';
		 }
		 */
    }
    *dirpart = dpart;
    *filepart = fpart;
    return 0;
}

/*
 **  Split a pathname into allocated directory and trailing filename parts.
 */
STATIC int SplitRelativePath(const char *path,char **dirpart,char **filepart)
{
    static char	DOT[] = "./";
    static char EOL[] = "\0";
    char *dpart;
    char *fpart;
	
    if ((fpart = strrchr(path, '/')) == NULL) {
		/* No slashes in path */
		if ((dpart = STRDUP(EOL)) == NULL)
			return -1;
		if ((fpart = STRDUP(path)) == NULL) {
			DISPOSE(dpart);
			return -1;
		}
    }
    else {
		if ((dpart = STRDUP(path)) == NULL)
			return -1;
		/* Include the slash -- Tuomo */
		dpart[fpart - path + 1] = '\0';
		if ((fpart = STRDUP(++fpart)) == NULL) {
			DISPOSE(dpart);
			return -1;
		}
		/* Root no longer a special case due above -- Tuomo
		 if (dpart[0] == '\0')
		 {
		 dpart[0] = '/';
		 dpart[1] = '\0';
		 }
		 */
    }
    *dirpart = dpart;
    *filepart = fpart;
    return 0;
}

/*
 **  Attempt to complete the pathname, returning an allocated copy.
 **  Fill in *unique if we completed it, or set it to 0 if ambiguous.
 */
#if 0
static char *el_complete(char *pathname,int *unique)
{
    char	**av;
    char	*dir;
    char	*file;
    char	*new;
    char	*p;
    SIZE_T	ac;
    SIZE_T	end;
    SIZE_T	i;
    SIZE_T	j;
    SIZE_T	len;
	
    if (SplitPath(pathname, &dir, &file) < 0)
		return NULL;
	
    if ((ac = FindMatches(dir, file, &av)) == 0) {
		DISPOSE(dir);
		DISPOSE(file);
		return NULL;
    }
	
    p = NULL;
    len = strlen(file);
    if (ac == 1) {
		/* Exactly one match -- finish it off. */
		*unique = 1;
		j = strlen(av[0]) - len + 2;
		if ((p = NEW(char, j + 1)) != NULL) {
			COPYFROMTO(p, av[0] + len, j);
			if ((new = NEW(char, strlen(dir) + strlen(av[0]) + 2)) != NULL) {
				(void)strcpy(new, dir);
				(void)strcat(new, "/");
				(void)strcat(new, av[0]);
				el_add_slash(new, p);
				DISPOSE(new);
			}
		}
    }
    else {
		*unique = 0;
		if (len) {
			/* Find largest matching substring. */
			for (i = len, end = strlen(av[0]); i < end; i++)
			for (j = 1; j < ac; j++)
		    if (av[0][i] != av[j][i])
				goto breakout;
		breakout:
			if (i > len) {
				j = i - len + 1;
				if ((p = NEW(char, j)) != NULL) {
					COPYFROMTO(p, av[0] + len, j);
					p[j - 1] = '\0';
				}
			}
		}
    }
	
    /* Clean up and return. */
    DISPOSE(dir);
    DISPOSE(file);
    for (i = 0; i < ac; i++)
		DISPOSE(av[i]);
    DISPOSE(av);
    return p;
}
#endif

static int complete_homedir(const char *username, char ***cp_ret, char **beg)
{
	struct passwd *pw;
	char *name;
	char **cp;
	int n=0, l=strlen(username);
	
	*cp_ret=NULL;
	
	for(pw=getpwent(); pw!=NULL; pw=getpwent()){
		name=pw->pw_name;
		
		if(l && strncmp(name, username, l))
			continue;
		
		name=scat3("~", name, "/");
		
		if(name==NULL){
			warn_err();
			continue;
		}
		
		cp=REALLOC_N(*cp_ret, char*, n, n+1);
		
		if(cp==NULL){
			warn_err();
			free(name);
			if(*cp_ret!=NULL)
			free(*cp_ret);
		}else{
			cp[n]=name;
			n++;
			*cp_ret=cp;
		}
	}
	
	endpwent();
	
	if(n==1){
		name=**cp_ret;
		name[strlen(name)-1]='\0';
		pw=getpwnam(name+1);
		if(pw!=NULL && pw->pw_dir!=NULL){
			name=scat(pw->pw_dir, "/");
			if(name!=NULL){
				free(**cp_ret);
				**cp_ret=name;
			}
		}
	}
	
	return n;
}

/* 
 * ret: 0 not a home directory,
 * 		1 home directory (repath set)
 *		2 someone's home directory
 */
static int tilde_complete(char *path, char **retpath)
{
	char *home;
	char *p;
	struct passwd *pw;
	
	if(*path!='~')
		return 0;
	
	if(*(path+1)!='/' && *(path+1)!='\0'){
		p=strchr(path, '/');
		
		if(p==NULL)
			return 2;
		
		*p='\0';
		pw=getpwnam(path+1);
		*p='/';
		
		if(pw==NULL)
			return 0;
		
		home=pw->pw_dir;
	}else{
		p=path+1;
		home=getenv("HOME");
	}
	
	if(home!=NULL){
		if(*p=='\0')
			*retpath=scat3(home, p, "/");
		else
			*retpath=scat(home, p);
	}
	
	return (*retpath!=NULL);
}


/*
 **  Return all possible completions.
 */
int complete_file(char *pathname, char ***avp, char **beg)
{
    char	*dir;
    char	*file, *path=NULL, *tt;
    int		ac=0, i;
    
    switch(tilde_complete(pathname, &path)){
    case 0:
		i=SplitPath(pathname, &dir, &file);
		break;
    case 2:
		return complete_homedir(pathname+1, avp, beg);
    default:
		i=SplitPath(path, &dir, &file);
    }
    
    if(i<0)
		return 0;
    
    ac=FindMatches(dir, file, avp);
    
    DISPOSE(file);
    
    if(ac==0 && path!=NULL){
		*avp=ALLOC(char*);
		if(*avp==NULL)
			return 0;
		**avp=path;
			return 1;
    }else if(path!=NULL){
		free(path);
    }
    
    /* Identify directories with trailing / */
    for(i=0; i<ac; i++){
		path = NEW(char,strlen(dir)+strlen((*avp)[i])+3);
		sprintf(path,"%s/%s",dir,(*avp)[i]);
		if(el_is_directory(path)){
			tt = NEW(char,strlen((*avp)[i])+2);
			sprintf(tt,"%s/",(*avp)[i]);
			DISPOSE((*avp)[i]);
			(*avp)[i] = tt;
		}
		DISPOSE(path);
    }
    
    *beg=dir;
    
    return ac;
}


/*
 **  Return all possible completions.
 */
int complete_file_with_path(char *pathname, char ***avp, char **beg)
{
    char	*dir;
    char	*file, *path=NULL, *tt;
    int		ac=0, i, dcomp=FALSE;
    
    switch(tilde_complete(pathname, &path)){
    case 0:
		i=SplitRelativePath(pathname, &dir, &file);
		break;
    case 2:
		return complete_homedir(pathname+1, avp, beg);
    default:
		i=SplitPath(path, &dir, &file);
    }
    
    if(i<0)
		return 0;
    
    if(*dir=='\0')
		ac=FindFullPathMatches(file, avp); /* No slashes in path so far. */
	
	if(ac==0){
		if(*dir=='\0'){
			dir=scopy("./");
			if(dir==NULL){
				warn_err();
				return 0;
			}
		}
		ac=FindMatches(dir, file, avp);
		dcomp=TRUE;
	}
    
    DISPOSE(file);
    
    if(ac==0 && path!=NULL){
		*avp=ALLOC(char*);
		if(*avp==NULL)
			return 0;
		**avp=path;
			return 1;
    }else if(path!=NULL){
		free(path);
    }
    
    /* Identify directories with trailing / */
	if(dcomp){
	    for (i = 0; i < ac; i++) {
			path = NEW(char,strlen(dir)+strlen((*avp)[i])+3);
			sprintf(path,"%s/%s",dir,(*avp)[i]);
			if (el_is_directory(path)) {
				tt = NEW(char,strlen((*avp)[i])+2);
				sprintf(tt,"%s/",(*avp)[i]);
				DISPOSE((*avp)[i]);
				(*avp)[i] = tt;
			}
			DISPOSE(path);
	    }
	}
    
    *beg=dir;
    
    return ac;
}
