/*
 * ion/ioncore/readconfig.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <unistd.h>
#include <ltdl.h>
#include <libtu/output.h>

#include "common.h"
#include "global.h"
#include "readconfig.h"
#include "extl.h"


static char* dummy_paths=NULL;
static char **etcpaths=&dummy_paths;
static int n_etcpaths=0;
/*static char **libpaths=&dummy_paths;
static int n_libpaths=0;*/


/*{{{ Init */


static bool add_dir(char ***pathsptr, int *n_pathsptr, const char *dir)
{
	char **paths;
	char *dircp;
	int i;
	
	if(dir==NULL)
		return FALSE;
	
	dircp=scopy(dir);
	
	if(dircp==NULL){
		warn_err();
		return FALSE;
	}
		
	if(*n_pathsptr==0)
		paths=ALLOC_N(char*, 2);
	else
		paths=REALLOC_N(*pathsptr, char*, (*n_pathsptr)+1, (*n_pathsptr)+2);
	
	if(paths==NULL){
		warn_err();
		free(dircp);
		return FALSE;
	}
	
	paths[*n_pathsptr]=dircp;
	paths[(*n_pathsptr)+1]=NULL;
	(*n_pathsptr)++;
	*pathsptr=paths;
	
	return TRUE;
}


bool ioncore_add_confdir(const char *dir)
{
	return add_dir(&etcpaths, &n_etcpaths, dir);
}


bool ioncore_add_libdir(const char *dir)
{
	return (lt_dladdsearchdir(dir)==0);
	/*return add_dir(&libpaths, &n_libpaths, dir);*/
}


#if 0
bool ioncore_add_default_paths(const char *appname, const char *etcdir,
							   const char *libdir)
{
	char *home, *tmp, *tmp2;
	int fails=0;
	
	home=getenv("HOME");
	
	if(home==NULL){
		warn("$HOME not set");
	}else{
		tmp=scat3(home, "/.", appname);
		if(tmp==NULL){
			warn_err();
		}else{
			fails+=!ioncore_add_confdir(tmp);
			tmp2=scat(tmp, "/lib");
			if(tmp2==NULL){
				warn_err();
			}else{
				fails+=!ioncore_add_libdir(tmp2);
				free(tmp2);
			}
			free(tmp);
		}
	}

	tmp=scat3(etcdir, "/", appname);
	if(tmp==NULL){
		warn_err();
	}else{
		fails+=!ioncore_add_confdir(tmp);
		free(tmp);
	}

	tmp=scat3(libdir, "/", appname);
	if(tmp==NULL){
		warn_err();
	}else{
		fails+=!ioncore_add_libdir(tmp);
		free(tmp);
	}
	
	return (fails==0);
}
#endif


/*}}}*/


/*{{{ get_cfgfile */


static char *search_etcpath2(const char *const *files, bool noaccesstest)
{
	char *tmp=NULL;
	char *const *dir;
	const char *const *file;
	for(dir=etcpaths; *dir!=NULL; dir++){
		for(file=files; *file!=NULL; file++){
			libtu_asprintf(&tmp, "%s/%s", *dir, *file);
			if(tmp==NULL){
				warn_err();
				continue;
			}
			if(noaccesstest || access(tmp, F_OK)==0)
				return tmp;
			free(tmp);
		}
	}
	return NULL;
}


EXTL_EXPORT
void do_include(const char *file, const char *current_dir)
{
	const char *files[]={NULL, NULL};
	char* tmp=NULL;
	
	if(file==NULL)
		return;
	
	if(file[0]=='/'){
		if(access(file, F_OK)!=0)
			goto fail;
		extl_dofile(file, NULL, NULL);
		return;
	}
	if(current_dir==NULL)
		current_dir=".";
	
	libtu_asprintf(&tmp, "%s/%s", current_dir, file);
	if(tmp!=NULL){
		if(access(tmp, F_OK)!=0){
			free(tmp);
			tmp=NULL;
		}
	}
	
	if(tmp==NULL){
		files[0]=file;
		tmp=search_etcpath2(files, FALSE);
	}
	
	if(tmp==NULL)
		goto fail;
	
	extl_dofile(tmp, NULL, NULL);
	free(tmp);
	return;
	
fail:
	warn("Could not find file %s in search path", file);
}


