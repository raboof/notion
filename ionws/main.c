/*
 * ion/main.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <stdlib.h>
#include <libtu/util.h>
#include <libtu/optparser.h>

#include <wmcore/common.h>
#include <wmcore/wmcore.h>
#include <wmcore/global.h>
#include <wmcore/readconfig.h>
#include <wmcore/event.h>
#include <wmcore/eventh.h>
#include <wmcore/hooks.h>
#include <wmcore/clientwin.h>
#include <wmcore/exec.h>
#include <query/main.h>
#include "conf.h"
#include "funtabs.h"
#include "../version.h"
#include "placement.h"
#include "workspace.h"
#include "bindmaps.h"


/*{{{ Optparser data */


/* Options. Getopt is not used because getopt_long is quite gnu-specific
 * and they don't know of '-display foo' -style args anyway.
 * Instead, I've reinvented the wheel in libtu :(.
 */
static OptParserOpt opts[]={
	{OPT_ID('d'), 	"display", 	OPT_ARG, "host:dpy.scr", "X display to use"},
	{'c', 			"cfgfile", 	OPT_ARG, "config_file", "Configuration file"},
	{OPT_ID('o'), 	"onescreen", 0, NULL, "Manage default screen only"},
	END_OPTPARSEROPTS
};


static const char ion_usage_tmpl[]=
	"Usage: $p [options]\n\n$o\n";


static const char ion_about[]=
	"Ion " ION_VERSION ", copyright (c) Tuomo Valkonen 1999-2002.\n"
	"This program may be copied and modified under the terms of the "
	"Clarified Artistic License.\n";


static OptParserCommonInfo ion_cinfo={
	ION_VERSION,
	ion_usage_tmpl,
	ion_about
};


/*}}}*/

	
/*{{{ Main */


int main(int argc, char*argv[])
{
	const char *cfgfile=NULL;
	const char *display=NULL;
	bool onescreen=FALSE;
	int opt;
	
	libtu_init(argv[0]);
	
	optparser_init(argc, argv, OPTP_MIDLONG, opts, &ion_cinfo);
	
	while((opt=optparser_get_opt())){
		switch(opt){
		case OPT_ID('d'):
			display=optparser_get_arg();
			break;
		case 'c':
			cfgfile=optparser_get_arg();
			break;
		case OPT_ID('o'):
			onescreen=TRUE;
			break;
		default:
			optparser_print_error();
			return EXIT_FAILURE;
		}
	}

	wglobal.argc=argc;
	wglobal.argv=argv;
	
	if(!wmcore_init("ion-devel", ETCDIR, display, onescreen))
		return EXIT_FAILURE;
	
	init_funclists();
	query_init();
	
	if(!ion_read_config(cfgfile))
		goto configfail;
	
	if(ion_frame_bindmap.nbindings==0 ||
	   ion_moveres_bindmap.nbindings==0)
		goto configfail;
	
	setup_screens();
	
	mainloop();
	
	/* The code should never return here */
	return EXIT_SUCCESS;
	
configfail:
#define MSG \
    "Unable to load configuration or inadequate binding configurations. " \
	"Refusing to start.\nYou *must* install proper configuration files " \
	"either in ~/.ion or "ETCDIR"/ion to use Ion."
	
	warn(MSG);
	setup_environ(DefaultScreen(wglobal.dpy));
	XCloseDisplay(wglobal.dpy);
	wm_do_exec("xmessage 'ion: " MSG "'");
	return FALSE;
}


/*}}}*/

