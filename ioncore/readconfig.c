/*
 * wmcore/readconfig.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
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


static char *appname=NULL, *appetcdir=NULL;

/* Use application settings for the moment as there is only one program
 * using this library.
 */
#define WMCORENAME appname
#undef ETCDIR
#define ETCDIR appetcdir


/*{{{ Init */


bool wmcore_set_cfgpath(const char *name, const char *etcdir)
{
	/* Should only be called from wmcore_init */
	assert(appname==NULL);
	
	appname=scopy(name);
	if(appname==NULL){
		warn_err();
		return FALSE;
	}
	
	appetcdir=scopy(etcdir);
	if(appetcdir==NULL){
		warn_err();
		free(appname);
		return FALSE;
	}
	
	return TRUE;
}


/*}}}*/


/*{{{ get_cfgfile */


static char *do_get_cfgfile_for(bool core, const char *module,
								const char *postfix, bool noaccesstest)
{
	int tryno, psfnotset=(postfix==NULL);
	char *tmp, *home, *etc, *nm;

	if(module==NULL)
		return NULL;
	
	home=getenv("HOME");
	
	tryno=psfnotset+(home==NULL ? 0 : 2);
	
	if(core){
		nm=WMCORENAME;
		etc=ETCDIR;
	}else{
		nm=appname;
		etc=appetcdir;
	}
	
	for(tryno=psfnotset; tryno<4; tryno+=1+psfnotset){
		switch(tryno){
		case 0:
			/* ~/.appname/module-postfix.conf */
			libtu_asprintf(&tmp, "%s/.%s/%s-%s.conf",
						   home, nm, module, postfix);
			break;
		case 1:
			/* ~/.appname/module.conf */
			libtu_asprintf(&tmp, "%s/.%s/%s.conf",
						   home, nm, module);
			break;
		case 2:
			/* ETCDIR/appname/module-postfix.conf */
			libtu_asprintf(&tmp, "%s/%s/%s-%s.conf",
						   etc, nm, module, postfix);
			break;
		case 3:
			/* ETCDIR/appname/module.conf */
			libtu_asprintf(&tmp, "%s/%s/%s.conf",
						   etc, nm, module);
			break;
		}
		
		if(tmp==NULL){
			warn_err();
			continue;
		}
	
		if(!noaccesstest && access(tmp, F_OK)!=0){
			if(tryno!=3){
				free(tmp);
				continue;
			}
		}
		break;
	}
	
	return tmp;
}


static char *do_get_cfgfile_for_scr(bool core, const char *module,
									WScreen *scr, bool noaccesstest)
{
	char *ret, *tmp, *display, *dpyend;
	
	display=XDisplayName(wglobal.display);
	
	dpyend=strchr(display, ':');
	if(dpyend==NULL)
		goto fallback;
	
	dpyend=strchr(dpyend, '.');
	
	if(dpyend!=NULL)
		return do_get_cfgfile_for(core, module, display, noaccesstest);
	
	libtu_asprintf(&tmp, "%.*s.%d", (int)(dpyend-display)-1,
				   display, scr->xscr);
	
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


char *get_core_cfgfile_for_scr(const char *module, WScreen *scr)
{
	return do_get_cfgfile_for_scr(TRUE, module, scr, FALSE);
	
}


char *get_core_cfgfile_for(const char *module)
{
	return do_get_cfgfile_for(TRUE, module, NULL, FALSE);
	
}


char *get_cfgfile_for_scr(const char *module, WScreen *scr)
{
	return do_get_cfgfile_for_scr(FALSE, module, scr, FALSE);
	
}


char *get_cfgfile_for(const char *module)
{
	return do_get_cfgfile_for(FALSE, module, NULL, FALSE);
	
}


char *get_savefile_for_scr(const char *module, WScreen *scr)
{
	return do_get_cfgfile_for_scr(FALSE, module, scr, TRUE);
	
}


char *get_savefile_for(const char *module)
{
	return do_get_cfgfile_for(FALSE, module, NULL, TRUE);
	
}


/*}}}*/


/*{{{ read_config */


char *includepaths[]={
	NULL, NULL
};


bool read_config(const char *cfgfile, const ConfOpt *opts)
{
	bool retval=FALSE;
	Tokenizer *tokz;
	
	tokz=tokz_open(cfgfile);
	    
	if(tokz==NULL)
		return FALSE;
	
	includepaths[0]=appetcdir;
	
	tokz->flags=TOKZ_ERROR_TOLERANT;
	tokz_set_includepaths(tokz, includepaths);
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


bool read_core_config_for_scr(const char *module, WScreen *scr,
							  const ConfOpt *opts)
{
	return do_read_config_for(get_core_cfgfile_for_scr(module, scr), opts);
}


bool read_config_for(const char *module, const ConfOpt *opts)
{
	return do_read_config_for(get_cfgfile_for(module), opts);
}


bool read_config_for_scr(const char *module, WScreen *scr,
						 const ConfOpt *opts)
{
	return do_read_config_for(get_cfgfile_for_scr(module, scr), opts);
}


/*}}}*/