static char *do_get_cfgfile_for(const char *module, const char *postfix,
								bool noaccesstest)
{
	char *files[]={NULL, NULL, NULL};
	char *ret;
	
	if(module==NULL)
		return NULL;
	
	if(postfix!=NULL){
		libtu_asprintf(files+0, "%s-%s.%s", module, postfix,
					   extl_extension());
	}
	libtu_asprintf(files+(postfix==NULL ? 0 : 1), "%s.%s", module,
				   extl_extension());
	
	ret=search_etcpath2((const char**)&files, noaccesstest);
	
	return ret;
}


static char *do_get_cfgfile_for_scr(const char *module, int xscr,
									bool noaccesstest)
{
	char *ret, *tmp, *display, *dpyend;
	
	display=XDisplayName(wglobal.display);
	
	dpyend=strchr(display, ':');
	if(dpyend==NULL)
		goto fallback;
	
	dpyend=strchr(dpyend, '.');
	
	if(dpyend==NULL) {
		libtu_asprintf(&tmp, "%s.%d", display, xscr);
	}else{
		libtu_asprintf(&tmp, "%.*s.%d", (int)(dpyend-display),
				   						display, xscr);
	}
	
	/* It seems Xlib doesn't want this freed */
	/*XFree(display);*/
	
	if(tmp==NULL){
		warn_err();
		goto fallback;
	}

	/* Some second-rate OSes/filesystems don't like the colon */
#ifdef CF_SECOND_RATE_OS_FS
	{
		char *colon=tmp;
		while(1){
			colon=strchr(colon, ':');
			if(colon==NULL)
				break;
			*colon='_';
		}
	}
#endif
	
	ret=do_get_cfgfile_for(module, tmp, noaccesstest);
	
	free(tmp);
	
	return ret;

fallback:
	ret=do_get_cfgfile_for(module, NULL, noaccesstest);
	return ret;
}


char *get_cfgfile_for_scr(const char *module, int xscr)
{
	return do_get_cfgfile_for_scr(module, xscr, FALSE);
	
}


char *get_cfgfile_for(const char *module)
{
	return do_get_cfgfile_for(module, NULL, FALSE);
}


char *get_savefile_for_scr(const char *module, int xscr)
{
	return do_get_cfgfile_for_scr(module, xscr, TRUE);
	
}


char *get_savefile_for(const char *module)
{
	return do_get_cfgfile_for(module, NULL, TRUE);
	
}


/*}}}*/


/*{{{ read_config */


bool read_config(const char *cfgfile)
{
	return extl_dofile(cfgfile, NULL, NULL);
}


static bool do_read_config_for(const char *module, char *cfgfile)
{
	bool ret=TRUE;
	if(cfgfile==NULL){
		warn("Could not find configuration file for \"%s\".", module);
	}else{
		ret=read_config(cfgfile);
		free(cfgfile);
	}
	return ret;
}


bool read_config_for(const char *module)
{
	return do_read_config_for(module, get_cfgfile_for(module));
}


bool read_config_for_scr(const char *module, int xscr)
{
	return do_read_config_for(module, get_cfgfile_for_scr(module, xscr));
}


/*}}}*/


/*{{{ find_module */


/*char *find_module(const char *fname)
{
	char *tmp, **dir;

	if(fname==NULL)
		return NULL;
	
	for(dir=libpaths; *dir!=NULL; dir++){
		libtu_asprintf(&tmp, "%s/%s", *dir, fname);

		if(tmp==NULL){
			warn_err();
			continue;
		}
	
		if(access(tmp, F_OK)==0)
			return tmp;
		
		free(tmp);
	}
	
	warn("Could not find module %s.", fname);
	
	return NULL;
}*/


/*}}}*/

