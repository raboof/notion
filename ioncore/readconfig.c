/*
 * wmcore/readconfig.c
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


static char *etcpaths[]={
	NULL, NULL, NULL
};


static char *libpaths[]={
	NULL, NULL, NULL
};


/*{{{ Init */


bool wmcore_set_paths(const char *appname, const char *etcdir, const char *libdir)
{
	char *p, *home;
	int libi=0, etci=0;
	
	/* Should only be called from wmcore_init */
	assert(appname!=NULL && etcdir!=NULL && libdir!=NULL);
	
	home=getenv("HOME");
	
	if(home==NULL){
		warn("$HOME not set");
	}else{
		char *tmp=scat3(home, "/.", appname);
		if(tmp==NULL){
			warn_err();
		}else{
			etcpaths[etci++]=tmp;
			
			p=scat(tmp, "/lib");
			if(p!=NULL)
				libpaths[libi++]=p;
			else
				warn_err();
		}
	}

	p=scat3(etcdir, "/", appname);
	if(p!=NULL)
		etcpaths[etci++]=p;
	else
		warn_err();
	
	p=scat3(libdir, "/", appname);
	if(p!=NULL)
		libpaths[libi++]=p;
	else
		warn_err();
	
	return (etci>0 && libi>0);
}


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
	
	warn("Could not find configuration file %s.conf\n", module);
	
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
	
	if(tmp==NULL){
		warn_err();
		goto fallback;
	}

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
	
	warn("Could not find module %s\n", fname);
	
	return NULL;
}


/*}}}*/

