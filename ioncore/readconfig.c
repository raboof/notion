/*
 * ion/ioncore/readconfig.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <unistd.h>

#include <libtu/parser.h>
#include <libtu/tokenizer.h>
#include <libtu/output.h>

#include "common.h"
#include "global.h"
#include "readconfig.h"


static char* dummy_paths=NULL;
static char **etcpaths=&dummy_paths;
static int n_etcpaths=0;
static char **libpaths=&dummy_paths;
static int n_libpaths=0;


/*{{{ Init */


bool add_dir(char ***pathsptr, int *n_pathsptr, const char *dir)
{
	char **paths;
	char *dircp=scopy(dir);
	int i;
	
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
	return add_dir(&libpaths, &n_libpaths, dir);
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


static char *do_get_cfgfile_for(bool core, const char *module,
								const char *postfix, bool noaccesstest)
{
	int tryno, psfnotset=(postfix==NULL);
	char *tmp, **dir;
	
	if(module==NULL)
		return NULL;
	
	for(dir=etcpaths; *dir!=NULL; dir++){
		for(tryno=psfnotset; tryno<2; tryno++){
			if(tryno==0)
				libtu_asprintf(&tmp, "%s/%s-%s.conf", *dir, module, postfix);
			else
				libtu_asprintf(&tmp, "%s/%s.conf", *dir, module);
		
			if(tmp==NULL){
				warn_err();
				continue;
			}
	
			if(noaccesstest || access(tmp, F_OK)==0)
				return tmp;
		
			free(tmp);
		}
	}
	
	warn("Could not find configuration file %s.conf.", module);
	
	return NULL;
}


static char *do_get_cfgfile_for_scr(bool core, const char *module,
									int xscr, bool noaccesstest)
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
	
	ret=do_get_cfgfile_for(core, module, tmp, noaccesstest);
	
	free(tmp);
	
	return ret;

fallback:
	ret=do_get_cfgfile_for(core, module, NULL, noaccesstest);
	return ret;
}


char *get_core_cfgfile_for_scr(const char *module, int xscr)
{
	return do_get_cfgfile_for_scr(TRUE, module, xscr, FALSE);
	
}


char *get_core_cfgfile_for(const char *module)
{
	return do_get_cfgfile_for(TRUE, module, NULL, FALSE);
	
}


char *get_cfgfile_for_scr(const char *module, int xscr)
{
	return do_get_cfgfile_for_scr(FALSE, module, xscr, FALSE);
	
}


char *get_cfgfile_for(const char *module)
{
	return do_get_cfgfile_for(FALSE, module, NULL, FALSE);
	
}


char *get_savefile_for_scr(const char *module, int xscr)
{
	return do_get_cfgfile_for_scr(FALSE, module, xscr, TRUE);
	
}


char *get_savefile_for(const char *module)
{
	return do_get_cfgfile_for(FALSE, module, NULL, TRUE);
	
}


/*}}}*/


/*{{{ read_config */


bool read_config(const char *cfgfile, const ConfOpt *opts)
{
	bool retval=FALSE;
	Tokenizer *tokz;
	
	tokz=tokz_open(cfgfile);
	    
	if(tokz==NULL)
		return FALSE;
	
	tokz->flags=TOKZ_ERROR_TOLERANT;
	tokz_set_includepaths(tokz, etcpaths);
	retval=parse_config_tokz(tokz, opts);
	tokz_close(tokz);
	
	return retval;
}


static bool do_read_config_for(char *cfgfile, const ConfOpt *opts)
{
	bool ret=TRUE;
	if(cfgfile!=NULL){
		ret=read_config(cfgfile, opts);
		free(cfgfile);
	}
	return ret;
}


bool read_core_config_for(const char *module, const ConfOpt *opts)
{
	return do_read_config_for(get_core_cfgfile_for(module), opts);
}


bool read_core_config_for_scr(const char *module, int xscr,
							  const ConfOpt *opts)
{
	return do_read_config_for(get_core_cfgfile_for_scr(module, xscr), opts);
}


bool read_config_for(const char *module, const ConfOpt *opts)
{
	return do_read_config_for(get_cfgfile_for(module), opts);
}


bool read_config_for_scr(const char *module, int xscr,
						 const ConfOpt *opts)
{
	return do_read_config_for(get_cfgfile_for_scr(module, xscr), opts);
}


/*}}}*/


/*{{{ find_module */


char *find_module(const char *fname)
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
}


/*}}}*/

